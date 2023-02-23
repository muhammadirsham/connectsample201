__copyright__ = "Copyright (c) 2020-2022, NVIDIA CORPORATION. All rights reserved."
__license__ = """
NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

import re

import clang.cindex

from .diagnostics import DiagnosticReporter


class Attributes(object):
    """Base class for grabbing OMNI_ATTR information from a cursor's children."""

    def __init__(self, cursor):
        # fmt: off
        attrs = {
            # pointer id never NULL
            "not_null" : False,

            # pointer points to read-only data.
            #
            # in c++, use OMNI_ATTR(in). we have to rename 'in' to 'input' here because 'in' is a python keyword.
            "input" : False,

            # pointer points to write-only data.
            #
            # in c++, use OMNI_ATTR(out). we have to rename 'out' to 'output' to match the required 'in' to 'input'
            # renaming.
            "output" : False,

            # do not generate python bindings
            "no_py" : False,

            # do no generate the nice c++ api wrapper
            "no_api" : False,

            # typedef represents a bit flag type
            "flag" : False,

            # typedef represents a group of related constants
            "constant" : False,

            # prefix for either a 'flag', 'enum', or 'constant'
            #
            # example: OMNI_ATTR(constant, prefix=kResult)
            "prefix" : None,

            # the given const char* pointer points to a string and not a single char.
            "c_str" : False,

            # the pointer points to an array. the size of the array can be found in 'count'
            #
            # example: OMNI_ATTR(in, count=dataCount)
            "count" : None,

            # the method registers a consumer. the given parameter should be the callback method
            #
            # example: OMNI_ATTR(consumer=onDrop_abi)
            "consumer" : None,

            # internal flag. when `count` is given, `count_of` points to the array parameter.
            "count_of" : None,

            # marks a get/set method as not a getter/setter for a property
            "not_prop" : False,

            # disable checking if type is ABI safe. useful for test code when passing complex types as POD in order to
            # test the code generator.
            "allow_unsafe_abi_in_test" : False,

            # the given field should be a named argument in the constructor. only needed when the constructor arguments
            # are ambiguous (such as when wrapping a union).
            "init_arg" : False,

            # the given type is a math vec type (e.g. float2, int3, etc.)
            "vec" : False,

            # the returned pointer points to hidden data
            "opaque" : False,

            # in addition to making the method a property getter, also bind the raw get function
            "py_get" : False,

            # in addition to making the method a property setter, also bind the raw get function
            "py_set" : False,

            # the setter/getter method should not be bound as a property in python
            "py_not_prop" : False,

            # use the given name when binding in python
            "py_name" : None,

            # if true, treat a class as a struct with methods.  by default non-interface classes are ignored.
            "bind_class" : False,

            # if true, the api layer should use a reference
            "ref" : False,

            # if true, the returned interface is not internally acquired
            "no_acquire" : False,

            # if true, if the api method should throw when the returned omni::Result is a failure
            "throw_result" : False,

            # if true, return the parameter from the method
            "returned" : False,

            # if true, throws an exception if the parameter is nullptr
            "throw_if_null" : False,
        }
        # fmt: on

        for key, value in attrs.items():
            setattr(self, key, value)

        self.pointee = None

        if not (cursor is None):
            self._parse(cursor)

    def _parse(self, cursor):
        # the annotation is in the format: "omni_attr:in,out,count=dataCount"
        # split out right side of the string
        m = re.match("^omni_attr:(.+)$", cursor.spelling)
        if m:
            s = m.groups()[0]
        else:
            raise ValueError(f"{cursor.location}: unknown omni attr format: {cursor.spelling}")

        # string is now: "in,out,count=dataCount"
        # tokenize based on commas
        tokens = [x.strip() for x in s.split(",")]
        for t in tokens:
            if t.find("=") != -1:
                # the token is of the form:
                #
                #   "count=dataCount"
                #   "*count=dataCount"
                #   "**count=dataCount"
                #
                # split the string into a key/value
                left, value = [x.strip() for x in t.split("=")]
                key, attributes = self._get_key_and_attributes(left)
            else:
                # the token is of the form:
                #
                #   "in"
                #   "*in"
                #   "**out"
                value = True
                key, attributes = self._get_key_and_attributes(t)

            if hasattr(attributes, key):
                setattr(attributes, key, value)
            elif key == "in":  # 'in' is a python reserved word so we name this internally 'input'
                attributes.input = value
            elif key == "out":  # we internally call 'out' 'output' to match 'input'
                attributes.output = value
            elif key == "return":  # 'return' is a python reserved word so we name this internally 'returned'
                attributes.returned = value
            else:
                raise ValueError(f"{cursor.location}: unknown OMNI_ATTR: '{key}'")

    def _get_key_and_attributes(self, key):
        # an attribute can start with any number of '*' where each '*' represent a pointer being dereferenced
        level = 0
        while key.startswith("*"):
            key = key[1:]
            level += 1

        # based on the current deref level, find the appropriate attributes object. if one doesn't exists, we create it.
        attributes = self
        for i in range(level):
            if not attributes.pointee:
                attributes.pointee = Attributes(None)
            attributes = attributes.pointee

        return key, attributes

    @staticmethod
    def is_valid(cursor):
        return cursor.spelling.startswith("omni_attr:")


