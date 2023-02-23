__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import re
import typing

import clang.cindex

from . import model
from .diagnostics import DiagnosticReporter


def get_raw_comment(cursor):
    """Returns the raw comment from the cursor."""
    comment = ""
    if cursor.raw_comment:
        comment = cursor.raw_comment.replace("\r\n", "\n")
    return comment


def is_reference(t):
    """Return True if the given type is a reference."""
    return clang.cindex.TypeKind.LVALUEREFERENCE == t.kind


def is_void(t):
    """Return True if the given type is a void."""
    return clang.cindex.TypeKind.VOID == t.kind


def is_char(t):
    """Return True if the given type is a char."""
    return clang.cindex.TypeKind.CHAR_S == t.kind or clang.cindex.TypeKind.CHAR_U == t.kind


def is_pointer(t):
    """Return True if the given type is a pointer."""
    return clang.cindex.TypeKind.POINTER == t.kind


def is_standard_layout(hierarchy, t, reporter: DiagnosticReporter) -> bool:
    """Return True if the given type is a standard layout type."""

    # these checks aren't perfect, but hit 90% of practical uses. the remaining 10% will be caught by the:
    #
    #   static_assert(std::is_standard_layout<t>::value, ...)
    #
    # we put in the generated api code

    cursor = t.get_declaration()
    chain = hierarchy.get_hierarchy(cursor)
    assert len(chain) > 0

    chain_with_data_members = 0
    member_access_level = clang.cindex.AccessSpecifier.NONE

    for i in reversed(range(len(chain))):
        cursor = chain[i]
        for child in cursor.get_children():
            if clang.cindex.CursorKind.FIELD_DECL == child.kind:
                if chain_with_data_members > i:
                    reporter.warning(
                        child, "declaring data members in a subclass of a class with data members is not ABI safe"
                    )
                    return False
                else:
                    chain_with_data_members = i

                if clang.cindex.AccessSpecifier.NONE == member_access_level:
                    member_access_level = child.access_specifier
                elif member_access_level != child.access_specifier:
                    reporter.warning(
                        child,
                        "all members must have same access level to be ABI safe: expected: {} got: {}",
                        member_access_level,
                        child.access_specifier,
                    )
                    return False

                t = child.type
                while t is not None:
                    if not is_abi_safe_type(hierarchy, get_attributes(child, reporter), t, reporter):
                        reporter.warning(child, f"is not ABI safe")
                        return False

                    if is_pointer(t) and (not hierarchy.is_interface_pointer(t)):
                        t = t.get_pointee()
                    else:
                        t = None
            else:
                if child.is_virtual_method() or child.is_pure_virtual_method():
                    reporter.warning(child, f"virtual methods are not ABI safe")
                    return False

    return True


def is_abi_safe_type(hierarchy, attributes, t, reporter: DiagnosticReporter) -> bool:
    """Return True if the given type is ABI safe (plain-old-data, pointer, or reference)."""
    allow_unsafe_abi = attributes and attributes.allow_unsafe_abi_in_test
    return allow_unsafe_abi or (t.get_canonical().is_pod() or is_void(t)) or is_standard_layout(hierarchy, t, reporter)


def pointee_name(t):
    """Returns the name of the type pointed to by the given pointer type.

    Raises an error if the given type is not a pointer type.
    """
    assert is_pointer(t)
    return t.get_pointee().spelling


def get_full_name(node):
    """Given a cursor, returns the name of the cursor qualified with namespace."""
    name = []
    while node:
        # the node name can be empty for anonymous structs/unions
        if len(node.spelling) > 0:
            name.insert(0, node.spelling)
        parent = node.semantic_parent

        if parent.kind == clang.cindex.CursorKind.TRANSLATION_UNIT:
            node = None
        else:
            node = parent

    full_name = ""
    for i in range(len(name)):
        if i > 0:
            full_name += "::"
        full_name += name[i]

    return full_name


def is_opaque_struct(t):
    """Return True if the given type is a struct/union that is opaque (doesn't have a definition)."""
    canonical = t.get_canonical().get_declaration()
    return (
        (canonical.kind == clang.cindex.CursorKind.STRUCT_DECL)
        or (canonical.kind == clang.cindex.CursorKind.UNION_DECL)
    ) and (canonical.get_definition() is None)


def is_opaque_struct_pointer(t):
    """Return True if the given pointer points to a struct/union that is opaque (doesn't have a definition)."""
    assert is_pointer(t)
    return is_opaque_struct(t.get_pointee())


def get_opaque_struct_pointer_info(t):
    """Returns information about an opaque pointer struct.

    The object returned has the following keys:

      "type"      -> "struct"|"union"
      "namespace" -> [ "ns1", "ns2" ]
      "name"      -> "MyStruct"
    """
    assert is_opaque_struct_pointer(t)
    out = {}
    canonical = t.get_pointee().get_canonical().get_declaration()
    if canonical.kind == clang.cindex.CursorKind.STRUCT_DECL:
        out["type"] = "struct"
    else:
        out["type"] = "union"

    namespaces = []
    names = []
    node = canonical
    while node:
        # the node name can be empty for anonymous structs/unions
        if len(node.spelling) > 0:
            if node.kind == clang.cindex.CursorKind.NAMESPACE:
                namespaces.insert(0, node.spelling)
            else:
                names.insert(0, node.spelling)
        parent = node.semantic_parent

        if parent.kind == clang.cindex.CursorKind.TRANSLATION_UNIT:
            node = None
        else:
            node = parent

    name = ""
    for i in range(len(names)):
        if i > 0:
            name += "::"
        name += names[i]

    out["namespace"] = namespaces
    out["name"] = name

    return out


def get_full_type_name(t):
    type_name = t.get_declaration().type.spelling
    if type_name:
        return type_name
    else:
        return t.spelling


def is_definition(node):
    return node.is_definition and node.get_definition() and (node.get_definition() == node)


def is_definition_or_opaque(node, reporter: DiagnosticReporter):
    attr = get_attributes(node, reporter)
    if attr and attr.opaque:
        return True
    else:
        return is_definition(node)


def should_bind_class(node, reporter: DiagnosticReporter):
    attr = get_attributes(node, reporter)
    return attr and attr.bind_class


def get_attributes(cursor, reporter: DiagnosticReporter):
    """Find the OMNI_ATTR node associated with the given cursor.

    Returns None if there is no OMNI_ATTR.

    Raises an error if more than one OMNI_ATTR is associated with the cursor.
    """
    attributes = None

    for child in cursor.get_children():
        if clang.cindex.CursorKind.ANNOTATE_ATTR == child.kind:
            if model.Attributes.is_valid(child):
                if attributes is None:
                    attributes = model.Attributes(child)
                else:
                    reporter.error(cursor, "OMNI_ATTR cannot be specified more than once on a declaration")

    return attributes