class CursorObject(object):
    """Object backed by a node (Cursor) in the AST."""

    def __init__(self, cursor, hierarchy, reporter: DiagnosticReporter):
        from . import extract

        assert isinstance(reporter, DiagnosticReporter)
        self._reporter = reporter

        self.cursor = cursor
        self.hierarchy = hierarchy

        self.attributes = extract.get_attributes(cursor, reporter)
        if self.attributes is None:
            self.attributes = Attributes(None)

    @property
    def comment(self):
        """Returns the raw C++ comment (including leading //)."""
        from . import extract

        return extract.get_raw_comment(self.cursor)

    @property
    def reporter(self) -> DiagnosticReporter:
        return self._reporter


class Parameter(CursorObject):
    """Parameter passed to a method."""

    def __init__(self, cursor, index, method, reporter: DiagnosticReporter):
        super().__init__(cursor, method.hierarchy, reporter)

        # the parameter may be unnamed. if so, create a dummy name.
        self._name = self.cursor.spelling
        if self._name == "":
            self._name = f"arg{index}"

        self._is_interface_pointer = self.hierarchy.is_interface_pointer(self.cursor.type)
        self._is_kit_interface_pointer = self.hierarchy.is_kit_interface_pointer(self.cursor.type)

        self._no_acquire = False

        if self.is_pointer:
            self._is_pointer_to_interface_pointer = self.hierarchy.is_interface_pointer(self.cursor.type.get_pointee())
        else:
            self._is_pointer_to_interface_pointer = False

        with reporter.nest_verbose(cursor, "checking ABI safety") as subreporter:
            self._validate_attributes(subreporter)
            subreporter.validate()

    def _validate_attributes(self, reporter: DiagnosticReporter):
        from . import extract

        self._returned_indirection = ""
        self._is_returned = False

        attributes = self.attributes
        cursor = self.cursor
        t = cursor.type
        indirection = ""
        while not (t is None):
            if not extract.is_abi_safe_type(self.hierarchy, attributes, t, reporter):
                reporter.error(cursor, "type is not ABI safe: {}", t.spelling)

            if not self.hierarchy.is_interface_pointer(t):
                if attributes.returned:
                    self._returned_indirection = indirection
                    self._is_returned = True

                if attributes.no_acquire:
                    reporter.error(
                        cursor, f"'{indirection}no_acquire' OMNI_ATTR must be used with an interface pointer"
                    )

                if extract.is_pointer(t):
                    pointee = t.get_pointee()
                    if not attributes:
                        reporter.error(
                            cursor, "insufficient pointer information supplied to OMNI_ATTR (did you forget 'in')"
                        )
                    if attributes.c_str:
                        if not extract.is_char(pointee):
                            reporter.error(cursor, "'{}c_str' OMNI_ATTR must be on a char pointer", indirection)
                        if attributes.output:
                            reporter.error(cursor, f"'{indirection}c_str' OMNI_ATTR cannot be combined with 'out'")
                        if not pointee.is_const_qualified():
                            reporter.error(cursor, f"'{indirection}c_str' OMNI_ATTR must be on a const pointer")

                    if attributes.ref:
                        if attributes.c_str:
                            reporter.error(cursor, f"'{indirection}ref' OMNI_ATTR cannot be combined with 'c_str'")
                        if attributes.count:
                            reporter.error(cursor, f"'{indirection}ref' OMNI_ATTR cannot be combined with 'count'")
                        if not self.attributes.not_null:
                            reporter.error(cursor, f"'{indirection}ref' OMNI_ATTR must also specify 'not_null'")

                    if extract.is_opaque_struct_pointer(t):
                        if attributes.input:
                            reporter.error(cursor, f"'{indirection}in' OMNI_ATTR must not be on opaque types")
                        if attributes.output:
                            reporter.error(cursor, f"'{indirection}out' OMNI_ATTR must not be on opaque types")
                        if attributes.c_str:
                            reporter.error(cursor, f"'{indirection}c_str' OMNI_ATTR must not be on opaque types")
                        t = None
                    else:
                        if attributes.input and (not attributes.output):
                            # in
                            if not pointee.is_const_qualified():
                                reporter.error(cursor, f"'{indirection}in' OMNI_ATTR must be on a const pointer")
                        elif attributes.output:
                            # out or inout
                            if pointee.is_const_qualified():
                                reporter.error(cursor, f"'{indirection}out' OMNI_ATTR must be on a non-const pointer")
                        elif not attributes.c_str:
                            reporter.error(
                                cursor,
                                f"pointer must have '{indirection}c_str', '{indirection}in', and/or '{indirection}out' OMNI_ATTR",
                            )

                        t = pointee
                        if attributes.pointee is None:
                            attributes.pointee = Attributes(None)
                        attributes = attributes.pointee
                    indirection += "*"
                else:
                    if attributes.ref:
                        reporter.error(cursor, f"'{indirection}ref' OMNI_ATTR must be used with a pointer")
                    t = None
            else:
                # interface
                if not extract.is_pointer(t):
                    reporter.error(cursor, "cannot pass interfaces by value")
                if not attributes.count:
                    # not an array of interfaces
                    if attributes.input or attributes.output:
                        reporter.error(cursor, "interface pointers are implicitly 'in' and 'out'")
                if attributes.ref:
                    reporter.error(cursor, f"'{indirection}ref' OMNI_ATTR must not be on interface pointers")
                if attributes.returned:
                    self._returned_indirection = indirection
                    self._is_returned = True
                if attributes.no_acquire:
                    self._no_acquire = True

                t = None

        if self.is_returned and self._returned_indirection == "":
            reporter.error(
                cursor, f"'return' OMNI_ATTR must be used with a pointer and currently prefixed with one or more '*'"
            )

    @property
    def is_interface_pointer(self):
        """True if the type of the parameter inherits from omni::core::IObject."""
        return self._is_interface_pointer

    @property
    def is_kit_interface_pointer(self):
        """True if the type of the parameter inherits from carb::IObject."""
        return self._is_kit_interface_pointer

    @property
    def name(self):
        """Name of the parameter (e.g. 'foo' in 'int foo')."""
        return self._name

    @property
    def type_name(self):
        """Name of the type of the parameter. The name returned is qualified with namespaces.

        Example:
            int foo             ->  "int"
            IKeyboard* k        ->  "omni::input::IKeyboard*"
            const IKeyboard* k  ->  "const omni::input::IKeyboard*"
        """
        from . import extract

        return extract.get_full_type_name(self.cursor.type)

    @property
    def _pointee_type(self):
        from . import extract

        assert extract.is_pointer(self.cursor.type)
        return self.cursor.type.get_pointee()

    @property
    def as_interface(self):
        """Based on the parameter's type, returns the parameter as an Interface.

        Raises an error if the type of the parameter is not a pointer to an interface.
        """
        return Interface(self._pointee_type.get_declaration(), self.hierarchy, self.reporter)

    @property
    def pointee_name(self):
        """Returns the type name of the pointee. The name returned is qualified with namespaces.

        Example:
            int* foo       ->  "int"
            IKeyboard* k  ->  "omni::input::IKeyboard"

        Raises an error if the type of the parameter is not a pointer.
        """
        return self._pointee_type.spelling

    @property
    def pointee_pointee_name(self):
        """Returns the type name of the what the pointee points to. The name returned is qualified with namespaces.

        Example:
            int** foo       ->  "int"
            IKeyboard** k  ->  "omni::input::IKeyboard"

        Raises an error if the type of the parameter is not a pointer to a pointer.
        """
        return self._pointee_type.get_pointee().spelling

    @property
    def is_array(self):
        """Returns if the parameter is a pointer to an array."""
        return not (self.attributes.count is None)

    @property
    def is_pointer(self):
        """Returns if the parameter is a pointer."""
        from . import extract

        return extract.is_pointer(self.cursor.type)

    @property
    def is_pointer_to_pointer(self):
        """Returns if the parameter is a pointer to a pointer."""
        from . import extract

        return extract.is_pointer(self._pointee_type)

    @property
    def is_const_pointer(self):
        """Return true if the pointer points to const data (e.g. const char* -> True)."""
        if self.is_pointer:
            return self.cursor.type.get_pointee().is_const_qualified()
        return False

    @property
    def is_struct(self):
        """Returns true if the parameter is a struct or union being passed by value."""
        return clang.cindex.TypeKind.RECORD == self.cursor.type.kind

    @property
    def is_va_list(self):
        """True if the type of the parameter is va_list."""
        return self.cursor.type.spelling == "va_list"

    @property
    def is_opaque_struct_pointer(self):
        """True if the type of the parameter points to an opaque type."""
        from . import extract

        return extract.is_pointer(self.cursor.type) and extract.is_opaque_struct_pointer(self.cursor.type)

    @property
    def opaque_struct_pointer_info(self):
        """Returns info on the opaque struct pointer parameter. Assumes this Parameter is opaque."""
        from . import extract

        assert extract.is_opaque_struct_pointer(self.cursor.type)
        return extract.get_opaque_struct_pointer_info(self.cursor.type)

    @property
    def can_be_a_reference(self):
        """Returns True if the parameter can be passed-by-reference."""
        return self.attributes.ref

    @property
    def not_null(self):
        """Returns True if the parameter must not be null."""
        return self.attributes.not_null

    @property
    def throw_if_null(self):
        """Returns True if the API layer should throw an exception if the parameter is null."""
        return self.attributes.throw_if_null

    @property
    def is_returned(self):
        """Returns True if the parameter should be returned."""
        return self._is_returned

    @property
    def returned_indirection(self):
        """Returns how many pointer jumps are needed to return the parameter."""
        return self._returned_indirection

    @property
    def is_pointer_to_interface_pointer(self):
        """Returns True if the parameter is a pointer to an interface pointer (useful for ouput pointers)."""
        return self._is_pointer_to_interface_pointer

    @property
    def no_acquire(self):
        """Returns True if the parameter should not have acquire() on it before being returned (useful for output pointers)."""
        return self._no_acquire