class Hierarchy(object):
    """Database of the C++ class hierarchy."""

    def __init__(self, index, translation_unit, reporter: DiagnosticReporter):
        self._usr2info = {}
        self._reporter = reporter
        index_action = clang.cindex.IndexAction.create(index)
        index_action.index(translation_unit, index_declaration=_index_declaration, client_data=self)

    def add(self, cursor, bases=[]):
        """Adds a class hierarchy.

        cursor is a reference to the class.

        bases is an array of base class names.
        """
        self._usr2info[cursor.get_usr()] = {"cursor": cursor.canonical.type.get_declaration(), "bases": bases}

    def get_hierarchy(self, cursor):
        """Given a cursor (or class name), returns a list of the class hierarchy.

        Example:
            IKeyboard -> [ "omni::input::IKeyboard", "omni::core::IObject" ]
        """
        if isinstance(cursor, str):
            cursor = self._usr2info[cursor]["cursor"]
        return self._get_hierarchy(cursor)

    def is_interface(self, cursor):
        """Returns True if the given CLASS_DECL cursor inherits from omni::core::IObject_abi."""
        chain = self.get_hierarchy(cursor)
        return chain[-1].type.spelling == "omni::core::IObject_abi"

    def is_api(self, cursor):
        """Returns True if the given CLASS_DECL cursor immediately inherits from omni::core::Generated<>."""
        chain = self.get_hierarchy(cursor)
        return (len(chain) > 1) and chain[1].type.spelling.startswith("omni::core::Generated<")

    def is_kit_interface(self, cursor):
        """Returns True if the given CLASS_DECL cursor inherits from carb::IObject."""
        chain = self.get_hierarchy(cursor)
        return chain[-1].type.spelling == "carb::IObject"

    def is_interface_pointer(self, t):
        """Returns True if the given type inherits from omni::core::IObject_abi."""
        if is_pointer(t):
            return self.is_interface(t.get_pointee().get_canonical().get_declaration())
        return False

    def is_kit_interface_pointer(self, t):
        """Returns True if the given type inherits from carb::IObject."""
        if is_pointer(t):
            return self.is_kit_interface(t.get_pointee().get_canonical().get_declaration())
        return False

    def _get_hierarchy(self, node, out: typing.Optional[list] = None):
        out = [] if out is None else out
        if node:
            canonical = node.type.get_canonical().get_declaration()
            if node != canonical:
                # this appears to be a typedef or using, go to the real type
                self._reporter.verbose(None, "not canonical: can:{}", canonical.displayname)
                return self._get_hierarchy(canonical, out)

            out.append(node)

            usr = node.get_usr()
            if usr in self._usr2info:
                self._reporter.verbose(None, "usr: {}", usr)
                class_info = self._usr2info[usr]
                if 1 == len(class_info["bases"]):
                    base = class_info["bases"][0]
                    return self._get_hierarchy(base, out)
            else:
                # base is probably a specialization, which clang doesn't provide in the class hierarchy.
                # we'll have to manually figure things out
                base = self._get_template_base(node)
                if base and (base != node):
                    return self._get_hierarchy(base, out)
        return out

    def _get_template_base(self, node, template_input=[]):
        if (clang.cindex.CursorKind.STRUCT_DECL == node.kind) or (clang.cindex.CursorKind.CLASS_DECL == node.kind):
            specialization = clang.cindex.conf.lib.clang_getSpecializedCursorTemplate(node)
            if specialization:
                # this node is a template specialization. for example, the 'B' node in:
                #
                #   class A : public B<int> { };
                #
                # we have to manually grab the template parameters to the specialization (e.g. int), find the generic
                # template node (e.g. B), substitute the template params into the template, and determine the base
                # class
                specialization_input = []
                for i in range(clang.cindex.conf.lib.clang_Type_getNumTemplateArguments(node.type)):
                    specialization_input.append(
                        clang.cindex.conf.lib.clang_Type_getTemplateArgumentAsType(node.type, i)
                    )
                return self._get_template_base(specialization, specialization_input)
            else:
                return node
        elif clang.cindex.CursorKind.TEMPLATE_REF == node.kind:
            # node is reference to a CLASS_TEMPLATE node
            return self._get_template_base(node.referenced, template_input)
        elif clang.cindex.CursorKind.CLASS_TEMPLATE == node.kind:
            # we're dealing with a generic template. the child nodes contain a list of template parameters
            # and the base class
            template_params = []
            base = None
            for child in node.get_children():
                if (clang.cindex.CursorKind.TEMPLATE_TYPE_PARAMETER == child.kind) or (
                    clang.cindex.CursorKind.TEMPLATE_NON_TYPE_PARAMETER == child.kind
                ):
                    template_params.append(child)
                elif clang.cindex.CursorKind.CXX_BASE_SPECIFIER == child.kind:
                    base = child

            # the template does not have a base class
            if not base:
                return None

            # check if the base class is one of the template parameters. for example:
            #   template <typename T> Foo : public T { };
            for i in range(len(template_params)):
                param = template_params[i]
                if param.spelling == base.spelling:
                    return template_input[i].get_declaration()

            # base is not a template arg
            base_child = list(base.get_children())[0]
            if clang.cindex.CursorKind.TYPE_REF == base_child.kind:
                return self._get_template_base(base_child.referenced)
            elif clang.cindex.CursorKind.TEMPLATE_REF == base_child.kind:
                # we're processing a template and base is a template.  we need to copy over the template inputs.
                # for example:
                #
                #  template <typename B1> class B : public B1 { };
                #  template <typename A1, typename A2> class A : public B<A2> { };
                #  class Foo : public A<int, IBar> { }
                #
                # above, when determining A's base class, we determine that B<A2> needs the second template parameter
                # from A, so we copy that into "base_template_input" below.
                num_base_template_params = clang.cindex.conf.lib.clang_Type_getNumTemplateArguments(base.type)
                base_template_input = []
                for i in range(num_base_template_params):
                    base_template_param_type = clang.cindex.conf.lib.clang_Type_getTemplateArgumentAsType(base.type, i)
                    matched_type = base_template_param_type
                    for param in template_params:
                        if base_template_param_type.spelling == param.spelling:
                            matched_type = template_input[i]
                    base_template_input.append(matched_type)

                return self._get_template_base(base_child, base_template_input)
            else:
                print(f"unhandled CXX_BASE_SPECIFIER child: {base_child.kind}")
                return None
        elif clang.cindex.CursorKind.NO_DECL_FOUND == node.kind:
            return None
        else:
            print(f"!!!!!!!!!!!!!!!!!!   not sure what to do {node.kind} !!!!!!!!!!!!!!!!")
            return None


class InterfaceMatcher(object):
    """Given a cursor to a translation unit, finds all of the interfaces in the translation unit.

    Other interesting entities in the translation unit (such as struct, unions, constants, enum, etc.) are also
    captured.
    """

    def __init__(self, node, hierarchy, filename, reporter: DiagnosticReporter):
        self._reporter = reporter
        self._interfaces = []
        self._enums = []
        self._flags = []
        self._constant_groups = []
        self._structs = []
        self._filename = filename
        self._hierarchy = hierarchy
        self._traverse(node, reporter)

    @property
    def interfaces(self):
        """Returns the list of interfaces (as Interface objects)."""
        return self._interfaces

    @property
    def enums(self):
        """Returns the list of enums (as Enum objects)."""
        return self._enums

    @property
    def flags(self):
        """Returns the list of groups of constants representing bit flags (as ConstantGroup objects)."""
        return self._flags

    @property
    def constant_groups(self):
        """Returns the list of groups of constants (as ConstantGroup objects)."""
        return self._constant_groups

    @property
    def structs(self):
        """Returns the list of unions and structs (as Struct objects)."""
        return self._structs

    @property
    def filename(self):
        """Returns the filename of the tranlsation unit."""
        return self._filename

    def _traverse(self, node, reporter: DiagnosticReporter):
        from pathlib import Path

        if node.location.file and (Path(node.location.file.name) != Path(self.filename)):
            return
        if (
            ((clang.cindex.CursorKind.CLASS_DECL == node.kind) or (clang.cindex.CursorKind.STRUCT_DECL == node.kind))
            and self._class_starts_with_i(node.type)
            and is_definition(node)
        ):
            if self._hierarchy.is_interface(node):
                if not self._hierarchy.is_api(node):
                    if not node.displayname.endswith("_abi"):
                        reporter.error(node, "interface does not end with '_abi'")
                    if not (clang.cindex.CursorKind.CLASS_DECL == node.kind):
                        reporter.error(node, "interface must be declared as a class")
                    self._interfaces.append(model.Interface(node, self._hierarchy, self._reporter))
            elif clang.cindex.CursorKind.CLASS_DECL == node.kind:
                reporter.warning(node, "ignoring {}, which may be an interface", node.displayname)
            elif clang.cindex.CursorKind.STRUCT_DECL == node.kind:
                self._structs.append(model.Struct(node, self._hierarchy, self._reporter))
        elif (clang.cindex.CursorKind.STRUCT_DECL == node.kind) and is_definition_or_opaque(node, reporter):
            self._structs.append(model.Struct(node, self._hierarchy, self._reporter))
        elif (
            (clang.cindex.CursorKind.CLASS_DECL == node.kind)
            and should_bind_class(node, reporter)
            and is_definition_or_opaque(node, reporter)
        ):
            self._structs.append(model.Struct(node, self._hierarchy, self._reporter, "class"))
        elif (clang.cindex.CursorKind.UNION_DECL == node.kind) and is_definition_or_opaque(node, reporter):
            self._structs.append(model.Struct(node, self._hierarchy, self._reporter, "union"))
        elif (clang.cindex.CursorKind.TYPEDEF_DECL == node.kind) or (
            clang.cindex.CursorKind.TYPE_ALIAS_DECL == node.kind
        ):
            self._traverse_typedef_decl(node)
        elif clang.cindex.CursorKind.VAR_DECL == node.kind:
            self._traverse_var_decl(node)
        elif clang.cindex.CursorKind.ENUM_DECL == node.kind:
            self._traverse_enum_decl(node)
        else:
            for child in node.get_children():
                self._traverse(child, reporter)

    def _class_starts_with_i(self, t):
        m = re.search(r":?I[A-Z][\w\d_]*$", t.spelling)
        if m:
            return True
        return False

    def _traverse_typedef_decl(self, node):
        attributes = get_attributes(node, self._reporter)
        if not (attributes is None):
            if attributes.flag:
                self._flags.append(model.ConstantGroup(node, self._hierarchy, self._reporter))
            elif attributes.constant:
                self._constant_groups.append(model.ConstantGroup(node, self._hierarchy, self._reporter))

    def _traverse_var_decl(self, node):
        for child in node.get_children():
            if clang.cindex.CursorKind.TYPE_REF == child.kind:
                for flag in self._flags:
                    if child.referenced == flag.cursor:
                        flag.add(node)
                        return
                for group in self._constant_groups:
                    if child.referenced == group.cursor:
                        group.add(node)
                        return

    def _traverse_enum_decl(self, node):
        if node.spelling == "":
            return  # skip anonymous enums
        self._enums.append(model.Enum(node, self._hierarchy, self._reporter))