class Method(CursorObject):
    """A method in an interface."""

    def __init__(self, cursor, interface, reporter: DiagnosticReporter):
        from . import extract

        super().__init__(cursor, interface.hierarchy, reporter)

        self.parameters = []
        self.interface = interface

        # strip _abi from method name
        if not cursor.spelling.endswith("_abi"):
            reporter.error(cursor, "interface ABI methods must end in '_abi'")
        self._name = cursor.spelling[:-4]

        if not cursor.is_pure_virtual_method():
            reporter.error(cursor, "method must be pure virtual")

        if clang.cindex.ExceptionSpecificationKind.BASIC_NOEXCEPT != cursor.exception_specification_kind:
            reporter.error(cursor, "noexcept must be specified")

        result = cursor.type.get_result()
        if not extract.is_abi_safe_type(self.hierarchy, self.attributes, result, reporter):
            reporter.error(cursor, "return type is not ABI safe")

        if clang.cindex.AccessSpecifier.PROTECTED != cursor.access_specifier:
            reporter.error(cursor, "ABI method must be protected")

        # iterate over children
        parameter_cursors = []
        for child in cursor.get_children():
            if clang.cindex.CursorKind.PARM_DECL == child.kind:
                parameter_cursors.append(child)

        # add parameters
        for i in range(len(parameter_cursors)):
            self.parameters.append(Parameter(parameter_cursors[i], i, self, reporter))

        # some parameters are the the array size (i.e. "count") of another parameter. here we mark them as such (we
        # generally don't want to bind these parameters as function arguments in non-C bindings).
        for param in self.parameters:
            if param.attributes.count:
                for p in self.parameters:
                    if p.name == param.attributes.count:
                        p.attributes.count_of = param
                        break

    @property
    def property_setter_name(self):
        """If this is a "set" method, return the name of the property.

        None is returned if this is not a property setter method.

        If the 'not_prop" attribute has been specified on the method, None is returned

        For example:

            setLastName -> "lastName"
            myFunc -> None
        """
        if self.attributes.not_prop:
            return None
        if 1 != len(self.parameters):
            return None
        m = re.match("^set([A-Z].*)", self.name)
        if m:
            return self._lower_first(m.groups()[0])
        return None

    @property
    def property_getter_name(self):
        """If this is a "get" method, return the name of the property.

        None is returned if this is not a property getter method.

        If the 'not_prop" attribute has been specified on the method, None is returned

        For example:

            getLastName -> "lastName"
            isEnabled -> "enabled"
            myFunc -> None
        """
        if self.attributes.not_prop:
            return None
        if 0 != len(self.parameters):
            return None
        m = re.match("^get([A-Z].*)", self.name)
        if m:
            return self._lower_first(m.groups()[0])
        m = re.match("^is([A-Z].*)", self.name)
        if m:
            return self._lower_first(m.groups()[0])
        return None

    def _lower_first(self, s):
        return s[0].lower() + s[1:]

    @property
    def is_property(self):
        """Returns True if this method is a getter or setter for a property."""
        return (self.property_getter_name is not None) or (self.property_setter_name is not None)

    @property
    def return_type_name(self):
        """Returns the name of the type returned by the method.

        The name returned is qualified with namespaces.

        Examples:
            void foo_abi(int x) noexcept = 0;            ->   "void"
            IKeyboard* bar_abi(int x) noexcept = 0;      ->   "omni::input::IKeyboard*"
            const Payload* fooBar_abi() noexcept = 0;    ->   "const Payload*"
        """
        from . import extract

        return extract.get_full_type_name(self.cursor.type.get_result())

    @property
    def return_type_pointee_name(self):
        """Returns the name of the type pointed to by the return value.

        The name returned is qualified with namespaces.

        Raises an error if the return type is not a pointer

        Examples:
            void foo_abi(int x) noexcept = 0;            ->   Error!
            IKeyboard* bar_abi(int x) noexcept = 0;      ->   "omni::input::IKeyboard"
            const Payload* fooBar_abi() noexcept = 0;    ->   "const Payload"
        """
        from . import extract

        result = self.cursor.type.get_result()
        return extract.pointee_name(result)

    @property
    def returns_interface_pointer(self):
        """Returns True if the method returns an interface (omni::core::IObject) pointer.

        Examples:
            IMyInterface* bar_abi(int x) noexcept = 0;   ->   True
            const Payload* fooBar_abi() noexcept = 0;    ->   False
        """
        from . import extract

        result = self.cursor.type.get_result()
        if extract.is_pointer(result):
            return self.interface.hierarchy.is_interface(result.get_pointee().get_canonical().get_declaration())
        return False

    @property
    def returns_interface_pointer_without_acquire(self):
        """Returns True if the method returns an ONI interface pointer but does not internally call acquire().

        Examples:
            IMyInterface* bar_abi(int x) noexcept = 0;                 ->   False
            IMyInterface* harWithoutAcquire_abi(int x) noexcept = 0;   ->   True
            Payload* fooBar_abi() noexcept = 0;                        ->   False
        """
        return (self.name.endswith("WithoutAcquire") or self.attributes.no_acquire) and self.returns_interface_pointer

    @property
    def returns_kit_interface_pointer(self):
        """Returns True if the method returns a kit interface (carb::IObject) pointer.

        Examples:
            IMyInterface* bar_abi(int x) noexcept = 0;   ->   True
            const Payload* fooBar_abi() noexcept = 0;    ->   False
        """
        from . import extract

        result = self.cursor.type.get_result()
        if extract.is_pointer(result):
            return self.interface.hierarchy.is_kit_interface(result.get_pointee().get_canonical().get_declaration())
        return False

    @property
    def returns_pointer(self):
        """Returns True if the method returns a pointer."""
        from . import extract

        return extract.is_pointer(self.cursor.type.get_result())

    @property
    def returns_void(self):
        """Returns True if the method returns void."""
        return clang.cindex.TypeKind.VOID == self.cursor.type.get_result().kind

    @property
    def returns_opaque_struct_pointer(self):
        """Returns True if the method returns an pointer to an opaque struct/union."""
        from . import extract

        return self.returns_pointer and extract.is_opaque_struct_pointer(self.cursor.type.get_result())

    @property
    def return_type_opaque_struct_pointer_info(self):
        from . import extract

        assert extract.is_opaque_struct_pointer(self.cursor.type.get_result())
        return extract.get_opaque_struct_pointer_info(self.cursor.type.get_result())

    @property
    def name(self):
        """Returns the name of the method (without the _abi postfix).

        The returned name does not have namespaces.

        Example:
            void foo_abi(int x) noexcept = 0;            ->   "foo"
            IMyInterface* bar_abi(int x) noexcept = 0;   ->   "bar"
            const Payload* fooBar_abi() noexcept = 0;    ->   "fooBar"

        See also: abi_name
        """
        return self._name

    @property
    def abi_name(self):
        """Returns the name of the method.

        The returned name does not have namespaces.

        Example:
            void foo_abi(int x) noexcept = 0;            ->   "foo_abi"
            IMyInterface* bar_abi(int x) noexcept = 0;   ->   "bar_abi"
            const Payload* fooBar_abi() noexcept = 0;    ->   "fooBar_abi"

        See also: name
        """
        return self.cursor.spelling

    @property
    def parameters_as_abi_text(self):
        """Returns a string containing the program parameter list and a string containing parameter names.

        Example:
            void foo_abi(int x, float y) noexcept = 0;            ->   ("int x, float y", "x, y")
        """
        parameters_text = ""
        parameter_names_text = ""
        separator = ""
        for i in range(len(self.parameters)):
            param = self.parameters[i]
            parameters_text += f"{separator}{param.type_name} {param.name}"
            parameter_names_text += f"{separator}{param.name}"
            separator = ", "

        return parameters_text, parameter_names_text

    def get_previous_parameter(self, param):
        prev = None
        for p in self.parameters:
            if p is param:
                return prev
            prev = p

        return None

    @property
    def throws_result(self):
        """Returns true if the return type is omni::core::Result and the method throws omni::core::ResultError

        Also see self.returns_result.
        """
        return self.attributes.throw_result and (self.return_type_name == "omni::core::Result")

    @property
    def can_return_reference(self):
        """Returns true if the return type is a pointer and has the "ref" attrbute set."""

        return self.attributes.ref and self.returns_pointer