def _index_declaration(hierarchy, decl_info):
    reporter = hierarchy._reporter
    reporter.verbose(
        decl_info.cursor,
        "{displayname} usr:{usr} def:{isdef} redecl:{isredecl} imp:{isimplconv} temp:{tmplkind}",
        displayname=decl_info.cursor.displayname,
        usr=decl_info.cursor.get_usr(),
        isdef=decl_info.is_definition,
        isredecl=decl_info.is_redeclaration,
        isimplconv=decl_info.is_implicit,
        tmplkind=decl_info.entity_info.template_kind,
    )

    if not decl_info.is_definition:
        return

    class_info = decl_info.cxx_class_decl_info
    bases = []
    if class_info:
        with reporter.nest_verbose(decl_info.cursor, "Bases") as subreporter:
            for base in class_info.bases:
                bases.append(base.cursor.canonical.type.get_declaration())

                subreporter.verbose(
                    base.cursor,
                    "base:{} canonical_usr:{}",
                    base.cursor.displayname,
                    base.cursor.canonical.type.get_declaration().get_usr(),
                )

    hierarchy.add(decl_info.cursor, bases)


def get_includes(translation_unit):
    from pathlib import Path

    includes = {}
    for inc in translation_unit.get_includes():
        path = Path(str(inc.include))
        path = path.resolve()
        if path.exists():
            includes[str(path)] = True

    return includes