class ConstantGroup(CursorObject):
    """A grouping of related constants (such as enum, flags)."""

    def __init__(self, cursor, hierarchy, reporter: DiagnosticReporter):
        super().__init__(cursor, hierarchy, reporter)
        self.values = []
        self._prefix = self.attributes.prefix
        if self._prefix is None:
            self._prefix = ""

    @property
    def prefix(self):
        """Returns the groups prefix (e.g. kResultSuccess -> "kResult")

        This property is determined by the 'prefix' attribute.
        """
        return self._prefix

    @property
    def name(self):
        """Returns the type name of the group (e.g. KeyboardKey -> "omni::input::KeyboardKey").

        The returned name is qualified with namespaces.

        See also: short_name.
        """
        return self.cursor.displayname

    @property
    def short_name(self):
        """Returns the type name of the group without any namespaces.

        See also: name.
        """
        return self.cursor.spelling

    def add(self, value):
        """Adds a node as a part of the group."""
        if not value.spelling.startswith(self._prefix):
            self.reporter.warning(
                value, "skipping {} because it does not start with '{}'", value.displayname, self._prefix
            )
            return
        self.values.append(PrefixedValue(value, self.hierarchy, value.spelling[len(self._prefix) :], self.reporter))


class PrefixedValue(CursorObject):
    """Represents a value (constant, enum value, etc.) that has a known prefix."""

    def __init__(self, cursor, hierarchy, no_prefix_name, reporter: DiagnosticReporter):
        from . import extract

        super().__init__(cursor, hierarchy, reporter)

        self._name = extract.get_full_name(cursor)
        self._no_prefix_short_name = no_prefix_name

    @property
    def name(self):
        """Returns the name of the value (with namespaces).

        Example:
            namespace omni { constexpr Result kResultSuccess = 0; }   -> "omni::kResultSuccess"

        See also: short_name, no_prefix_short_name
        """
        return self._name

    @property
    def short_name(self):
        """Returns the name of the value (without namespaces).

        Example:
            namespace omni { constexpr Result kResultSuccess = 0; }   -> "kResultSuccess"

        See also: name, no_prefix_short_name
        """
        return self.cursor.spelling

    @property
    def no_prefix_short_name(self):
        """Returns the name of the value (without namespaces and without the prefix).

        Example:
            namespace omni { constexpr Result kResultSuccess = 0; }   -> "Success"

        See also: name, short_name
        """
        return self._no_prefix_short_name


class Enum(CursorObject):
    """Represents the groups of values in an enumeration (enum)."""

    def __init__(self, cursor, hierarchy, reporter: DiagnosticReporter):
        from . import extract

        super().__init__(cursor, hierarchy, reporter)

        self.values = []
        self._name = extract.get_full_name(cursor)

        prefix = self.attributes.prefix
        if not prefix:
            prefix = ""
        for child in cursor.get_children():
            if child.kind == clang.cindex.CursorKind.ENUM_CONSTANT_DECL:
                if not child.spelling.startswith(prefix):
                    reporter.warning(
                        child, "skipping {} because it does not start with '{}'", child.displayname, prefix
                    )
                else:
                    self.values.append(PrefixedValue(child, self.hierarchy, child.spelling[len(prefix) :], reporter))

    @property
    def name(self):
        """Returns the name of the enum (with namespaces).

        Example:
            namespace omni { enum Foo { /* */ }; }   -> "omni::Foo"

        See also: short_name, no_prefix_short_name
        """
        return self._name

    @property
    def short_name(self):
        """Returns the name of the enum (without namespaces).

        Example:
            namespace omni { enum Foo { /* */ }; }   -> "Foo"

        See also: short_name, no_prefix_short_name
        """
        return self.cursor.spelling


class Interface(CursorObject):
    """Represents an object that inherits from omni::core::IObject."""

    def __init__(self, cursor, hierarchy, reporter: DiagnosticReporter):
        super().__init__(cursor, hierarchy, reporter)
        if cursor.type.spelling.endswith("_abi"):
            self.abi_name = cursor.type.spelling
            self.name = self._remove_abi_postfix(self.abi_name)
        else:
            self.abi_name = f"{cursor.type.spelling}_abi"
            self.name = cursor.type.spelling
        self.methods = []

        for child in cursor.get_children():
            if clang.cindex.CursorKind.CXX_METHOD == child.kind:
                self.methods.append(Method(child, self, reporter))

        self.baseName = None
        chain = hierarchy.get_hierarchy(cursor)
        for b in chain:
            baseName = b.type.spelling
            if baseName.startswith("omni::core::Api"):
                self.baseName = baseName
                break
        if self.baseName is None:
            self.baseName = "omni::core::IObject"

        self.properties = {}
        for method in self.methods:
            prop = method.property_setter_name
            if prop:
                if not prop in self.properties:
                    self.properties[prop] = {"set": method, "get": None}
                else:
                    self.properties[prop]["set"] = method

            prop = method.property_getter_name
            if prop:
                if not prop in self.properties:
                    self.properties[prop] = {"set": None, "get": method}
                else:
                    self.properties[prop]["get"] = method

    @property
    def short_name(self):
        """Returns the name of the interface (without namespaces and without the '_abi' postfix.)"""
        return self._remove_abi_postfix(self.cursor.spelling)

    def _remove_abi_postfix(self, s):
        """Remove '_abi' from the end of a string (e.g. 'name_abi' -> 'name')."""
        return s[: s.index("_abi")]


class Struct(CursorObject):
    """Represents a struct or union."""

    def __init__(self, cursor, hierarchy, reporter: DiagnosticReporter, keyword="struct"):
        super().__init__(cursor, hierarchy, reporter)

        self._fields = []
        self._keyword = keyword
        self._in_union = 0

        keyword_plural = str.format("{}{}", keyword, "es" if keyword.endswith("s") else "s")

        for child in cursor.get_children():
            if clang.cindex.CursorKind.FIELD_DECL == child.kind:
                self._add_field(child)
            elif clang.cindex.CursorKind.UNION_DECL == child.kind:
                self._add_record(child, "union")
            elif clang.cindex.CursorKind.STRUCT_DECL == child.kind:
                self._add_record(child, "struct")
            elif clang.cindex.CursorKind.CXX_METHOD == child.kind:
                if "class" != self._keyword:
                    reporter.error(child, "{} must not contain methods", keyword_plural)
            elif clang.cindex.CursorKind.CONSTRUCTOR == child.kind:
                if "class" != self._keyword:
                    reporter.error(child, "{} must not contain constructors", keyword_plural)
            elif clang.cindex.CursorKind.CXX_ACCESS_SPEC_DECL == child.kind:
                if "class" != self._keyword:
                    reporter.error(child, "{}s must not contain accessor specifiers", keyword_plural)
            elif clang.cindex.CursorKind.CONVERSION_FUNCTION == child.kind:
                if "class" != self._keyword:
                    reporter.error(child, "{} must not contain conversion functions", keyword_plural)
            elif clang.cindex.CursorKind.ANNOTATE_ATTR == child.kind:
                pass
            else:
                reporter.error(child, "unsupported {} field detected: {}", keyword, child.kind)

    def _add_field(self, node):
        f = Field(node, self.hierarchy, self._reporter)
        if not self._in_union > 0:
            f.attributes.init_arg = True

        if node.access_specifier == clang.cindex.AccessSpecifier.PUBLIC:
            self._fields.append(f)

    def _add_record(self, node, keyword):
        if node.spelling != "":
            self._reporter.error(
                node, "named {}{} are not currently supported", keyword, "es" if keyword.endswith("s") else "s"
            )

        if keyword == "union":
            self._in_union += 1

        for child in node.get_children():
            if clang.cindex.CursorKind.FIELD_DECL == child.kind:
                self._add_field(child)
            elif clang.cindex.CursorKind.UNION_DECL == child.kind:
                self._add_record(child, "union")
            elif clang.cindex.CursorKind.STRUCT_DECL == child.kind:
                self._add_record(child, "struct")
            else:
                self._keyword.error(child, "unsupported {} field detected: {}", keyword, child.kind)

        if keyword == "union":
            self._in_union -= 1

    @property
    def fields(self):
        """Returns a list of fields in the struct/union."""
        return self._fields

    @property
    def name(self):
        """Returns the type name of the struct (with namespaces)."""
        return self.cursor.type.spelling

    @property
    def short_name(self):
        """Returns the type name of the struct (without namespaces)."""
        return self.cursor.spelling

    @property
    def has_union(self):
        """The struct/union contains a union as a field."""
        return self._has_union

    @property
    def is_opaque(self):
        """The struct/union is opaque (no definition)."""
        from . import extract

        return extract.is_opaque_struct(self.cursor.type)


class Field(CursorObject):
    """A field in a struct or union."""

    def __init__(self, cursor, hierarchy, reporter: DiagnosticReporter):
        from . import extract

        super().__init__(cursor, hierarchy, reporter)
        if not extract.is_abi_safe_type(self.hierarchy, self.attributes, cursor.type, reporter):
            raise ValueError(reporter.fatal(cursor, "is not ABI safe type").message)

        self.cursor = cursor
        self._name = extract.get_full_name(cursor)
        self._is_fixed_length_string = False

        # check if the field is a fixed length string and cache the result.  Note that this
        # is done here during init so that any potential errors are detected during parsing.
        # If they are detected later, a temporary file could be left around on disk if the
        # type or attributes are not correct.
        if cursor.type.get_array_size() >= 0:
            type = cursor.type.get_array_element_type()
            if type != None:
                # fixed length arrays are only allowed on C strings.  Make sure the field's type
                # and attributes are appropriate before allowing the field to be created.
                if extract.is_char(type):
                    if not self.attributes.c_str and not self.attributes.no_py:
                        reporter.error(
                            cursor,
                            "a fixed length array on a field may only be of type 'char' and must be tagged as 'OMNI_ATTR(\"c_str\")'.",
                        )
                    self._is_fixed_length_string = True

                elif not self.attributes.no_py:
                    reporter.error(
                        cursor,
                        "a fixed length array is only allowed as a 'c_str' value.  Other types could lead to data truncation and are not safe.  "
                        'Please add OMNI_ATTR("no_py") to ignore this field.',
                    )

    def is_fixed_length_string(self):
        """Checks if this field is a fixed length character array.  This is a special case for struct
        fields.  Fixed size arrays are not (yet) supported in general because python doesn't have
        a concept of a fixed length array or list.  For array types other than char, this could
        result in data being lost due to truncation.  For fixed length strings, truncation is
        acceptable in most cases.

        Note that fixed length array fields are only allowed if they are tagged with the 'c_str'
        attribute.  Fixed length arrays of other types are not allowed because of the potential
        for data loss on truncation.

        Example:
            namespace ns
            {
                struct Foo
                {
                    char OMNI_ATTR("c_str") bar[32]; // this method returns true.
                    char bar2[32]; // this method will cause a build error.
                    int bar3[32]; // this method will cause a build error.
                    std::string bar4; // this method returns false.
                    const char* bar5; // this method returns false.
                    char* bar6; // this method returns false.
                };
            }
        """
        return self._is_fixed_length_string

    @property
    def name(self):
        """Returns the name of the field (with namespaces).

        Example:
            namespace ns
            {
                struct Foo
                {
                    int bar; // this method returns "ns::Foo::bar"
                };
            }

        See also: short_name
        """
        return self._name

    @property
    def short_name(self):
        """ "Returns the name of the field (without namespaces).

        Example:
            namespace ns
            {
                struct Foo
                {
                    int bar; // this method returns "bar"
                };
            }

        See also: name
        """
        return self.cursor.spelling

    @property
    def type_name(self):
        """Returns the type name of the field (with namespaces)."""
        from . import extract

        return extract.get_full_type_name(self.cursor.type)

    @property
    def element_type_name(self):
        """Returns the type name of the field (with namespaces).

        If the type is an array, the type of the array's elements is returned.
        """
        from . import extract

        # handle arrays and arrays of arrays...
        t = self.cursor.type
        n = t
        while n:
            t = n
            if n.get_array_size() > -1:
                n = n.get_array_element_type()
            else:
                n = None

        return extract.get_full_type_name(t)
