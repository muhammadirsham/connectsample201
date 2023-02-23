#!/usr/bin/python3
#
# Copyright (c) 2020-2021, NVIDIA CORPORATION. All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto. Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
import argparse
import ast
import copy
import datetime
import enum
import json
import os
import pathlib
import random
import sys
import textwrap

from omni.repo.format import format_single_cpp


def format_cpp(path, real_file_name, repo_root):
    # we run format twice since clang-format seems to take two tries to stabilize its output
    for i in range(2):
        format_single_cpp(path, real_file_name, repo_root=repo_root)
    return True


# can't figure out how to get pprint to format properly
# import pprint

JSON_NODE_VERSION = 0
BLOB_VERSION = 0
TEXT_WRAP_LENGTH = 66

# jsonref is an optional install, so we may not have it
try:
    import jsonref as jsonbackend
    from jsonref import JsonRefError as JsonError
except:
    import json as jsonbackend
    from json.decoder import JSONDecodeError as JsonError

    print("WARNING: jsonref could not be imported, $ref commands in your schema cannot be resolved")


class InternalError(Exception):
    pass


class UnpackError(Exception):
    pass


class JsonType(enum.Enum):
    """
    The full set of JSON types (excluding null because it's useless).
    """

    BOOL = 0
    INT = 1
    NUMBER = 2
    STRING = 3
    ARRAY = 4
    OBJECT = 5

    # omniverse more-specific
    INT64 = 6  # wider int than is interoperable
    FLOAT32 = 7  # narrower float than is standard
    BINARY = 8  # base64 encoded binary

    # unsigned alternatives to some above types
    UINT32 = 9
    UINT64 = 10

    def fromString(value):
        if not isinstance(value, str):
            raise TypeError("'value' parameter was " + str(type(value)) + " not str")

        options = {
            "boolean": JsonType.BOOL,
            "integer": JsonType.INT,
            "number": JsonType.NUMBER,
            "array": JsonType.ARRAY,
            "object": JsonType.OBJECT,
            "string": JsonType.STRING,
            "int64": JsonType.INT64,
            "float32": JsonType.FLOAT32,
            "binary": JsonType.BINARY,
            "uint32": JsonType.UINT32,
            "uint64": JsonType.UINT64,
        }
        return options[value]

    def toString(self):
        if not isinstance(self, JsonType):
            raise TypeError("'self' parameter was " + str(type(str)) + " not JsonType")

        options = {
            JsonType.BOOL: "boolean",
            JsonType.INT: "integer",
            JsonType.NUMBER: "number",
            JsonType.ARRAY: "array",
            JsonType.OBJECT: "object",
            JsonType.STRING: "string",
            JsonType.INT64: "int64",
            JsonType.FLOAT32: "float32",
            JsonType.BINARY: "binary",
            JsonType.UINT32: "uint32",
            JsonType.UINT64: "uint64",
        }
        return options[self]

    def getJsonNativeType(self):
        if not isinstance(self, JsonType):
            raise TypeError("'self' parameter was " + str(type(str)) + " not JsonType")

        options = {
            JsonType.INT64: JsonType.INT,
            JsonType.FLOAT32: JsonType.NUMBER,
            JsonType.BINARY: JsonType.STRING,
            JsonType.UINT32: JsonType.INT,
            JsonType.UINT64: JsonType.INT,
        }
        if self not in options:
            return self
        return options[self]

    def fromValue(value):
        if isinstance(value, bool):
            return JsonType.BOOL
        elif isinstance(value, int):
            if value >= 2**64 or value < -(2**63):
                return JsonType.NUMBER  # too large for int
            elif value >= 2**63 and value < 2**64:
                return JsonType.UINT64
            elif value >= 2**32 or value < -(2**31):
                return JsonType.INT64
            elif value >= 2**31 and value < 2**32:
                return JsonType.UINT32
            else:
                return JsonType.INT
        elif isinstance(value, float):
            return JsonType.NUMBER
        elif isinstance(value, str):
            return JsonType.STRING
        elif isinstance(value, dict):
            return JsonType.OBJECT
        elif isinstance(value, list):
            return JsonType.ARRAY
        else:
            raise TypeError("'value' type " + str(type(value)) + " does not map to JSON data types.")

    def fromArray(array):
        if not isinstance(array, list):
            raise TypeError("'array' parameter was " + str(type(str)) + " not list")

        array_type = JsonType.fromValue(array[0])

        # go through the array to validate the type
        for i in range(1, len(array)):
            t = JsonType.fromValue(array[i])

            # promote integers to numbers
            if (
                array_type == JsonType.INT
                or array_type == JsonType.INT64
                or array_type == JsonType.UINT32
                or array_type == JsonType.UINT64
            ) and t == JsonType.NUMBER:
                array_type = JsonType.NUMBER

            if array_type == JsonType.NUMBER and (
                t == JsonType.INT or t == JsonType.INT64 or t == JsonType.UINT32 or t == JsonType.UINT64
            ):
                t = JsonType.NUMBER

            # go from signed to unsigned
            if array_type == JsonType.INT and t == JsonType.UINT32:
                array_type = JsonType.UINT32

            if array_type == JsonType.UINT32 and t == JsonType.INT:
                t = JsonType.UINT32

            # increase int width
            if (array_type == JsonType.INT or array_type == JsonType.UINT32) and (
                t == JsonType.INT64 or t == JsonType.UINT64
            ):
                array_type = JsonType.INT64

            if (array_type == JsonType.INT64 or array_type == JsonType.UINT64) and (
                t == JsonType.INT or t == JsonType.UINT32
            ):
                t = JsonType.INT64

            # go from signed to unsigned
            if array_type == JsonType.INT64 and t == JsonType.UINT64:
                array_type = JsonType.UINT64

            if array_type == JsonType.UINT32 and t == JsonType.INT:
                t = JsonType.UINT64

            if t != array_type:
                raise TypeError(
                    "array contains mixed types [0] is " + str(array_type) + "[" + str(i) + "] is " + str(t)
                )

        return array_type

    def get_c_default_value(self):
        """
        Retrieve the C++ default value for a JSON type name string.

        Returns:
            The default value string representing this type.
        """
        if not isinstance(self, JsonType):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonType")

        calls = {
            JsonType.BOOL: "false",
            JsonType.INT: "0",
            JsonType.NUMBER: "0.0",
            JsonType.STRING: "nullptr",
            JsonType.ARRAY: "nullptr",
            JsonType.OBJECT: "{}",
            JsonType.INT64: "0",
            JsonType.FLOAT32: "0.f",
            JsonType.BINARY: "nullptr",
            JsonType.UINT32: "0",
            JsonType.UINT64: "0",
        }
        return calls[self]

    def get_c_type(self):
        """
        Retrieve the C++ primitive type given a JSON type name string.
        This doesn't work for OBJECT or ARRAY.

        Returns:
            The type name string representing type_name.
            An InternalError is raised otherwise.
        """
        if not isinstance(self, JsonType):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonType")

        calls = {
            JsonType.BOOL: "bool",
            JsonType.INT: "int32_t",
            JsonType.NUMBER: "double",
            JsonType.STRING: "const char*",
            JsonType.INT64: "int64_t",
            JsonType.FLOAT32: "float",
            JsonType.BINARY: "const uint8_t*",
            JsonType.UINT64: "uint64_t",
            JsonType.UINT32: "uint32_t",
        }
        try:
            return calls[self]
        except KeyError:
            raise InternalError("unknown type name: '" + str(self) + "'")

    def get_c_const_value(self, const_val):
        """
        Retrieve the value in C code for a constant.

        Args:
            const_val:  The value for the constant.

        Returns
            The C expression for this constant.
        """
        if not isinstance(self, JsonType):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonType")

        if self == JsonType.STRING:
            return '"' + const_val + '"'
        elif self == JsonType.BOOL:
            return str(const_val).lower()
        elif self == JsonType.INT:
            return str(const_val)
        elif self == JsonType.INT64:
            return str(const_val) + "ll"
        elif self == JsonType.NUMBER and isinstance(const_val, int):
            return str(const_val) + ".0"
        elif self == JsonType.NUMBER:
            return str(const_val)
        elif self == JsonType.FLOAT32 and isinstance(const_val, int):
            return str(const_val) + ".f"
        elif self == JsonType.FLOAT32:
            return str(const_val) + "f"
        elif self == JsonType.UINT32:
            return str(const_val) + "u"
        elif self == JsonType.UINT64:
            return str(const_val) + "ull"
        else:
            raise InternalError("invalid type " + str(self))


class JsonEnumValue:
    def __init__(self, value):
        """
        Create an enumeration value from a JSON enumeration.

        Args:
            value the entry in the JSON enumeration array to build into a JsonEnumValue.
        """
        if not isinstance(self, JsonEnumValue):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonEnumValue")

        if (
            not isinstance(value, bool)
            and not isinstance(value, int)
            and not isinstance(value, float)
            and not isinstance(value, str)
        ):
            raise UnpackError("'" + str(value) + "' is not a scalar type")

        if isinstance(value, str):
            if len(value) == 0:
                raise UnpackError("an empty string cannot be part of an enumeration")

            # upper case the first letter to camelCase it
            c_name = value[0].upper() + value[1:]
            py_name = value.upper()
            if not py_name[0:1].isidentifier():
                py_name = "E" + py_name
        else:
            c_name = str(value)
            if isinstance(value, bool):
                py_name = str(value).upper()
            else:
                py_name = "E" + str(value)

        self.c_name = "e" + c_name.replace(".", "_")
        self.py_name = py_name.replace(".", "_")
        if not self.c_name.isidentifier():
            raise UnpackError("'" + self.c_name + "' is not a valid C++ identifier, ignoring this enum")

        self.value = value

    py_name = None
    c_name = None
    value = None


class Parameter:
    """
    A parameter to a binding function.

    This provides the full set of useful information for a property of an event.
    This may be a caller-specified parameter or a constant parameter.
    This may also be an element within an object within a property of an event.
    """

    def __init__(self, name, json_type, array_type=None):
        """
        Create a Parameter.

        Args:
            name The property name from the schema element/object element.
            json_type The JsonType that was pulled from the schema.
            array_type The JsonType for the array elements.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(name, str):
            raise TypeError("'name' parameter type " + str(type(name)) + " was not str")
        if not isinstance(json_type, JsonType):
            raise TypeError("'json_type' parameter type " + str(type(json_type)) + " was not of JsonType")
        if array_type is not None and not isinstance(array_type, JsonType):
            raise TypeError("'array_type' parameter type " + str(type(array_type)) + " was not of JsonType")

        self._const_val = None
        self._description = None
        self._object_name = None
        self._enumeration = None
        self._obj = None

        self._name = name
        self._c_name = name.replace(".", "_").replace(" ", "_").replace("-", "_")
        self._json_type = json_type
        self.array_type = array_type

        if (
            array_type is not None
            and array_type != JsonType.BOOL
            and array_type != JsonType.INT
            and array_type != JsonType.NUMBER
            and array_type != JsonType.STRING
            and array_type != JsonType.INT64
            and array_type != JsonType.FLOAT32
            and array_type != JsonType.OBJECT
            and array_type != JsonType.UINT32
            and array_type != JsonType.UINT64
        ):
            raise UnpackError("array types must be primitive or object, array type was '" + str(self.array_type) + "'")

        if not self._c_name.isidentifier():
            raise UnpackError(
                "event name: '" + self._c_name + "' (original name was: '" + name + "') is not a valid identifier"
            )

    def get_c_param_type(self, use_obj_pointer=False):
        """
        Retrieve the C++ type of a given parameter type.

        Returns:
            A string with the C++ for param.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.enumeration is not None:
            return self.enum_name
        elif self.json_type == JsonType.OBJECT or (
            self.json_type == JsonType.ARRAY and self.array_type == JsonType.OBJECT
        ):
            if use_obj_pointer or (self.json_type == JsonType.ARRAY and self.array_type == JsonType.OBJECT):
                return "const " + self.object_name + "*"
            return "const " + self.object_name + "&"
        elif self.json_type == JsonType.ARRAY:
            return self.array_type.get_c_type() + " const*"
        else:
            return self.json_type.get_c_type()

    def get_python_binding_param_type(self, enum_namespace, use_obj_pointer=False):
        """
        Retrieve the C++ type of a given type in the context of a struct member.
        in a python binding wrapper struct.

        Returns:
            A string with the C++ for param.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.enumeration is not None:
            return enum_namespace + self.enum_name
        elif self.json_type == JsonType.OBJECT:
            if use_obj_pointer:
                return "const " + self.obj.get_wrap_struct_name() + "*"
            return "const " + self.obj.get_wrap_struct_name() + "&"
        elif self.json_type == JsonType.ARRAY:
            if self.array_type == JsonType.STRING:
                return "std::vector<std::string>"
            elif self.array_type == JsonType.OBJECT:
                return "std::vector<" + self.obj.get_wrap_struct_name() + ">"
            else:
                return "std::vector<" + self.array_type.get_c_type() + ">"
        elif self.json_type == JsonType.STRING:
            return "std::string"
        elif self.json_type == JsonType.BINARY:
            return "std::vector<uint8_t>"
        else:
            return self.get_c_param_type()

    def get_python_binding_struct_member_type(self, enum_namespace):
        """
        Retrieve the C++ type of a given type in the context of a struct member.
        in a python binding wrapper struct.

        Returns:
            A string with the C++ for param.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.enumeration is None and self.json_type == JsonType.OBJECT:
            return self.obj.get_wrap_struct_name()
        else:
            return self.get_python_binding_param_type(enum_namespace)

    def get_c_struct_member_type(self):
        """
        Retrieve the C++ type of a given type in the context of a struct member.

        Returns:
            A string with the C++ for param.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.json_type == JsonType.OBJECT:
            return self.object_name
        elif self.json_type == JsonType.STRING:
            return "omni::structuredlog::StringView"
        else:
            return self.get_c_param_type()

    def get_c_array_len_type(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if not self.need_length_parameter():
            raise InternalError("requested array length type declaration of non-array type")
        return "size_t"

    def get_c_const_type(self):
        """
        Retrieve the C++ type of a given type in the context of a constant argument.

        Returns:
            A string with the C++ for param.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.json_type == JsonType.OBJECT:
            return self.object_name
        elif self.json_type == JsonType.ARRAY:
            return self.array_type.get_c_type() + " const"
        elif self.json_type == JsonType.STRING:
            return "const char"
        else:
            return "const " + self.get_c_param_type()

    def _get_c_const_val(self, val):
        """
        Retrieve the value in C code for a constant or enum.

        Args:
            val: either of self.const_val or self.enumeration.

        Returns
            The C expression for this constant.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.json_type == JsonType.ARRAY:
            code = "{ "
            for i in range(len(val)):
                if i > 0:
                    code += ", "
                code += self.array_type.get_c_const_value(val[i])
            code += " }"
            return code
        else:
            return self.json_type.get_c_const_value(val)

    def get_c_const_val(self):
        """
        Retrieve the value in C code for a constant.

        Returns
            The C expression for this constant.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        # due to the way that constant objects are handled here, they use their
        # default initializers to initialize themselves
        if self.json_type == JsonType.OBJECT:
            return "{}"

        if self.const_val is None:
            raise InternalError("this parameter is not constant")

        return self._get_c_const_val(self.const_val)

    def get_c_enumeration(self):
        """
        Retrieve the value in C code for an enumeration to pass into the tree creation.

        Returns
            The C expression for this constant.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.enumeration is None:
            raise InternalError("this parameter is not constant")

        val = []
        for e in self.enumeration:
            val.append(e.value)

        return self._get_c_const_val(val)

    def is_const_obj_array_with_dynamic_len(self):
        """
        Check for the degenerate case where an object has all const members but
        also doesn't have a fixed length

        Returns
            True if the object type is const but this is a dynamic length array.
            False if this is any type.
            False if the object type is non-const.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self.json_type == JsonType.ARRAY and self.array_type == JsonType.OBJECT and self.obj.is_const()

    def get_c_param_documentation(self, for_macro, indent="    "):
        """
        Generate documentation for this parameter.

        Args:
            for_macro: set to `True` if the docs are being generated for a helper macro.
                Set to `False` otherwise.
            indent: The level of indentation to generate this code at.

        Returns:
            The generated code.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        spec = "[in]"
        comment_prefix = indent + " *             "  # spacing to make the comments look nice
        code = ""
        suffix = ""

        if for_macro:
            suffix = "_"

        # array and string parameters have their size passed as an additional parameter
        if self.need_length_parameter():
            # this is a degenerate case where the object is fully specified but it has a dynamic length
            if self.is_const_obj_array_with_dynamic_len():
                code += (
                    indent
                    + " *  @param[in] "
                    + self.get_array_length_name()
                    + " the number of constant objects to submit\n"
                )
                code += comment_prefix + "This must be less than or equal to UINT16_MAX\n"
            else:
                generic_desc = "array"
                code += (
                    indent
                    + " *  @param[in] "
                    + self.get_array_length_name()
                    + suffix
                    + " length of the "
                    + generic_desc
                    + " parameter "
                    + self.c_name
                    + "\n"
                )

        # document the parameter if it not the degenerate case where it's a
        # constant object array without a fixed length
        if not self.is_const_obj_array_with_dynamic_len():
            code += (
                indent
                + " *  @param"
                + spec
                + " "
                + self.c_name
                + suffix
                + " Parameter from schema at path '/"
                + self.name
                + "'.\n"
            )

        # null arrays are legal if length is set to 0
        if (
            self.json_type == JsonType.ARRAY
            and self.enumeration is None
            and not self.is_const_obj_array_with_dynamic_len()
        ):
            code += comment_prefix + "This may be nullptr if " + self.get_array_length_name() + " is 0.\n"

        # string arrays can have nullptr elements
        if self.enumeration is None and self.json_type == JsonType.ARRAY and self.array_type == JsonType.STRING:
            code += comment_prefix + "The strings in this array may individually be nullptr.\n"

        # add the documentation from the schema if it exists
        if self.description is not None:
            lines = textwrap.wrap(self.description, TEXT_WRAP_LENGTH)

            for line in lines:
                code += comment_prefix + line + "\n"

        return code

    def get_array_length_name(self):
        """
        Get the name for the array length of this variable.

        Returns:
            The string name of the array length variable for this parameter.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if not self.need_length_parameter():
            raise InternalError("type is " + str(self.json_type) + " but array length name was requested?")
        return self.c_name + "_len"

    def generate_enum_definition(self, indent=""):
        """
        Generate the definition for enum type that was used for this parameter.

        Args:
            indent: The level of indentation to generate this code at.

        Returns:
            The generated code.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        if self.enumeration is None:
            raise InternalError("this cannot be called on a non-enum parameter")

        code = indent + "/** enumeration for parameter " + self.c_name + " */\n"
        code += indent + "enum class " + self.enum_name + " : uint16_t\n"
        code += indent + "{\n"
        for v in self.enumeration:
            code += indent + "    /** for value " + self.array_type.get_c_const_value(v.value) + " */\n"
            code += indent + "    " + v.c_name + ",\n"
            code += "\n"
        code += indent + "};\n"
        return code

    def generate_py_enum_definition(self, namespace, module, indent=""):
        """
        Generate the definition for an enum type used in python.

        Args:
            indent: The level of indentation to generate this code at.

        Returns:
            The generated code.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        if self.enumeration is None:
            raise InternalError("this cannot be called on a non-enum parameter")

        code = indent + "py::enum_<" + namespace + self.enum_name + ">(" + module + ', "' + self.enum_name + '")'
        for v in self.enumeration:
            code += (
                "\n" + indent + '    .value("' + v.py_name + '", ' + namespace + self.enum_name + "::" + v.c_name + ")"
            )
        code += ";\n"

        return code

    def need_length_parameter(self):
        """
        Check if this parameter needs a separate length parameter

        Returns:
            true if a length parameter is needed.
            false if no length parameter is needed.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return (self.json_type == JsonType.ARRAY and self.enumeration is None) or self.json_type == JsonType.BINARY

    def _python_binding_type_to_native(
        self,
        namespace,
        var_prefix,
        indent="",
        defs_indent="",
        parent_tmp_name=None,
        use_obj_pointer=False,
        dimension=[],
    ):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        defs = ""
        code = ""

        # name of the original var
        name = var_prefix + self.c_name

        # name for the temp variable
        tmp_name = name.replace("->", "_").replace(".", "_").replace("[", "_").replace("]", "_") + "_"

        # name for any iterator
        it_name = "i" + "_" * len(dimension)

        vec_deref = ""
        for i in range(len(dimension)):
            vec_deref += "[i" + "_" * i + "]"

        vec_deref_full = ""
        for i in range(len(dimension) + 1):
            vec_deref_full += "[i" + "_" * i + "]"

        code = ""
        if self.json_type == JsonType.OBJECT or (
            self.json_type == JsonType.ARRAY
            and self.array_type == JsonType.OBJECT
            and not self.is_const_obj_array_with_dynamic_len()
        ):
            new_indent = indent
            new_dimension = copy.deepcopy(dimension)
            if self.json_type == JsonType.OBJECT:
                # python bindings use a special wrapper type, so we need to copy it into the native type
                if parent_tmp_name is None:
                    # we only need to have a tmp object for the top level
                    code += indent + namespace + self.obj.get_struct_name() + " " + tmp_name + " = {};\n"
            else:
                # python bindings use an array of the wrapper types, so we need to create a vector of the native type
                count = name + ".size()"
                new_dimension.append(count)

                defs += defs_indent
                defs += len(dimension) * "std::vector<"
                defs += "std::vector<" + namespace + self.obj.get_struct_name() + ">"
                defs += len(dimension) * ">"
                defs += " " + tmp_name + "(" + new_dimension[0] + ");\n"

                if len(dimension) > 0:
                    code += indent + tmp_name + vec_deref + ".reserve(" + count + ");\n"

                code += (
                    indent
                    + "for (size_t "
                    + it_name
                    + " = 0; "
                    + it_name
                    + " < "
                    + name
                    + ".size(); "
                    + it_name
                    + "++)\n"
                )
                code += indent + "{\n"
                new_indent += "    "

            for p in self.obj.params.items():
                if self.json_type == JsonType.OBJECT:
                    if parent_tmp_name is None:
                        if use_obj_pointer or (self.json_type == JsonType.ARRAY and self.array_type == JsonType.OBJECT):
                            new_prefix = name + "->"
                        else:
                            new_prefix = name + "."
                        new_parent_tmp_name = tmp_name
                    else:
                        new_prefix = name + "."
                        new_parent_tmp_name = parent_tmp_name + "." + self.c_name
                else:
                    new_parent_tmp_name = tmp_name + vec_deref_full
                    if parent_tmp_name is None:
                        new_prefix = name + "[" + it_name + "]."
                    else:
                        new_prefix = name + "[" + it_name + "]."

                (c, d) = p[1]._python_binding_type_to_native(
                    namespace + self.obj.get_struct_name() + "::",
                    new_prefix,
                    new_indent,
                    defs_indent,
                    new_parent_tmp_name,
                    use_obj_pointer,
                    new_dimension,
                )

                code += c
                defs += d

            if self.json_type != JsonType.OBJECT:
                code += indent + "}\n"

                if parent_tmp_name is not None:
                    code += indent + parent_tmp_name + "." + self.c_name + " = " + tmp_name + vec_deref + ".data();\n"
                    code += indent + parent_tmp_name + "." + self.get_array_length_name() + " = " + name + ".size();\n"

        elif self.enumeration is None and self.json_type == JsonType.ARRAY and self.array_type == JsonType.STRING:
            # python bindings use a vector of std::string, which needs to be converted to char**
            defs += defs_indent
            defs += len(dimension) * "std::vector<"
            defs += "std::unique_ptr<const char*[]>"
            defs += len(dimension) * ">"
            defs += " " + tmp_name + "("
            count = name + ".size()"
            if len(dimension) == 0:
                defs += "new const char*[" + count + "]"
            else:
                defs += dimension[0]
                code += indent + tmp_name + vec_deref + ".reset(new const char*[" + count + "]);\n"
            defs += ");\n"

            code += (
                indent + "for (size_t " + it_name + " = 0; " + it_name + " < " + name + ".size(); " + it_name + "++)\n"
            )
            code += indent + "{\n"
            code += indent + "    " + tmp_name + vec_deref_full + " = " + name + "[" + it_name + "].c_str();\n"
            code += indent + "}\n"
            if parent_tmp_name is not None:
                code += indent + parent_tmp_name + "." + self.c_name + " = " + tmp_name + vec_deref + ".get();\n"
                code += indent + parent_tmp_name + "." + self.get_array_length_name() + " = " + name + ".size();\n"
        elif self.enumeration is None and self.json_type == JsonType.ARRAY and self.array_type == JsonType.BOOL:
            # python bindings use a vector<bool>, which has no .data() member for some reason
            defs += defs_indent + "// std::vector<bool> has no .data() member, so we need to manually copy :(\n"
            defs += defs_indent
            defs += len(dimension) * "std::vector<"
            defs += "std::unique_ptr<bool[]>"
            defs += len(dimension) * ">"
            defs += " " + tmp_name + "("
            count = name + ".size()"
            if len(dimension) == 0:
                defs += "new bool[" + count + "]"
            else:
                defs += dimension[0]
                code += indent + tmp_name + vec_deref + ".reset(new bool[" + count + "]);\n"
            defs += ");\n"

            code += (
                indent + "for (size_t " + it_name + " = 0; " + it_name + " < " + name + ".size(); " + it_name + "++)\n"
            )
            code += indent + "{\n"
            code += indent + "    " + tmp_name + vec_deref_full + " = " + name + "[" + it_name + "];\n"
            code += indent + "}\n"
            if parent_tmp_name is not None:
                code += indent + parent_tmp_name + "." + self.c_name + " = " + tmp_name + vec_deref + ".get();\n"
                code += indent + parent_tmp_name + "." + self.get_array_length_name() + " = " + name + ".size();\n"
        elif self.enumeration is None and (self.json_type == JsonType.ARRAY or self.json_type == JsonType.BINARY):
            # not a first level conversion, so it needs to be written back to the parent struct
            if parent_tmp_name is not None:
                code += indent + parent_tmp_name + "." + self.c_name + " = " + name + ".data();\n"
                code += indent + parent_tmp_name + "." + self.get_array_length_name() + " = " + name + ".size();\n"
        else:
            if parent_tmp_name is not None:
                code += indent + parent_tmp_name + "." + self.c_name + " = " + name + ";\n"

        return code, defs

    def python_binding_type_to_native(
        self, namespace, var_prefix, indent="", use_obj_pointer=False, parent_tmp_name=None
    ):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")
        code, defs = self._python_binding_type_to_native(
            namespace, var_prefix, indent, indent, parent_tmp_name, use_obj_pointer
        )
        return defs + code

    def _get_json_type(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self._json_type

    def _get_array_type(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.json_type != JsonType.ARRAY:
            raise InternalError("type is " + str(self.json_type) + " but array type is being requested")
        return self._array_type

    def _set_array_type(self, value):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if value is None:
            if self.json_type == JsonType.ARRAY:
                raise UnpackError("type is 'array' and items/type was not specified to statically type the contents")
        elif not isinstance(value, JsonType):
            raise TypeError("'value' parameter was " + str(type(value)) + " not JsonType")
        self._array_type = value

    def _get_const_val(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self._const_val

    def _validate_type(self, value):
        """
        Raise an exception if the value does not match the parameter's JSON type.

        Args:
            value The value to test the type of.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if (
            (self.json_type == JsonType.BOOL and not isinstance(value, bool))
            or ((self.json_type == JsonType.INT or self.json_type == JsonType.INT) and not isinstance(value, int))
            or (
                (self.json_type == JsonType.NUMBER or self.json_type == JsonType.FLOAT32)
                and not isinstance(value, float)
                and not isinstance(value, int)
            )
            or ((self.json_type == JsonType.STRING or self.json_type == JsonType.BINARY) and not isinstance(value, str))
            or (self.json_type == JsonType.ARRAY and not isinstance(value, list))
        ):
            raise TypeError("'value' type " + str(type(value)) + " does not match JSON type " + str(self.json_type))

    def _set_const_val(self, value):
        """
        Set the constant value that was read from JSON.

        Args:
            value The constant value that was read from JSON.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if (
            not isinstance(value, bool)
            and not isinstance(value, int)
            and not isinstance(value, float)
            and not isinstance(value, str)
            and not isinstance(value, list)
            and not isinstance(value, bool)
        ):
            raise TypeError("unexpected 'const' type " + str(type(value)))

        self._validate_type(value)

        self._const_val = value

    def _get_description(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self._description

    def _set_description(self, value):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(value, str):
            raise TypeError("'value' parameter was " + str(type(value)) + " not a string")

        self._description = value

    def _get_object_name(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.json_type != JsonType.OBJECT and (
            self.json_type != JsonType.ARRAY or self.array_type != JsonType.OBJECT
        ):
            raise InternalError("type is " + str(self.json_type) + " but object name is being requested")
        return "Struct_" + self._c_name

    def _get_enum_name(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        if self.enumeration is None:
            raise InternalError("enum name is being requested on non-enum value")
        return "Enum_" + self._name

    def _get_name(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self._name

    def _get_c_name(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self._c_name

    def _get_enumeration(self):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self._enumeration

    def _set_enumeration(self, values):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(values, list):
            raise TypeError("'values' parameter was " + str(type(values)) + " not a list")

        if self.json_type == JsonType.ARRAY or self.json_type == JsonType.OBJECT:
            raise UnpackError("enums are only supported with scalar types")

        temp = []
        for v in values:
            self._validate_type(v)
            temp.append(JsonEnumValue(v))

        # this becomes an array now
        self._array_type = self._json_type
        self._json_type = JsonType.ARRAY

        if len(temp) == 0:
            raise UnpackError("enumeration is empty")

        self._enumeration = temp

    def _get_obj(self):
        """
        Retrieve the object type of this parameter.

        Returns:
            The object specification, if this is an object or object array type.
            None if this is another type.
        """
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")

        return self._obj

    def _set_obj(self, o):
        if not isinstance(self, Parameter):
            raise TypeError("'self' parameter was " + str(type(self)) + " not Parameter")
        if not isinstance(o, JsonObject):
            raise TypeError("'o' parameter was " + str(type(self)) + " not JsonObject")

        if self.json_type != JsonType.OBJECT and (
            self.json_type != JsonType.ARRAY or self.array_type != JsonType.OBJECT
        ):
            raise InternalError("requested object specification of non-object type")
        self._obj = o

    # The type of the property within the set of JSON types
    # This is of type JsonType
    # This will be the "/type" field of the property in a property
    json_type = property(_get_json_type)

    # The type of the array contents when json_type is JsonType.ARRAY
    # This is of JsonType
    # This will be the "/items/type" field of the property in a property
    array_type = property(_get_array_type, _set_array_type)

    # This is a generic type that can have the constant value written to it.
    # The type will be validated when set; it must match json_type or a TypeError
    # will be thrown
    const_val = property(_get_const_val, _set_const_val)

    # the description parameter of the property.
    # this is just there to provide documentation
    description = property(_get_description, _set_description)

    # the name of this parameter
    name = property(_get_name)

    # the C name of this parameter
    c_name = property(_get_c_name)

    # the name of the object if this is of type JsonType.OBJECT
    object_name = property(_get_object_name)

    # the name of the enum type if this has an enumeration property
    enum_name = property(_get_enum_name)

    # the enumeration values that were specified for this parameter
    # this is a list of JsonEnumValue
    enumeration = property(_get_enumeration, _set_enumeration)

    # the JsonObject layout if this is an OBJECT type
    obj = property(_get_obj, _set_obj)


class JsonObject:
    """
    The representation of an event object that has been pulled from JSON.

    This accumulates the parameters, constants and sub-objects for this specific
    JSON event.
    """

    def __init__(self, eventName, eventPrefix=None, flags=None, desc=None):
        """
        Constructor

        Args:
            eventName   The name of this event. This must be specified.
            eventPrefix The optional schema name to prefix the name with.
                        This is only needed for the top level object which has a
                        separately specified 'eventPrefix' prefix to simplify
                        schema writing and the generated code.
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if eventPrefix is not None and not isinstance(eventPrefix, str):
            raise TypeError("'eventPrefix' parameter was " + str(type(eventPrefix)) + " not a string")
        if not isinstance(eventName, str):
            raise TypeError("'eventName' parameter was " + str(type(eventName)) + " not a string")
        if flags is not None and not isinstance(flags, list):
            raise TypeError("'flags' parameter was " + str(type(flags)) + " not a list or None")

        # name is the full name
        self._name = eventName

        # strip the schema name prefix off of the name
        if eventPrefix is not None:
            if eventName.startswith(eventPrefix + "."):
                eventName = eventName[len(eventPrefix) + 1 :]
            else:
                print(
                    "WARNING: schema property '" + eventName + "' does not start with prefix '" + eventPrefix + "'",
                    file=sys.stderr,
                )

        self._c_name = eventName.replace(".", "_").replace(" ", "_").replace("-", "_")
        self._event_id_name = "k" + self._c_name[0].upper() + self._c_name[1:] + "EventId"
        self._params = {}
        self._consts = {}
        self._objects = {}
        self._desc = desc
        self._flags = flags

        if not self._c_name.isidentifier():
            raise UnpackError(
                "event name: '" + self._c_name + "' (original name was: '" + eventName + "') is not a valid identifier"
            )

    def add_param(self, name, param):
        """
        Add a parameter to this object.

        Args:
            name The name of this parameter (the property name from JSON).
            param The Parameter that was generated from the JSON of this property.

        Returns:
            No return value.
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if not isinstance(name, str):
            raise TypeError("'name' parameter was " + str(type(name)) + " not a string")
        if not isinstance(param, Parameter):
            raise TypeError("'param' parameter was " + str(type(param)) + " not Parameter")

        # object arrays can't be const at the moment
        is_const_struct = param.json_type == JsonType.OBJECT and param.obj.is_const()

        if param.const_val is not None or is_const_struct:
            self._consts[name] = param
        else:
            self._params[name] = param

    def is_const(self):
        """
        check whether a struct has all properties specified with a constant.

        returns:
            true if the struct is constant.
            false if the struct requires parameters specified by the user.
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")

        return len(self.params.items()) == 0

    def generate_struct_code(self, indent, python=False, enum_namespace=""):
        """
        Generate the struct specification for a JSON object.

        Args:
            indent: The level of indentation to prefix code with

        Returns:
            The code for this struct specification
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        if python:
            name = self.get_wrap_struct_name()
        else:
            name = self.get_struct_name()

        enum_namespace += self.get_struct_name() + "::"

        code = indent + "struct " + name + "\n"
        code += indent + "{"

        if not python:
            for p in self.params.items():
                if p[1].enumeration is not None:
                    code += "\n"
                    code += p[1].generate_enum_definition(indent + "    ")

        for p in self.params.items():
            if p[1].obj is not None:
                code += "\n"
                code += indent + "    /** struct definition used for member " + p[1].c_name + ". */\n"
                code += p[1].obj.generate_struct_code(indent + "    ", python, enum_namespace)

        for p in self.params.items():
            code += "\n"
            if p[1].need_length_parameter() and not python:
                code += indent + "    /** length of the array member " + p[1].c_name + ". */\n"
                init = ""
                code += (
                    indent + "    " + p[1].get_c_array_len_type() + " " + p[1].get_array_length_name() + init + ";\n"
                )
            if p[1].description is not None:
                lines = textwrap.wrap(p[1].description, TEXT_WRAP_LENGTH)
                first = lines.pop(0)

                if len(lines) == 0:
                    code += indent + "    /** " + first + " */\n"

                else:
                    code += indent + "    /** " + first + "\n"

                    for line in lines:
                        code += indent + "     *  " + line + "\n"

                    code += indent + "     */\n"

            code += indent + "    "
            if python:
                code += p[1].get_python_binding_struct_member_type(enum_namespace)
            else:
                code += p[1].get_c_struct_member_type()
            code += " " + p[1].c_name + ";\n"

        for p in self.consts.items():
            if p[1].json_type == JsonType.OBJECT:
                # a fully constant object doesn't need to be specified
                continue

            if p[1].description is not None:
                lines = textwrap.wrap(p[1].description, TEXT_WRAP_LENGTH)

                first = lines.pop(0)

                code += "\n"

                if len(lines) == 0:
                    code += indent + "    /** " + first + " */\n"

                else:
                    code += indent + "    /** " + first + "\n"

                    for line in lines:
                        code += indent + "     *  " + line + "\n"

                    code += indent + "     */\n"

            code += indent + "    " + p[1].get_c_const_type() + " " + p[1].c_name
            if p[1].json_type == JsonType.ARRAY or p[1].json_type == JsonType.STRING:
                length = len(p[1].const_val)
                if p[1].json_type == JsonType.STRING:
                    length += 1
                code += "[" + str(length) + "]"
            code += " = " + p[1].get_c_const_val() + ";\n"

        if not python:
            # emit a default constructor if it won't conflict with the other constructor.
            if self.params is not None and len(self.params) > 0:
                code += "\n"
                code += "        /** Default constructor for @ref " + name + ". */\n"
                code += " " + self.get_struct_name() + "() = default;\n"

            # emit a constructor that allows the object to be explicitly constructed.
            code += "\n"
            code += "        /** Basic constructor for @ref " + name + ". */\n"
            code += " " + self.get_struct_name() + "("

            for p in self.params.items():
                if p[1].need_length_parameter():
                    code += p[1].get_c_array_len_type() + " " + p[1].get_array_length_name() + "_, "

                code += p[1].get_c_struct_member_type() + " " + p[1].c_name + "_, "

            if self.params is not None and len(self.params) > 0:
                code = code[0:-2]

            code += ")\n"
            code += "{\n"

            for p in self.params.items():
                if p[1].need_length_parameter():
                    code += p[1].get_array_length_name() + " = " + p[1].get_array_length_name() + "_;\n"

                code += p[1].c_name + " = " + p[1].c_name + "_;\n"

            code += "}\n"

            # emit an assignment operator.
            code += "\n"
            code += "        /** Basic assignment operator for @ref " + name + ". */\n"
            code += self.get_struct_name() + "& operator=(const " + self.get_struct_name() + "& other)\n"
            code += "{\n"

            for p in self.params.items():
                if p[1].need_length_parameter():
                    code += p[1].get_array_length_name() + " = other." + p[1].get_array_length_name() + ";\n"

                code += p[1].c_name + " = other." + p[1].c_name + ";\n"

            code += "    return *this;\n"
            code += "}\n"

            # emit a copy constructor.
            code += "\n"
            code += "        /** Basic copy constructor for @ref " + name + ". */\n"
            code += " " + self.get_struct_name() + "(const " + self.get_struct_name() + "& other)\n"
            code += "{\n"
            code += "    *this = other;\n"
            code += "}\n"

        code += indent + "};\n"

        return code

    def generate_py_struct_code(self, namespace, wrap_namespace, module, indent=""):
        """
        Generate the definition for a struct type used in python.

        Args:
            indent: The level of indentation to generate this code at.

        Returns:
            The generated code.
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")
        name = "bind_" + self.c_name
        wrap_name = self.get_wrap_struct_name()
        code = indent + "{\n"
        code += (
            indent
            + "    py::class_<"
            + wrap_namespace
            + wrap_name
            + "> "
            + name
            + "("
            + module
            + ', "'
            + self.get_struct_name()
            + '");\n'
        )
        code += indent + "    " + name + ".def(py::init<>());\n"  # default constructor
        for p in self.params.items():
            if p[1].enumeration is not None:
                code += p[1].generate_py_enum_definition(
                    namespace + self.get_struct_name() + "::", name, indent + "    "
                )

            # string arrays and object arrays don't work here
            if not p[1].is_const_obj_array_with_dynamic_len():
                if p[1].json_type == JsonType.OBJECT or (
                    p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.OBJECT
                ):
                    code += p[1].obj.generate_py_struct_code(
                        namespace + self.get_struct_name() + "::",
                        wrap_namespace + wrap_name + "::",
                        name,
                        indent + "    ",
                    )
                code += (
                    indent
                    + "    "
                    + name
                    + '.def_readwrite("'
                    + p[1].c_name
                    + '", &'
                    + wrap_namespace
                    + wrap_name
                    + "::"
                    + p[1].c_name
                    + ");\n"
                )

        code += indent + "}\n"
        return code

    def generate_result_check(self, result_var, log_channel_param, msg, payload="", allow_logging=True, indent="    "):
        """
        Generate code to check the return value of a call

        Args:
            result_var:        The name of the variable that holds the bool return value.
                               This can be None to generate no code.
            log_channel_param: The log channel parameter to OMNI_LOG_ERROR (including the comma)
            msg:               The message to put into OMNI_LOG_ERROR.
            payload:           The payload of code to insert when the result is false.
            indent:            The indentation level for this code

        Returns:
            The generated code
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if result_var is not None and not isinstance(result_var, str):
            raise TypeError("'result_var' parameter was " + str(type(result_var)) + " not str or None")
        if log_channel_param is not None and not isinstance(log_channel_param, str):
            raise TypeError("'log_channel_param'parameter was " + str(type(log_channel_param)) + " not str or None")
        if not isinstance(msg, str):
            raise TypeError("'msg' parameter was " + str(type(msg)) + " not str")
        if not isinstance(payload, str):
            raise TypeError("'payload' parameter was " + str(type(payload)) + " not str")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        if result_var is None:
            return ""

        code = indent + "if (!" + result_var + ")\n"
        code += indent + "{\n"
        if allow_logging:
            code += indent + "    OMNI_LOG_ERROR(" + log_channel_param + '"' + msg + '");\n'
        code += payload
        code += indent + "    return nullptr;\n"
        code += indent + "}\n"
        return code

    def generate_json_tree_operation(
        self,
        var_call,
        obj_call,
        obj_array_call,
        name_call,
        base_var=None,
        base_deref=".",
        log_channel_param=None,
        result_var=None,
        allow_logging=True,
        indent="    ",
    ):
        """
        Generate an operation that traverses the JSON tree.

        Args:
            var_call:          The call to perform for a primitive or non-object
                               array type property in the tree.
            obj_call:          The call to perform for an object type property
                               in the tree.
            obj_array_call:    The call to perform for an array of objects
                               property in the tree
            name_call:         The call to perform on each property name in the
                               tree.
            base_var:          The variable name for the base of the tree.
            base_deref:        How to dereference base_var (e.g. "->" or ".").
            log_channel_param: The log channel parameter to OMNI_LOG_ERROR
                               (including the comma)
            result_var:        The bool variable to store the results of calls in.
            indent:            The indentation level for this code

        Returns:
            The generated code
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if not isinstance(var_call, str):
            raise TypeError("'var_call' parameter was " + str(type(var_call)) + " not str")
        if not isinstance(obj_call, str):
            raise TypeError("'obj_call' parameter was " + str(type(obj_call)) + " not str")
        if not isinstance(obj_array_call, str):
            raise TypeError("'obj_array_call' parameter was " + str(type(obj_array_call)) + " not str")
        if not isinstance(name_call, str):
            raise TypeError("'name_call' parameter was " + str(type(name_call)) + " not str")
        if base_var is not None and not isinstance(base_var, str):
            raise TypeError("'base_var' parameter was " + str(type(base_var)) + " not str or None")
        if base_deref is not None and not isinstance(base_deref, str):
            raise TypeError("'base_deref' parameter was " + str(type(base_deref)) + " not str or None")
        if log_channel_param is not None and not isinstance(log_channel_param, str):
            raise TypeError("'log_channel_param' parameter was " + str(type(log_channel_param)) + " not str or None")
        if result_var is not None and not isinstance(result_var, str):
            raise TypeError("'result_var' parameter was " + str(type(result_var)) + " not str or None")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        first = True
        i = 0

        if log_channel_param is not None:
            result_assign = result_var + " = "
        else:
            result_assign = ""

        code = indent + "{\n"
        for q in [self.params.items(), self.consts.items()]:
            for p in q:
                if not first:
                    code += "\n"
                first = False

                # choose the first param for the set calls, if applicable
                node = None
                node_param = ""

                if base_var is not None:
                    node = base_var + base_deref + "data.objVal[" + str(i) + "]"
                    node_param = "&" + node + ", "

                # set the node's name
                code += indent + "    // property " + p[1].name + "\n"
                code += indent + "    " + result_assign + name_call + "(" + node_param + '"' + p[1].name + '");\n'
                code += self.generate_result_check(
                    result_var,
                    log_channel_param,
                    "failed to set the object name (bad size calculation?)",
                    "",
                    allow_logging,
                    indent + "    ",
                )

                if p[1].json_type == JsonType.OBJECT:
                    prop_count = len(p[1].obj.params.items()) + len(p[1].obj.consts.items())
                    code += (
                        indent
                        + "    "
                        + result_assign
                        + obj_call
                        + "("
                        + node_param
                        + str(prop_count)
                        + "); // object has "
                        + str(prop_count)
                        + " properties\n"
                    )
                    code += self.generate_result_check(
                        result_var,
                        log_channel_param,
                        "failed to create a new object node (bad size calculation?)",
                        "",
                        allow_logging,
                        indent + "    ",
                    )
                    code += p[1].obj.generate_json_tree_operation(
                        var_call,
                        obj_call,
                        obj_array_call,
                        name_call,
                        node,
                        ".",
                        log_channel_param,
                        result_var,
                        allow_logging,
                        indent + "    ",
                    )
                elif p[1].json_type == JsonType.ARRAY:
                    if p[1].array_type == JsonType.OBJECT:
                        if p[1].const_val is not None:
                            raise InternalError("const object arrays should not be possible yet")
                        elif p[1].enumeration is not None:
                            raise InternalError("enumerations of objects should not be possible yet")

                        prop_count = len(p[1].obj.params.items()) + len(p[1].obj.consts.items())
                        code += (
                            indent
                            + "    "
                            + result_assign
                            + obj_array_call
                            + "("
                            + node_param
                            + str(prop_count)
                            + ", 1); // subobjects have "
                            + str(prop_count)
                            + " properties\n"
                        )
                        code += self.generate_result_check(
                            result_var,
                            log_channel_param,
                            "failed to create a new object array node (bad size calculation?)",
                            "",
                            allow_logging,
                            indent + "    ",
                        )
                        subnode = None
                        if node is not None:
                            subnode = node + ".data.objVal[0]"
                        code += p[1].obj.generate_json_tree_operation(
                            var_call,
                            obj_call,
                            obj_array_call,
                            name_call,
                            subnode,
                            ".",
                            log_channel_param,
                            result_var,
                            allow_logging,
                            indent + "    ",
                        )
                    elif p[1].const_val is None and p[1].enumeration is None:
                        code += (
                            indent
                            + "    "
                            + result_assign
                            + var_call
                            + "("
                            + node_param
                            + "static_cast<"
                            + p[1].get_c_param_type()
                            + ">(nullptr), 0);\n"
                        )
                        code += self.generate_result_check(
                            result_var,
                            log_channel_param,
                            "failed to set type '" + p[1].get_c_param_type() + "' (shouldn't be possible)",
                            "",
                            allow_logging,
                            indent + "    ",
                        )
                    else:
                        tmp_var = "a" + indent.replace("    ", "_")
                        if p[1].enumeration is None:
                            value = p[1].get_c_const_val()
                            length = len(p[1].const_val)
                            comment = ""
                        else:
                            value = p[1].get_c_enumeration()
                            length = len(p[1].enumeration)
                            comment = "// " + p[1].enum_name + " maps onto this array"

                        code += indent + "    {\n"
                        if comment != "":
                            code += indent + "        " + comment + "\n"
                        code += (
                            indent
                            + "        static "
                            + p[1].get_c_const_type()
                            + " "
                            + tmp_var
                            + "[] = "
                            + value
                            + ";\n"
                        )
                        code += (
                            indent
                            + "        "
                            + result_assign
                            + var_call
                            + "("
                            + node_param
                            + tmp_var
                            + ", uint16_t(CARB_COUNTOF("
                            + tmp_var
                            + ")));\n"
                        )
                        code += self.generate_result_check(
                            result_var,
                            log_channel_param,
                            "failed to set an array of length " + str(length) + " (bad size calculation?)",
                            "",
                            allow_logging,
                            indent + "        ",
                        )
                        code += indent + "    }\n"

                elif p[1].json_type == JsonType.STRING:
                    if p[1].const_val is not None:
                        code += (
                            indent
                            + "    "
                            + result_assign
                            + var_call
                            + "("
                            + node_param
                            + p[1].get_c_const_val()
                            + ", sizeof("
                            + p[1].get_c_const_val()
                            + "));\n"
                        )
                        code += self.generate_result_check(
                            result_var,
                            log_channel_param,
                            "failed to copy string '" + p[1].const_val + "' (bad size calculation?)",
                            "",
                            allow_logging,
                            indent + "    ",
                        )
                    else:
                        code += (
                            indent
                            + "    "
                            + result_assign
                            + var_call
                            + "("
                            + node_param
                            + "static_cast<const char*>(nullptr));\n"
                        )
                        code += self.generate_result_check(
                            result_var,
                            log_channel_param,
                            "failed to set type '" + p[1].get_c_param_type() + "' (shouldn't be possible)",
                            "",
                            allow_logging,
                            indent + "    ",
                        )
                elif p[1].json_type == JsonType.BINARY:
                    code += (
                        indent
                        + "    "
                        + result_assign
                        + var_call
                        + "("
                        + node_param
                        + "static_cast<const uint8_t*>(nullptr), 0);\n"
                    )
                    code += self.generate_result_check(
                        result_var,
                        log_channel_param,
                        "failed to set type '" + p[1].get_c_param_type() + "' (shouldn't be possible)",
                        "",
                        allow_logging,
                        indent + "    ",
                    )
                else:
                    if p[1].const_val is not None:
                        code += (
                            indent
                            + "    "
                            + result_assign
                            + var_call
                            + "("
                            + node_param
                            + p[1].get_c_param_type()
                            + "("
                            + p[1].get_c_const_val()
                            + "));\n"
                        )
                    else:
                        code += (
                            indent
                            + "    "
                            + result_assign
                            + var_call
                            + "("
                            + node_param
                            + p[1].get_c_param_type()
                            + "("
                            + p[1].json_type.get_c_default_value()
                            + "));\n"
                        )
                    code += self.generate_result_check(
                        result_var,
                        log_channel_param,
                        "failed to set type '" + p[1].get_c_param_type() + "' (shouldn't be possible)",
                        "",
                        allow_logging,
                        indent + "    ",
                    )

                # set the flags, if needed
                if base_var is not None:
                    flag = None
                    if p[1].const_val is not None:
                        flag = "omni::structuredlog::JsonNode::fFlagConst"
                    elif p[1].enumeration is not None:
                        flag = "omni::structuredlog::JsonNode::fFlagEnum"
                    if flag is not None:
                        code += (
                            indent
                            + "    "
                            + result_assign
                            + "omni::structuredlog::JsonBuilder::setFlags(\n"
                            + indent
                            + "        "
                            + node_param
                            + flag
                            + ");\n"
                        )
                        code += self.generate_result_check(
                            result_var,
                            log_channel_param,
                            "failed to set flag '" + flag + "'",
                            "",
                            allow_logging,
                            indent + "    ",
                        )

                i += 1

        code += indent + "}\n"
        return code

    def generate_tree_create_functions(self, log_channel_param, result_var, allow_logging, indent="    "):
        """
        Generate the function to calculate the schema tree size and to build the tree.

        Args:
            log_channel_param: The log channel parameter to OMNI_LOG_ERROR
                               (including the comma)
            result_var:        The bool variable to store the results of calls in.
            indent:            The indentation level for this code

        Returns:
            The generated code
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if log_channel_param is not None and not isinstance(log_channel_param, str):
            raise TypeError("'log_channel_param' parameter was " + str(type(log_channel_param)) + " not str or None")
        if result_var is not None and not isinstance(result_var, str):
            raise TypeError("'result_var' parameter was " + str(type(result_var)) + " not str or None")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        code = indent + "/** Calculate JSON tree size for structured log event: " + self.name + ".\n"
        code += indent + " *  @returns The JSON tree size in bytes for this event.\n"
        code += indent + " */\n"
        code += indent + "static size_t " + self.calc_size_fn_name + "()\n"
        code += indent + "{\n"
        # in theory, we could hard-code the size, but it's easier to leave the
        # size calculation up to this helper class
        code += indent + "    // calculate the buffer size for the tree\n"
        code += indent + "    omni::structuredlog::JsonTreeSizeCalculator calc;\n"
        code += indent + "    calc.trackRoot();\n"
        prop_count = len(self.params.items()) + len(self.consts.items())
        code += (
            indent + "    calc.trackObject(" + str(prop_count) + "); // object has " + str(prop_count) + " properties\n"
        )
        code += self.generate_json_tree_operation(
            "calc.track",
            "calc.trackObject",
            "calc.trackObjectArray",
            "calc.trackName",
            None,
            None,
            None,
            None,
            allow_logging,
            indent + "    ",
        )
        code += indent + "    return calc.getSize();\n"
        code += indent + "}\n"
        code += "\n"

        code += indent + "/** Generate the JSON tree for structured log event: " + self.name + ".\n"
        code += indent + " *  @param[in]    bufferSize The length of @p buffer in bytes.\n"
        code += indent + " *  @param[inout] buffer     The buffer to write the tree into.\n"
        code += indent + " *  @returns The JSON tree for this event.\n"
        code += indent + " *  @returns nullptr if a logic error occurred or @p bufferSize was too small.\n"
        code += indent + " */\n"
        code += (
            indent
            + "static omni::structuredlog::JsonNode* "
            + self.build_tree_fn_name
            + "(size_t bufferSize, uint8_t* buffer)\n"
        )
        code += indent + "{\n"
        code += indent + "    CARB_MAYBE_UNUSED bool result;\n"
        code += indent + "    omni::structuredlog::BlockAllocator alloc(buffer, bufferSize);\n"
        code += indent + "    omni::structuredlog::JsonBuilder builder(&alloc);\n"
        code += (
            indent
            + "    omni::structuredlog::JsonNode* base = static_cast<omni::structuredlog::JsonNode*>(alloc.alloc(sizeof(*base)));\n"
        )
        code += indent + "    if (base == nullptr)\n"
        code += indent + "    {\n"
        if allow_logging:
            code += (
                indent
                + "        OMNI_LOG_ERROR("
                + log_channel_param
                + "\n"
                + indent
                + '            "failed to allocate the base node for event "\n'
                + indent
                + "            \"'"
                + self.name
                + "' \"\n"
                + indent
                + '            "{alloc size = %zu, buffer size = %zu}",\n'
                + indent
                + "            sizeof(*base), bufferSize);\n"
            )
        code += indent + "        return nullptr;\n"
        code += indent + "    }\n"
        code += indent + "    *base = {};\n"
        code += "\n"
        code += indent + "    // build the tree\n"

        prop_count = len(self.params.items()) + len(self.consts.items())
        code += (
            indent
            + "    result = builder.createObject(base, "
            + str(prop_count)
            + "); // object has "
            + str(prop_count)
            + " properties\n"
        )
        code += self.generate_result_check(
            result_var,
            log_channel_param,
            "failed to create an object node (bad size calculation?)",
            "",
            allow_logging,
            indent + "    ",
        )

        code += self.generate_json_tree_operation(
            "builder.setNode",
            "builder.createObject",
            "builder.createObjectArray",
            "builder.setName",
            "base",
            "->",
            log_channel_param,
            result_var,
            allow_logging,
            indent + "    ",
        )
        code += "\n"
        code += indent + "    return base;\n"
        code += indent + "}\n"

        return code

    def generate_blob_operation(
        self,
        copy_call,
        deref=".",
        result_var=None,
        log_channel_param="",
        allow_logging=True,
        indent="    ",
        name_prefix="",
        depth=0,
    ):
        """
        Generate an operation that traverses the binary blob elements.

        Args:
            copy_call:         The call to perform on each data element.
            result_var:        The bool variable to store the results of calls in.
            log_channel_param: The log channel parameter to OMNI_LOG_ERROR
                               (including the comma)
            indent:            The indentation level for this code
            name_prefix:       The prefix to each property name (e.g. "obj.").

        Returns:
            The generated code
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if not isinstance(copy_call, str):
            raise TypeError("'copy_call' parameter was " + str(type(copy_call)) + " not str")
        if not isinstance(deref, str):
            raise TypeError("'deref' parameter was " + str(type(deref)) + " not str")
        if result_var is not None and not isinstance(result_var, str):
            raise TypeError("'result_var' parameter was " + str(type(result_var)) + " not str or None")
        if log_channel_param is not None and not isinstance(log_channel_param, str):
            raise TypeError("'log_cahnnel_param' parameter was " + str(type(log_channel_param)) + " not str or None")
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")
        if not isinstance(name_prefix, str):
            raise TypeError("'name_prefix' parameter was " + str(type(name_prefix)) + " not str")

        first = True
        i = 0

        code = indent + "{\n"
        for p in self.params.items():
            name = name_prefix + p[1].c_name

            # enums need to have an explicit type to make the templates work
            if p[1].enumeration is not None:
                name = "uint16_t(" + name + ")"

            if p[1].need_length_parameter():
                array_len_name = name_prefix + p[1].get_array_length_name()

            if not first:
                code += "\n"
            first = False

            if result_var is None:
                if (p[1].need_length_parameter() or p[1].json_type == JsonType.STRING) and allow_logging:
                    if p[1].json_type == JsonType.STRING:
                        code += indent + "    if (kValidateLength && " + name + ".length() + 1 > UINT16_MAX)\n"
                    else:
                        code += indent + "    if (kValidateLength && " + array_len_name + " > UINT16_MAX)\n"
                    code += indent + "    {\n"
                    code += (
                        indent
                        + "        OMNI_LOG_ERROR("
                        + log_channel_param
                        + "\n"
                        + indent
                        + "            "
                        + "\"length of parameter '"
                        + name
                        + "' exceeds max value "
                        + str(2**16 - 1)
                        + ' - "\n'
                        + indent
                        + '            "'
                        + 'it will be truncated (size was %zu)",\n'
                    )
                    code += indent + "            "
                    if p[1].json_type == JsonType.STRING:
                        code += name + ".length() + 1);\n"
                    else:
                        code += array_len_name + ");\n"

                    code += indent + "    }\n"
                    code += "\n"

            code += indent + "    // property " + name + "\n"
            if p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.OBJECT:
                code += indent + "    " + copy_call + "(uint16_t(CARB_MIN(" + array_len_name + ", UINT16_MAX)));\n"
                if len(p[1].obj.params.items()) > 0:
                    it_name = "i" + "_" * depth
                    code += (
                        indent
                        + "    for (size_t "
                        + it_name
                        + " = 0; "
                        + it_name
                        + " < CARB_MIN("
                        + array_len_name
                        + ", UINT16_MAX); "
                        + it_name
                        + "++)\n"
                    )
                    code += p[1].obj.generate_blob_operation(
                        copy_call,
                        ".",
                        result_var,
                        log_channel_param,
                        allow_logging,
                        indent + "    ",
                        name + "[" + it_name + "].",
                        depth + 1,
                    )
            elif p[1].json_type == JsonType.OBJECT:
                code += p[1].obj.generate_blob_operation(
                    copy_call,
                    ".",
                    result_var,
                    log_channel_param,
                    allow_logging,
                    indent + "    ",
                    name + deref,
                    depth + 1,
                )
            else:
                if p[1].need_length_parameter():
                    code += (
                        indent
                        + "    "
                        + copy_call
                        + "("
                        + name
                        + ", uint16_t(CARB_MIN("
                        + array_len_name
                        + ", UINT16_MAX)));\n"
                    )
                else:
                    code += indent + "    " + copy_call + "(" + name + ");\n"

            i += 1

        code += indent + "}\n"
        return code

    def generate_send_docs(self, for_macro, class_name="", indent="    "):
        if for_macro:
            code = "/** helper macro to send the '" + self.c_name + "' event.\n"

        else:
            code = indent + "/** Send the event '" + self.name + "'\n"

        code += indent + " *\n"

        if not for_macro:
            code += indent + " *  @param[in] strucLog The global structured log object to use to send\n"
            code += indent + " *             this event.  This must not be nullptr.  It is the caller's\n"
            code += indent + " *             responsibility to ensure a valid object is passed in.\n"

        if self.flags is not None and "fEventFlagExplicitFlags" in self.flags:
            suffix = ""
            if for_macro:
                suffix = "_"

            code += (
                indent + " *  @param[in] eventFlags" + suffix + " The flags to pass directly to the event handling\n"
            )
            code += indent + " *             function.  This may be any of the AllocFlags flags that can\n"
            code += indent + " *             be passed to @ref omni::structuredlog::IStructuredLog::allocEvent().\n"

        for p in self.params.items():
            code += p[1].get_c_param_documentation(for_macro, indent)

        code += indent + " *  @returns no return value.\n"
        code += indent + " *\n"

        if self.description is not None:
            lines = textwrap.wrap(self.description, TEXT_WRAP_LENGTH)

            first = lines.pop(0)
            code += indent + " *  @remarks " + first + "\n"

            for line in lines:
                code += indent + " *           " + line + "\n"

        if for_macro:
            code += indent + " *\n"
            code += indent + " *  @sa @ref " + class_name + "::" + self.send_fn_name + "().\n"
            code += indent + " *  @sa @ref " + class_name + "::" + self.enabled_fn_name + "().\n"

        code += indent + " */\n"
        return code

    def generate_send_macros(self, namespace, class_name, client_name, version):
        code = self.generate_send_docs(True, class_name, "")
        macro = "#define OMNI_"

        macro += client_name.upper() + "_" + version + "_" + self.c_name.upper() + "("

        first = True
        has_event_flags = False
        if self.flags is not None and "fEventFlagExplicitFlags" in self.flags:
            has_event_flags = True

        if has_event_flags:
            first = False
            macro += "eventFlags_"

        for p in self.params.items():
            if not first:
                macro += ", "
            first = False

            if p[1].need_length_parameter():
                macro += p[1].get_array_length_name() + "_"

            if not p[1].is_const_obj_array_with_dynamic_len():
                if p[1].need_length_parameter():
                    macro += ", "
                macro += p[1].c_name + "_"

        macro += ") "
        macro += "\\".rjust(120 - len(macro)) + "\n"
        code += macro
        code += "    OMNI_STRUCTURED_LOG(" + namespace + class_name + "::" + self.c_name

        if has_event_flags:
            code += ", eventFlags_"

        for p in self.params.items():
            code += ", "

            if p[1].need_length_parameter():
                code += p[1].get_array_length_name() + "_"

            if not p[1].is_const_obj_array_with_dynamic_len():
                if p[1].need_length_parameter():
                    code += ", "
                code += p[1].c_name + "_"

        code += ")\n"

        return code

    def generate_send_function(
        self, log_channel_param, result_var, is_private, allow_logging, schema_flags, indent="    "
    ):
        """
        Generate the call to send a binary blob for a structured log event.

        Args:
            log_channel_param: The log channel parameter to OMNI_LOG_ERROR
                               (including the comma)
            result_var:        The bool variable to store the results of calls in.
            indent:            The indentation level for this code

        Returns:
            The generated code
        """
        if not isinstance(self, JsonObject):
            raise TypeError("'self' parameter was " + str(type(self)) + " not JsonObject")
        if log_channel_param is not None and not isinstance(log_channel_param, str):
            raise TypeError("'log_channel_param' parameter was " + str(type(log_channel_param)) + " not str or None")
        if result_var is not None and not isinstance(result_var, str):
            raise TypeError("'result_var' parameter was " + str(type(result_var)) + " not str or None")
        if schema_flags is not None and not isinstance(schema_flags, list):
            raise TypeError(
                "'schema_flags' parameter was " + str(type(schema_flags)) + " not a list of strings or None"
            )
        if not isinstance(indent, str):
            raise TypeError("'indent' parameter was " + str(type(indent)) + " not str")

        if is_private:
            code = indent + "/** body for the " + self.send_fn_name + "() function. */\n"
        else:
            code = self.generate_send_docs(False)

        event_flags_name = "eventFlags"
        has_explicit_flags = False
        if self.flags is not None and "fEventFlagExplicitFlags" in self.flags:
            has_explicit_flags = True

        code += indent + "static void "
        if is_private:
            code += "_"

        code += self.send_fn_name + "(omni::structuredlog::IStructuredLog* strucLog"

        if has_explicit_flags:
            code += ", AllocFlags " + event_flags_name

        use_obj_pointer = False
        member_separator = "."
        if (schema_flags is not None and "fSchemaFlagUseObjectPointer" in schema_flags) or (
            self.flags is not None and "fEventFlagUseObjectPointer" in self.flags
        ):
            use_obj_pointer = True
            member_separator = "->"

        for p in self.params.items():
            code += ", "

            if p[1].need_length_parameter():
                code += p[1].get_c_array_len_type() + " " + p[1].get_array_length_name()

            if not p[1].is_const_obj_array_with_dynamic_len():
                if p[1].need_length_parameter():
                    code += ", "
                if p[1].json_type == JsonType.STRING:
                    if not p[1].is_const_obj_array_with_dynamic_len():
                        code += "const omni::structuredlog::StringView& " + p[1].c_name
                else:
                    code += p[1].get_c_param_type(use_obj_pointer) + " " + p[1].c_name

        code += ") noexcept\n"
        code += indent + "{\n"

        if not is_private:
            code += indent + "    _" + self.send_fn_name + "(strucLog"

            if has_explicit_flags:
                code += ", " + event_flags_name

            for p in self.params.items():
                code += ", "

                if p[1].need_length_parameter():
                    code += p[1].get_array_length_name()
                elif p[1].json_type == JsonType.STRING and is_private:
                    code += p[1].c_name + ".size()"

                if not p[1].is_const_obj_array_with_dynamic_len():
                    if p[1].need_length_parameter() or (p[1].json_type == JsonType.STRING and is_private):
                        code += ", "
                    code += p[1].c_name
                    if p[1].json_type == JsonType.STRING and is_private:
                        code += ".data()"

            code += ");\n"
            code += indent + "}\n"
            return code

        code += indent + "    omni::structuredlog::AllocHandle handle = {};\n"
        code += "\n"

        event_flags = "0"
        if has_explicit_flags:
            event_flags = event_flags_name

        if len(self.params.items()) > 0:
            code += indent + "    // calculate the required buffer size for the event\n"
            code += indent + "    omni::structuredlog::BinaryBlobSizeCalculator calc;\n"
            code += self.generate_blob_operation(
                "calc.track", member_separator, None, log_channel_param, allow_logging, indent + "    "
            )
            code += "\n"
            code += indent + "    // write out the event into the buffer\n"
            code += (
                indent
                + "    void* buffer = strucLog->allocEvent("
                + str(BLOB_VERSION)
                + ", "
                + self.event_id_name
                + ", "
                + event_flags
                + ", calc.getSize(), &handle);\n"
            )
            code += indent + "    if (buffer == nullptr)\n"
            code += indent + "    {\n"
            if allow_logging:
                code += (
                    indent
                    + "        OMNI_LOG_ERROR("
                    + log_channel_param
                    + "\n"
                    + indent
                    + '            "failed to allocate a %zu byte buffer for structured log event "\n'
                    + indent
                    + "            \"'"
                    + self.name
                    + "'\",\n"
                    + indent
                    + "            calc.getSize());\n"
                )
            code += indent + "        return;\n"
            code += indent + "    }\n"
            code += "\n"
            code += (
                indent
                + "    omni::structuredlog::BlobWriter<CARB_DEBUG, _onStructuredLogValidationError> writer(buffer, calc.getSize());\n"
            )
            code += self.generate_blob_operation(
                "writer.copy", member_separator, result_var, log_channel_param, allow_logging, indent + "    "
            )
            code += "\n"
        else:
            code += indent + "    // write out the event into the buffer\n"
            code += (
                indent
                + "    if (strucLog->allocEvent("
                + str(BLOB_VERSION)
                + ", "
                + self.event_id_name
                + ", "
                + event_flags
                + ", 0, &handle) == nullptr)\n"
            )
            code += indent + "    {\n"
            if allow_logging:
                code += (
                    indent
                    + "        OMNI_LOG_ERROR("
                    + log_channel_param
                    + '"failed to allocate a buffer for structured log event '
                    + self.name
                    + '");\n'
                )
            code += indent + "        return;\n"
            code += indent + "    }\n"

        code += indent + "    strucLog->commitEvent(handle);\n"
        code += indent + "}\n"
        return code

    def generate_py_send_function(self, schema_flags=None, indent=""):
        code = indent + "def " + self.c_name + "_send_event("

        first = True
        for p in self.params.items():
            if not first:
                code += ", "

            code += p[1].c_name
            first = False

        code += "):\n"

        code += indent + '    """\n'
        code += indent + "    Helper function to send the " + self.name + " event.\n"
        lines = textwrap.wrap(self.description, TEXT_WRAP_LENGTH)
        for line in lines:
            code += indent + "    " + line + "\n"

        if len(self.params) > 0:
            code += indent + "    Args:\n"
        for p in self.params.items():
            space = indent + "        " + (" " * len(p[1].c_name)) + "  "
            lines = textwrap.wrap(p[1].description, TEXT_WRAP_LENGTH)

            code += indent + "        " + p[1].c_name + ": " + lines[0] + "\n"
            for i in range(1, len(lines)):
                code += space + lines[i] + "\n"

            if p[1].obj is not None:
                code += space + "This structure must be passed as a dict.\n"
                code += space + "A dict with incorrect structure or types will not be sent.\n"

        code += indent + "    Returns: no return value.\n"

        code += indent + '    """\n'

        code += indent + "    if events is None:\n"
        code += indent + "        return\n"
        code += "\n"
        code += indent + "    try:\n"
        code += indent + '        omni.structuredlog.send_event(events["' + self.c_name + '"], {'

        first = True
        for p in self.params.items():
            if not first:
                code += ", "

            code += '"' + p[1].c_name + '": ' + p[1].c_name
            first = False

        code += indent + "})\n"
        code += indent + "    except Exception as e:\n"
        code += (
            indent
            + '        omni.log.error("failed to send telemetry event '
            + self.c_name
            + ' " + str(type(e)) + " " + str(e))\n'
        )

        # black adds two newlines
        code += "\n\n"

        return code

    def get_struct_name(self):
        return "Struct_" + self.c_name

    def get_wrap_struct_name(self):
        return "Struct_Wrap_" + self.c_name

    def _get_params(self):
        return self._params

    def _get_consts(self):
        return self._consts

    def _get_name(self):
        return self._name

    def _get_c_name(self):
        return self._c_name

    def _get_event_id_name(self):
        return self._event_id_name

    def _get_build_tree_name(self):
        return "_" + self.c_name + "_buildJsonTree"

    def _get_calc_size_fn_name(self):
        return "_" + self.c_name + "_calculateTreeSize"

    def _get_send_fn_name(self):
        return self.c_name + "_sendEvent"

    def _get_enabled_fn_name(self):
        return self.c_name + "_isEnabled"

    def _get_flags(self):
        return self._flags

    def _get_description(self):
        return self._desc

    # The dictionary of dynamic parameters belonging to this event.
    params = property(_get_params)

    # The dictionary of constant parameters belonging to this event.
    consts = property(_get_consts)

    # The event's name as pulled from the schema.
    name = property(_get_name)

    # The name for this event that can be emitted in C code.
    c_name = property(_get_c_name)

    # The name for this event's ID enum value in C code.
    event_id_name = property(_get_event_id_name)

    # The name of the function used to build the JsonNode tree.
    build_tree_fn_name = property(_get_build_tree_name)

    # The name of the function used to calculate the size of the JsonNode tree.
    calc_size_fn_name = property(_get_calc_size_fn_name)

    # The name of the function used to send a structured log event.
    send_fn_name = property(_get_send_fn_name)

    # The name of the function used to check if an event is enabled.
    enabled_fn_name = property(_get_enabled_fn_name)

    # description tag from the schema
    description = property(_get_description)

    # the EventFlags specified in the schema
    flags = property(_get_flags)

    _name = None
    _c_name = None
    _event_id_name = None
    _params = None
    _consts = None


def validate_type(param_type, ex_type, dbg_path):
    if param_type is not None and not isinstance(param_type, str) and not isinstance(param_type, list):
        raise TypeError("'param_type' parameter was " + str(type(param_type)) + " not str or list")
    if ex_type is not None and not isinstance(ex_type, str):
        raise TypeError("'ex_type' parameter was " + str(type(ex_type)) + " not str")
    if not isinstance(dbg_path, str):
        raise TypeError("'dbg_path' parameter was " + str(type(dbg_path)) + " not str")

    # param_type could legitimately be None if a 'const' tag was specified
    if param_type is None:
        if ex_type is not None:
            raise UnpackError("'" + dbg_path + "/type' is required when 'omniverseFormat' is defined")
        return

    if param_type not in ["boolean", "integer", "number", "string", "array", "object"]:
        raise UnpackError(dbg_path + "/type '" + param_type + "' is not a valid JSON data type")

    if ex_type is None:
        return

    options = {
        "float32": ["number"],
        "binary": ["string"],
        "uint32": ["integer", "number"],
        "int64": ["integer", "number"],
        "uint64": ["integer", "number"],
    }
    notes = {"float32": "", "binary": "", "uint32": "", "int64": "", "uint64": ""}
    try:
        # sort to make the comparison easier
        if isinstance(param_type, list):
            param_type.sort()

        err = "{"
        opt = options[ex_type]
        valid = False
        for o in opt:
            if param_type == o:
                valid = True
                break
            err += str(o) + ","
        if not valid:
            raise UnpackError(
                "'"
                + dbg_path
                + "/omniverseFormat': '"
                + ex_type
                + "' does not match type tag '"
                + str(param_type)
                + "', valid values are "
                + err
                + "}"
                + notes[ex_type]
            )
    except KeyError:
        raise UnpackError("'" + dbg_path + "/omniverseFormat' has unknown value '" + ex_type + "'")


def unpack_property(prop, name, dbg_path, unpacked):
    """
    Unpack a property element of an event into one of the property lists.

    Args:
        prop The property element to unpack
        name The name of this element.
        dbg_path The path in the JSON hierarchy previous to this property.
                 This is used for debug messages.
        unpacked The property is written into this.

    Returns:
        No return value
    """
    if not isinstance(name, str):
        raise TypeError("'name' parameter was " + str(type(name)) + " not str")
    if not isinstance(dbg_path, str):
        raise TypeError("'dbg_path' parameter was " + str(type(dbg_path)) + " not str")
    if not isinstance(unpacked, JsonObject):
        raise TypeError("'unpacked' parameter was " + str(type(unpacked)) + " not JsonObject")

    # check for an invalid schema
    if not isinstance(prop, dict):
        raise UnpackError("'" + dbg_path + "/" + name + "' is not of type dict, it is '" + str(type(prop)) + "'")

    # try out the omniverseFormat key first
    try:
        ex_type = prop["omniverseFormat"]
        if not isinstance(ex_type, str):
            raise UnpackError(
                "'" + dbg_path + "/" + name + "/omniverseFormat' is not of type str, it is '" + str(type(prop)) + "'"
            )
    except KeyError:
        ex_type = None

    # our bindings have to be statically typed, so we need to obtain the type
    try:
        param_type = prop["type"]
        if not isinstance(param_type, str) and not isinstance(param_type, list):
            raise UnpackError(
                "'" + dbg_path + "/" + name + "/type' is not of type str or list, it is '" + str(type(prop)) + "'"
            )
    except KeyError:
        param_type = None

    validate_type(param_type, ex_type, dbg_path + "/" + name)
    if ex_type is not None:
        param_type = ex_type

    elif isinstance(param_type, list):
        raise UnpackError("'" + dbg_path + "/" + name + "/type' cannot be a list of types")

    # ignore null types because they're useless
    if param_type == "null":
        return

    # this will be the output object from this function
    # this will have type Parameter
    param = None

    # create param
    if param_type is not None:
        json_array_type = None

        if isinstance(param_type, list):
            raise UnpackError(
                "'"
                + dbg_path
                + "/"
                + name
                + "/type' is a list of types."
                + " The C++ binding generator requires statically typed properties."
            )

        # check for an invalid schema
        if not isinstance(param_type, str):
            raise UnpackError(
                "'" + dbg_path + "/" + name + "/type' was not a string, it was a '" + str(type(param_type)) + "'"
            )

        # try out the omniverseFormat key first
        try:
            array_ex_type = prop["items"]["omniverseFormat"]
        except KeyError:
            array_ex_type = None

        # check for the items/type field for an array
        try:
            array_type = prop["items"]["type"]
        except KeyError:
            array_type = None  # this will happen if it's not an array

        if array_type is None:
            # check for contains/type for a weakly typed array
            try:
                array_type = prop["contains"]["type"]
                print("warning: " + dbg_path + "/" + name + "/contains/type: treating this as if it were /items/type")
            except KeyError:
                array_type = None  # this will happen if it's not an array

        if array_type is not None or array_ex_type is not None:
            validate_type(array_type, array_ex_type, dbg_path + "/" + name + "/items")
            if array_ex_type is not None:
                array_type = array_ex_type

        if array_type is not None:
            if isinstance(array_type, list):
                raise UnpackError(
                    "'"
                    + dbg_path
                    + "/"
                    + name
                    + "/items/type' is a list of types."
                    + " The C++ binding generator requires statically typed properties."
                )

            if not isinstance(array_type, str):
                raise UnpackError(
                    "'"
                    + dbg_path
                    + "/"
                    + name
                    + "/items/type': was not a string, it was a '"
                    + str(type(array_type) + "'")
                )

            # null type is useless
            if array_type == "null":
                return

            try:
                json_array_type = JsonType.fromString(array_type)
            except KeyError:
                raise UnpackError("'" + dbg_path + "/" + name + "/items/type': has unknown type (" + array_type + ")")

        try:
            json_type = JsonType.fromString(param_type)
        except KeyError:
            raise UnpackError("'" + dbg_path + "/" + name + "type': unknown JSON type '" + param_type + "'")

        # create the parameter based on its specified type
        try:
            param = Parameter(name, json_type, json_array_type)
        except UnpackError as e:
            raise UnpackError("'" + dbg_path + "/" + name + "': " + str(e))

        # check if it has a constant value specified
        try:
            param.const_val = prop["const"]
        except KeyError:
            pass  # parameter isn't constant, which is fine
        except TypeError as t:
            raise UnpackError("'" + dbg_path + "/" + name + "const': " + str(t))

        # you can't have an enumeration on a constant value
        if param.const_val is None:
            # grab the enum, if possible
            try:
                param.enumeration = prop["enum"]
            except KeyError:
                pass
            except TypeError as e:
                print("(non-fatal): '" + dbg_path + "/" + name + "enum': " + str(e))
            except UnpackError as e:
                print("(non-fatal): '" + dbg_path + "/" + name + "enum': " + str(e))
    else:
        # type was not specified, but we can infer it from the constant value
        try:
            const_val = prop["const"]
        except KeyError:
            raise UnpackError(
                "'"
                + dbg_path
                + "/"
                + name
                + "' has no 'type' or 'const' field."
                + " The C++ binding generator requires statically typed properties."
            )

        param_type = JsonType.fromValue(const_val)
        array_type = None

        if param_type == JsonType.OBJECT:
            raise UnpackError("'" + dbg_path + "/" + name + "/const' is an object, which is not supported yet")
        elif param_type == JsonType.ARRAY:
            if len(const_val) == 0:
                print("WARNING: '" + dbg_path + "/" + name + "/const' is an empty array?")
                return

            try:
                array_type = JsonType.fromArray(const_val)
            except TypeError as e:
                raise UnpackError(dbg_path + "/" + name + "/const is invalid: " + str(e))

        param = Parameter(name, param_type, array_type)
        param.const_val = const_val

    # parameter description
    try:
        param.description = prop["description"]
    except KeyError:
        raise UnpackError(
            "'" + dbg_path + "/" + name + "/description' doesn't exist. Descriptions are required on all properties."
        )
    except TypeError:
        raise UnpackError(
            "'"
            + dbg_path
            + "/"
            + name
            + "/description' is not a string, it is a '"
            + str(type(prop["description"]) + "'")
        )

    # if this is an object type, we need to obtain the properties of that object
    if param.json_type == JsonType.OBJECT:
        try:
            subprop = prop["properties"]
        except KeyError:
            raise UnpackError("'" + dbg_path + "/" + name + "' has type 'object' but no 'properties' field")
    elif param.json_type == JsonType.ARRAY and param.array_type == JsonType.OBJECT:
        try:
            subprop = prop["items"]["properties"]
        except KeyError:
            raise UnpackError("'" + dbg_path + "/" + name + "' has type 'object' but no 'items/properties'")
    else:
        # this is not an object type, so we just need to write this into the output dictionary and we're done
        unpacked.add_param(name, param)
        return

    if not isinstance(subprop, dict):
        raise UnpackError("'" + dbg_path + "/" + name + "/properties' is not a dict, it is a " + str(type(subprop)))

    obj = JsonObject(name)
    for p in subprop.items():
        unpack_property(p[1], p[0], dbg_path + "/" + name, obj)

    param.obj = obj
    unpacked.add_param(name, param)


def unpack_event(eventPrefix, ev):
    """
    Unpack a JSON event into a JsonObject

    Args:
        eventPrefix The prefix to event names.
        ev          The JSON event node to parse.

    Returns:
        The created JsonObject.
    """
    if eventPrefix is not None and not isinstance(eventPrefix, str):
        raise TypeError("'eventPrefix' parameter was " + str(type(eventPrefix)) + " not str")
    if not isinstance(ev, tuple):
        raise UnpackError("event '" + str(ev) + "' is not a tuple, it is a '" + str(type(ev)) + "'")

    event_name = ev[0]

    dbg_path = "definitions/events/" + event_name

    # event description
    try:
        desc = ev[1]["description"]
        if not isinstance(desc, str):
            raise UnpackError("'" + dbg_path + "/description' is not a string, it is a " + str(type(desc)))
    except KeyError:
        raise UnpackError("'" + dbg_path + "/description' doesn't exist. Descriptions are required on all events.")

    # check the event meta
    try:
        meta = ev[1]["eventMeta"]
        if not isinstance(meta, dict):
            raise UnpackError("'" + dbg_path + "/eventMeta' is not a dict, it is a " + str(type(meta)))

        # check that the service tag is marked correctly
        try:
            service = meta["service"]
            if not isinstance(service, str):
                raise UnpackError("'" + dbg_path + "/eventMeta/service' is not a string, it is a " + str(type(service)))
            if service != "telemetry" and service != "feedback":
                raise UnpackError(
                    "'" + dbg_path + "/eventMeta/service' is not 'telemetry' or 'feedback'. This is not a valid schema."
                )
        except KeyError:
            raise UnpackError(
                "'" + dbg_path + "/eventMeta/service' doesn't exist. eventMeta/service is required on all events."
            )

        try:
            flags = meta["omniverseFlags"]
            if not isinstance(flags, list):
                raise UnpackError("'" + dbg_path + "/eventMeta/omniverseFlags' isn't a list")
            for i in range(len(flags)):
                if not isinstance(flags[i], str):
                    raise UnpackError(
                        "'"
                        + dbg_path
                        + "/eventMeta/omniverseFlags/"
                        + str(i)
                        + "' isn't a string, it is a "
                        + str(type(flags[i]))
                    )
                if flags[i] not in [
                    "fEventFlagUseLocalLog",
                    "fEventFlagCriticalEvent",
                    "fEventFlagPseudonymize",
                    "fEventFlagAnonymize",
                    "fEventFlagExplicitFlags",
                    "fEventFlagIgnoreOldEvents",
                    "fEventFlagPseudonymizeOldEvents",
                    "fEventFlagUseObjectPointer",
                    "fEventFlagOutputToStdout",
                    "fEventFlagOutputToStderr",
                    "fEventFlagSkipLog",
                ]:
                    raise UnpackError(
                        "'" + dbg_path + "/eventMeta/omniverseFlags/" + flags[i] + "' is not a known flag name"
                    )
            if len(flags) == 0:
                flags = None
        except KeyError:
            flags = None

        try:
            privacy = meta["privacy"]

            if not isinstance(privacy, dict):
                raise UnpackError("'" + dbg_path + "/eventMeta/privacy' is not a string, it is a " + str(type(privacy)))
            try:
                category = privacy["category"]
                if not isinstance(category, str):
                    raise UnpackError(
                        "'" + dbg_path + "/eventMeta/privacy/category' is not a string, it is a " + str(type(category))
                    )
                if category not in ["usage", "personalization", "performance"]:
                    raise UnpackError(
                        "'"
                        + dbg_path
                        + "/eventMeta/privacy/category' must be one of: 'usage', 'personalization', 'performance'."
                    )
            except KeyError:
                raise UnpackError(
                    "'"
                    + dbg_path
                    + "/eventMeta/privacy/category' doesn't exist. eventMeta/privacy/category is required on all events."
                )
            try:
                if not isinstance(privacy["description"], str):
                    raise UnpackError(
                        "'"
                        + dbg_path
                        + "/eventMeta/privacy/description' is not a string, it is a "
                        + str(type(privacy["description"]))
                    )
            except KeyError:
                raise UnpackError(
                    "'"
                    + dbg_path
                    + "/eventMeta/privacy/description' doesn't exist. eventMeta/privacy/description is required on all events."
                )
        except KeyError:
            raise UnpackError(
                "'" + dbg_path + "/eventMeta/privacy' doesn't exist. eventMeta/privacy is required on all events."
            )

    except KeyError:
        raise UnpackError("'" + dbg_path + "/eventMeta' doesn't exist. eventMeta is required on all events.")

    try:
        t = ev[1]["type"]
        if not isinstance(t, str) or t != "object":
            raise UnpackError("'" + dbg_path + "/type' is not 'object' (it was " + str(t) + ")")
    except KeyError:
        raise UnpackError("'" + dbg_path + "/type' is not defined")

    try:
        a = ev[1]["additionalProperties"]
        if not isinstance(a, bool) or a:
            raise UnpackError(
                "'"
                + dbg_path
                + "/additionalProperties' must be false (it was '"
                + str(a)
                + "', type: '"
                + str(type(a))
                + "')"
            )
    except KeyError:
        raise UnpackError("'" + dbg_path + "/additionalProperties' is not defined")

    # the useful information from the event gets accumulated into this object
    unpacked = JsonObject(event_name, eventPrefix, flags, desc)

    try:
        prop = ev[1]["properties"]
    except KeyError:
        # no properties - just an empty event
        return unpacked

    if not isinstance(prop, dict):
        raise UnpackError("'" + dbg_path + "/properties' is not a dict, it is a " + str(type(prop)))

    # each of the properties in the event needs to be put into the dictionary
    # that gets sent into @ref omni::structuredlog::IStructuredLog::commitEvent(), so we need to unpack
    # them into a form we can handle
    for p in prop.items():
        unpack_property(p[1], p[0], dbg_path + "/properties", unpacked)

    # events meant for telemetry must have a 'required' property in each event => verify the
    # property is present and verify its contents.  However, note that the 'required' array
    # is not needed if the event has no properties though.
    if service == "telemetry" and len(prop) > 0:
        try:
            required = ev[1]["required"]
            if len(required) == 0:
                raise UnpackError("'" + dbg_path + "/required' must not be empty.")

            has_all = True
            missing_properties = []
            for p in prop.items():
                if not p[0] in required:
                    has_all = False
                    missing_properties.append(p[0])

            if not has_all:
                try:
                    dependencies = ev[1]["dependencies"]

                except KeyError:
                    raise UnpackError(
                        "'"
                        + dbg_path
                        + "/dependencies' is needed if '*/required' does not contain all properties.  The properties '"
                        + str(missing_properties)
                        + "' are missing."
                    )

        except KeyError:
            raise UnpackError("'" + dbg_path + "/required' is not defined.")

    return unpacked


def generate_code(events, client, version, flags, namespace):
    if not isinstance(events, list):
        raise TypeError("'events' parameter was " + str(type(events)) + " not list")
    if not isinstance(client, str):
        raise TypeError("'client' parameter was " + str(type(client)) + " not str")
    if not isinstance(version, str):
        raise TypeError("'version' parameter was " + str(type(version)) + " not str")
    if flags is not None and not isinstance(flags, list):
        raise TypeError("'flags' parameter was " + str(type(flags)) + " not list")
    if namespace is not None and not isinstance(namespace, list):
        raise TypeError("'namespace' parameter was " + str(type(namespace)) + " not list")

    allow_logging = True
    client_name = client.replace(".", "_").replace("-", "_").replace(" ", "_")
    version_symbol = version.replace(".", "_").replace("-", "_").replace(" ", "_").replace(",", "_").replace(":", "_")
    class_name = "Schema_" + client_name + "_" + version_symbol
    generator_flags = [
        "fEventFlagAnonymize",
        "fEventFlagPseudonymize",
        "fEventFlagExplicitFlags",
        "fEventFlagIgnoreOldEvents",
        "fEventFlagPseudonymizeOldEvents",
        "fEventFlagUseObjectPointer",
    ]

    if flags is not None:
        for f in flags:
            if f == "fSchemaFlagNoLogging":
                allow_logging = False
                break

    # log channels in headers are not realistic yet because they require the user
    # of that header to define a symbol in their module
    if False:
        log_channel_name = "k" + class_name + "LogChannel"
        log_channel_param = log_channel_name + ", "
    else:
        log_channel_name = None
        log_channel_param = ""

    # start out our output with the standard copyright header
    code = "// Copyright (c) " + str(datetime.datetime.now().year) + ", NVIDIA CORPORATION. All rights reserved.\n"
    code += "//\n"
    code += "// NVIDIA CORPORATION and its licensors retain all intellectual property\n"
    code += "// and proprietary rights in and to this software, related documentation\n"
    code += "// and any modifications thereto. Any use, reproduction, disclosure or\n"
    code += "// distribution of this software and related documentation without an express\n"
    code += "// license agreement from NVIDIA CORPORATION is strictly prohibited.\n"
    code += "//\n"

    # add a warning that this is a generated file
    code += "// DO NOT MODIFY THIS FILE. This is a generated file.\n"

    # write out which file this came from
    # only use the basename to avoid absolute paths requiring rebuilds
    code += "// This file was generated from: " + os.path.basename(sys.argv[1]) + "\n"

    # this is disabled for now because absolute paths could result in unnecessary git diffs
    if False:
        # write out the command line arguments, so it's easy to regenerate the file
        code += "// This file was generated with the following command:\n"
        code += "//   "
        for arg in sys.argv:
            code += " '" + arg + "'"
        code += "\n"

    code += "//\n"

    code += "#pragma once\n"
    code += "\n"

    if allow_logging:
        code += "#include <omni/log/ILog.h>\n"

    code += "#include <omni/structuredlog/IStructuredLog.h>\n"
    code += "#include <omni/structuredlog/JsonTree.h>\n"
    code += "#include <omni/structuredlog/BinarySerializer.h>\n"
    code += "#include <omni/structuredlog/StringView.h>\n"
    code += "\n"
    code += "#include <memory>\n"
    code += "\n"
    if log_channel_name is not None and allow_logging:
        code += (
            "OMNI_LOG_ADD_CHANNEL("
            + log_channel_name
            + '"omni.message.'
            + class_name
            + '", "logging channel for generated code for '
            + class_name
            + '")\n'
        )
        code += "\n"

    namespace_str = ""

    # namespace scoping
    for name in namespace:
        namespace_str += name + "::"
        code += "namespace " + name + "\n"
        code += "{\n"

    # generate custom macros for each event.
    for e in events:
        code += "\n"
        code += e.generate_send_macros(namespace_str, class_name, client_name, version_symbol)

    code += "\n"
    code += "class " + class_name + "\n"
    code += "{\n"
    code += "public:\n"

    # generate the enums
    for e in events:
        for p in e.params.items():
            if p[1].enumeration is not None:
                code += p[1].generate_enum_definition("    ")
                code += "\n"

    # generate the structs
    for e in events:
        for p in e.params.items():
            if p[1].json_type == JsonType.OBJECT or (
                p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.OBJECT
            ):
                code += "    /** struct definition for parameter " + p[1].c_name + " of event " + e.name + ". */\n"
                code += p[1].obj.generate_struct_code("    ")
                code += "\n"

    # generate the event IDs.
    code += "    /** the event ID names used to send the events in this schema.  These IDs\n"
    code += "     *  are used when the schema is first registered, and are passed to the\n"
    code += "     *  allocEvent() function when sending the event.\n"
    code += "     */\n"
    code += "    enum : uint64_t\n"
    code += "    {\n"
    for e in events:
        code += (
            "        "
            + e.event_id_name
            + ' = OMNI_STRUCTURED_LOG_EVENT_ID("'
            + client
            + '", "'
            + e.name
            + '", "'
            + version
            + '", "'
            + str(JSON_NODE_VERSION)
            + '"),\n'
        )

    code += "    };\n"

    # generate a default constructor.
    code += "\n"
    code += "    " + class_name + "() = default;\n"
    code += "\n"

    # generate the schema registration function.
    code += "    /** Register this class with the @ref omni::structuredlog::IStructuredLog interface.\n"
    code += "     *  @param[in] flags The flags to pass into @ref omni::structuredlog::IStructuredLog::allocSchema()\n"
    code += "     *                   This may be zero or more of the @ref omni::structuredlog::SchemaFlags flags.\n"
    code += "     *  @returns `true` if the operation succeded.\n"
    code += "     *  @returns `false` if @ref omni::structuredlog::IStructuredLog couldn't be loaded.\n"
    code += "     *  @returns `false` if a memory allocation failed.\n"
    code += "     */\n"
    code += "    static bool registerSchema(omni::structuredlog::IStructuredLog* strucLog) noexcept\n"
    code += "    {\n"
    code += "        return _registerSchema(strucLog);\n"
    code += "    }\n"
    code += "\n"
    code += "    /** Check whether this structured log schema is enabled.\n"
    code += "     *  @param[in] eventId     the ID of the event to check the enable state for.\n"
    code += "     *                         This must be one of the @a k*EventId symbols\n"
    code += "     *                         defined above.\n"
    code += "     *  @returns Whether this client is enabled.\n"
    code += "     */\n"
    code += "    static bool isEnabled(omni::structuredlog::EventId eventId) noexcept\n"
    code += "    {\n"
    code += "        return _isEnabled(eventId);\n"
    code += "    }\n"
    code += "\n"
    code += "    /** Enable/disable an event in this schema.\n"
    code += "     *  @param[in] eventId     the ID of the event to enable or disable.\n"
    code += "     *                         This must be one of the @a k*EventId symbols\n"
    code += "     *                         defined above.\n"
    code += "     *  @param[in] enabled     Whether is enabled or disabled.\n"
    code += "     */\n"
    code += "    static void setEnabled(omni::structuredlog::EventId eventId, bool enabled) noexcept\n"
    code += "    {\n"
    code += "        _setEnabled(eventId, enabled);\n"
    code += "    }\n"
    code += "\n"
    code += "    /** Enable/disable this schema.\n"
    code += "     *  @param[in] enabled     Whether is enabled or disabled.\n"
    code += "     */\n"
    code += "    static void setEnabled(bool enabled) noexcept\n"
    code += "    {\n"
    code += "        _setEnabled(enabled);\n"
    code += "    }\n"

    code += "\n"
    code += "    /** event enable check helper functions.\n"
    code += "     *\n"
    code += "     *  @param[in] strucLog   The structured log object to use to send this event.  This\n"
    code += "     *                        must not be nullptr.  It is the caller's responsibility\n"
    code += "     *                        to ensure that a valid object is passed in.\n"
    code += "     *  @returns `true` if the specific event and this schema are both enabled.\n"
    code += "     *  @returns `false` if either the specific event or this schema is disabled.\n"
    code += "     *\n"
    code += "     *  @remarks These check if an event corresponding to the function name is currently\n"
    code += "     *           enabled.  These are useful to avoid parameter evaluation before calling\n"
    code += "     *           into one of the event emitter functions.  These will be called from the\n"
    code += "     *           OMNI_STRUCTURED_LOG() macro.  These may also be called directly if an event\n"
    code += "     *           needs to be emitted manually, but the only effect would be the potential\n"
    code += "     *           to avoid parameter evaluation in the *_sendEvent() function.  Each\n"
    code += "     *           *_sendEvent() function itself will also internally check if the event\n"
    code += "     *           is enabled before sending it.\n"
    code += "     *  @{\n"
    code += "     */"

    for e in events:
        code += "\n"
        code += "    static bool " + e.enabled_fn_name + "(omni::structuredlog::IStructuredLog* strucLog) noexcept\n"
        code += "    {\n"
        code += "        return strucLog->isEnabled(" + e.event_id_name + ");\n"
        code += "    }\n"

    code += "    /** @} */\n"

    for e in events:
        code += "\n"
        code += e.generate_send_function(log_channel_param, "result", False, allow_logging, flags)

    code += "\n"
    code += "private:\n"
    code += "    /** This will allow us to disable array length checks in release builds,\n"
    code += "     *  since they would have a negative performance impact and only be hit\n"
    code += "     *  in unusual circumstances.\n"
    code += "     */\n"
    code += "    static constexpr bool kValidateLength = CARB_DEBUG;\n"
    code += "\n"

    code += "    /** body for the registerSchema() public function. */\n"
    code += "    static bool _registerSchema(omni::structuredlog::IStructuredLog* strucLog)\n"
    code += "    {\n"
    code += "        omni::structuredlog::AllocHandle handle = {};\n"
    code += "        omni::structuredlog::SchemaResult result;\n"
    code += "        uint8_t* buffer;\n"
    code += "        omni::structuredlog::EventInfo events[" + str(len(events)) + "] = {};\n"
    code += "        size_t bufferSize = 0;\n"
    code += "        size_t total = 0;\n"
    code += "        omni::structuredlog::SchemaFlags flags = "

    def _map_schema_flag_to_event_flag(flag: str):
        if flag == "fSchemaFlagOutputToStderr":
            return "fEventFlagOutputToStderr"

        if flag == "fSchemaFlagOutputToStdout":
            return "fEventFlagOutputToStdout"

        if flag == "fSchemaFlagSkipLog":
            return "fEventFlagSkipLog"

    anyFlags = False
    schema_log_flags = []
    if flags is not None:
        for f in flags:
            if f not in [
                "fSchemaFlagAnonymizeEvents",
                "fSchemaFlagPseudonymizeEvents",
                "fSchemaFlagNoLogging",
                "fSchemaFlagIgnoreOldEvents",
                "fSchemaFlagPseudonymizeOldEvents",
                "fSchemaFlagOutputToStderr",
                "fSchemaFlagOutputToStdout",
                "fSchemaFlagSkipLog",
            ]:
                anyFlags = True
                break

        for f in flags:
            # these flags are only for the code generator to pass over to the event side.
            if f in [
                "fSchemaFlagOutputToStderr",
                "fSchemaFlagOutputToStdout",
                "fSchemaFlagSkipLog",
            ]:
                schema_log_flags.append(_map_schema_flag_to_event_flag(f))

    if anyFlags:
        first = True
        for f in flags:
            # this flag is only for code generation
            if f in ["fSchemaFlagNoLogging"]:
                allow_logging = False
                continue

            # these flags are only for the transmitter
            if f in [
                "fSchemaFlagAnonymizeEvents",
                "fSchemaFlagPseudonymizeEvents",
                "fSchemaFlagIgnoreOldEvents",
                "fSchemaFlagPseudonymizeOldEvents",
            ]:
                continue

            if not first:
                code += " | "

            code += "omni::structuredlog::" + f
            first = False
    else:
        code += "0"

    code += ";\n"

    code += "\n"
    code += "        if (strucLog == nullptr)\n"
    code += "        {\n"

    if allow_logging:
        code += (
            "            OMNI_LOG_WARN(\n"
            + '                "no structured log object!  The schema "\n'
            + "                \"'"
            + class_name
            + "' \"\n"
            + '                "will be disabled.");\n'
        )

    code += "            return false;\n"
    code += "        }\n"
    code += "\n"
    code += "        // calculate the tree sizes\n"
    for e in events:
        code += "        size_t " + e.c_name + "_size = " + e.calc_size_fn_name + "();\n"
    code += "\n"
    code += "        // calculate the event buffer size\n"
    for e in events:
        code += "        bufferSize += " + e.c_name + "_size;\n"
    code += "\n"
    code += "        // begin schema creation\n"
    code += '        buffer = strucLog->allocSchema("' + client + '", "' + version + '", flags, bufferSize, &handle);\n'
    code += "        if (buffer == nullptr)\n"
    code += "        {\n"
    if allow_logging:
        code += (
            "            OMNI_LOG_ERROR("
            + log_channel_param
            + '"allocSchema failed (size = %zu bytes)", bufferSize);\n'
        )
    code += "            return false;\n"
    code += "        }\n"
    code += "\n"
    code += "        // register all the events\n"
    for i in range(len(events)):
        code += (
            "        events["
            + str(i)
            + "].schema = "
            + events[i].build_tree_fn_name
            + "("
            + events[i].c_name
            + "_size, buffer + total);\n"
        )
        code += "        events[" + str(i) + '].eventName = "' + events[i].name + '";\n'
        code += "        events[" + str(i) + "].parserVersion = " + str(JSON_NODE_VERSION) + ";\n"
        code += "        events[" + str(i) + "].eventId = " + events[i].event_id_name + ";\n"

        anyFlags = False
        event_flags = []
        if events[i].flags is not None:
            event_flags.extend(events[i].flags)
        event_flags.extend(schema_log_flags)
        if len(event_flags) > 0:
            for f in event_flags:
                if f not in generator_flags:
                    anyFlags = True
                    break

        # if there are any flags, add those
        if anyFlags:
            code += "        events[" + str(i) + "].flags ="
            first = True
            for f in event_flags:
                # these flags are only for the transmitter
                if f in generator_flags:
                    continue

                if not first:
                    code += " | "
                else:
                    code += " "

                code += "omni::structuredlog::" + f
                first = False

            code += ";\n"

        code += "        total += " + events[i].c_name + "_size;\n"

    code += "\n"
    code += "        result = strucLog->commitSchema(handle, events, CARB_COUNTOF(events));\n"
    code += "        if (result != omni::structuredlog::SchemaResult::eSuccess &&\n"
    code += "            result != omni::structuredlog::SchemaResult::eAlreadyExists)\n"
    code += "        {\n"
    if allow_logging:
        code += "            OMNI_LOG_ERROR(" + log_channel_param + "\n"
        code += '                "failed to register structured log events "\n'
        code += '                "{result = %s (%zu)}",\n'
        code += "                getSchemaResultName(result), size_t(result));\n"
    code += "            return false;\n"
    code += "        }\n"
    code += "\n"
    code += "        return true;\n"
    code += "    }\n"
    code += "\n"

    code += "    /** body for the isEnabled() public function. */\n"
    code += "    static bool _isEnabled(omni::structuredlog::EventId eventId)\n"
    code += "    {\n"
    code += "        omni::structuredlog::IStructuredLog* strucLog = omniGetStructuredLogWithoutAcquire();\n"
    code += "        return strucLog != nullptr && strucLog->isEnabled(eventId);\n"
    code += "    }\n"
    code += "\n"
    code += "    /** body for the setEnabled() public function. */\n"
    code += "    static void _setEnabled(omni::structuredlog::EventId eventId, bool enabled)\n"
    code += "    {\n"
    code += "        omni::structuredlog::IStructuredLog* strucLog = omniGetStructuredLogWithoutAcquire();\n"
    code += "        if (strucLog == nullptr)\n"
    code += "            return;\n"
    code += "\n"
    code += "        strucLog->setEnabled(eventId, 0, enabled);\n"
    code += "    }\n"
    code += "\n"
    code += "    /** body for the setEnabled() public function. */\n"
    code += "    static void _setEnabled(bool enabled)\n"
    code += "    {\n"
    code += "        omni::structuredlog::IStructuredLog* strucLog = omniGetStructuredLogWithoutAcquire();\n"
    code += "        if (strucLog == nullptr)\n"
    code += "            return;\n"
    code += "\n"
    code += (
        "        strucLog->setEnabled("
        + events[0].event_id_name
        + ", omni::structuredlog::fEnableFlagWholeSchema, enabled);\n"
    )
    code += "    }\n"

    code += "\n"
    code += "#if OMNI_PLATFORM_WINDOWS\n"
    code += "#    pragma warning(push)\n"
    code += "#    pragma warning(disable : 4127) // warning C4127: conditional expression is constant.\n"
    code += "#endif\n"
    for e in events:
        code += "\n"
        code += e.generate_send_function(log_channel_param, "result", True, allow_logging, flags)

    code += "#if OMNI_PLATFORM_WINDOWS\n"
    code += "#    pragma warning(pop)\n"
    code += "#endif\n"
    code += "\n"

    for e in events:
        code += e.generate_tree_create_functions(log_channel_param, "result", allow_logging)
        code += "\n"

    code += "    /** The callback that is used to report validation errors.\n"
    code += "     *  @param[in] s The validation error message.\n"
    code += "     */\n"
    code += "    static void _onStructuredLogValidationError(const char* s)\n"
    code += "    {\n"
    if allow_logging:
        code += "        OMNI_LOG_ERROR(" + log_channel_param + '"error sending a structured log event: %s", s);\n'
    code += "    }\n"
    code += "};\n"
    code += "\n"

    code += "// asserts to ensure that no one's modified our dependencies\n"
    code += (
        "static_assert(omni::structuredlog::BlobWriter<>::kVersion == "
        + str(BLOB_VERSION)
        + ', "BlobWriter verison changed");\n'
    )
    code += (
        "static_assert(omni::structuredlog::JsonNode::kVersion == "
        + str(JSON_NODE_VERSION)
        + ', "JsonNode verison changed");\n'
    )
    code += 'static_assert(sizeof(omni::structuredlog::JsonNode) == 24, "unexpected size");\n'
    code += 'static_assert(std::is_standard_layout<omni::structuredlog::JsonNode>::value, "this type needs to be ABI safe");\n'
    code += 'static_assert(offsetof(omni::structuredlog::JsonNode, type) == 0, "struct layout changed");\n'
    code += 'static_assert(offsetof(omni::structuredlog::JsonNode, flags) == 1, "struct layout changed");\n'
    code += 'static_assert(offsetof(omni::structuredlog::JsonNode, len) == 2, "struct layout changed");\n'
    code += 'static_assert(offsetof(omni::structuredlog::JsonNode, nameLen) == 4, "struct layout changed");\n'
    code += 'static_assert(offsetof(omni::structuredlog::JsonNode, name) == 8, "struct layout changed");\n'
    code += 'static_assert(offsetof(omni::structuredlog::JsonNode, data) == 16, "struct layout changed");\n'
    code += "\n"

    # namespace scoping
    for i in reversed(range(len(namespace))):
        code += "} // namespace " + namespace[i] + "\n"

    code += "\n"
    code += (
        "OMNI_STRUCTURED_LOG_ADD_SCHEMA("
        + namespace_str
        + class_name
        + ", "
        + client_name
        + ", "
        + version_symbol
        + ", "
        + str(JSON_NODE_VERSION)
        + ");\n"
    )

    return code


def generate_py_code(events, client, version, flags, schema):
    if not isinstance(events, list):
        raise TypeError("'events' parameter was " + str(type(events)) + " not list")
    if not isinstance(client, str):
        raise TypeError("'client' parameter was " + str(type(client)) + " not str")
    if not isinstance(version, str):
        raise TypeError("'version' parameter was " + str(type(version)) + " not str")
    if flags is not None and not isinstance(flags, list):
        raise TypeError("'flags' parameter was " + str(type(version)) + " not list")
    if not isinstance(schema, dict):
        raise TypeError("'schema' parameter was " + str(type(schema)) + " not dict")

    code = "# Copyright (c) " + str(datetime.datetime.now().year) + ", NVIDIA CORPORATION.  All rights reserved.\n"
    code += "#\n"
    code += "# NVIDIA CORPORATION and its licensors retain all intellectual property\n"
    code += "# and proprietary rights in and to this software, related documentation\n"
    code += "# and any modifications thereto. Any use, reproduction, disclosure or\n"
    code += "# distribution of this software and related documentation without an express\n"
    code += "# license agreement from NVIDIA CORPORATION is strictly prohibited.\n"
    code += "\n"
    code += "import omni.structuredlog\n"
    code += "import json\n"
    code += "import omni.log\n"
    code += "\n"

    # pformat() does a terrible job formatting, so we'll just directly add the JSON as a string
    # code += "schema = " + pprint.pformat(schema, indent = 4, width = 120, compact = False) + "\n"
    code += "# The JSON schema for these events\n"
    code += "# The $ref statements for these events have been expanded because python's\n"
    code += "# standard library json module can't expand $ref statements.\n"
    code += "# If you want to distribute the jsonref package, you can use $ref statements.\n"
    code += 'schema = """\n' + jsonbackend.dumps(schema, indent=4, sort_keys=False) + '\n"""\n'
    code += "\n"
    code += "# the telemetry events dictionary we can use to send events\n"
    code += "events = None\n"
    code += "try:\n"
    code += "    schema = json.loads(schema)\n"
    code += "    events = omni.structuredlog.register_schema(schema)\n"
    code += "except Exception as e:\n"
    code += '    omni.log.error("failed to register the schema: " + str(type(e)) + " " + str(e))\n'
    code += "\n"
    code += "# These are wrappers for the send functions that you can call to send telemetry events\n"
    for e in events:
        code += e.generate_py_send_function()

    code += "\n"
    return code


def generate_py_binding(events, cpp_header, py_header, client, version, namespace, schema_flags):
    c_name = client.replace(".", "_").replace("-", "_").replace(" ", "_")
    version_symbol = version.replace(".", "_").replace("-", "_").replace(" ", "_").replace(",", "_").replace(":", "_")
    class_name = "Schema_" + c_name + "_" + version_symbol
    wrap_class_name = "Wrap_" + c_name

    code = "// Copyright (c) " + str(datetime.datetime.now().year) + ", NVIDIA CORPORATION. All rights reserved.\n"
    code += "//\n"
    code += "// NVIDIA CORPORATION and its licensors retain all intellectual property\n"
    code += "// and proprietary rights in and to this software, related documentation\n"
    code += "// and any modifications thereto. Any use, reproduction, disclosure or\n"
    code += "// distribution of this software and related documentation without an express\n"
    code += "// license agreement from NVIDIA CORPORATION is strictly prohibited.\n"
    code += "//\n"

    # add a warning that this is a generated file
    code += "// DO NOT MODIFY THIS FILE. This is a generated file.\n"

    # write out which file this came from
    # only use the basename to avoid absolute paths requiring rebuilds
    code += "// This file was generated from: " + os.path.basename(sys.argv[1]) + "\n"

    # this is disabled for now because absolute paths could result in unnecessary git diffs
    if False:
        # write out the command line arguments, so it's easy to regenerate the file
        code += "// This file was generated with the following command:\n"
        code += "//   "
        for arg in sys.argv:
            code += " '" + arg + "'"
        code += "\n"

    code += "#pragma once\n"
    code += "\n"
    code += "#include <carb/BindingsPythonUtils.h>\n"
    code += '#include "' + os.path.relpath(cpp_header, os.path.dirname(py_header)).replace("\\", "/") + '"\n'
    code += "\n"

    # namespace scoping
    for name in namespace:
        code += "namespace " + name + "\n"
        code += "{\n"

    code += "\n"

    use_obj_pointer = False
    if schema_flags is not None and "fSchemaFlagUseObjectPointer" in schema_flags:
        use_obj_pointer = True

    for e in events:
        for p in e.params.items():
            if p[1].json_type == JsonType.OBJECT or (
                p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.OBJECT
            ):
                code += p[1].obj.generate_struct_code("", True, class_name + "::")
                code += "\n"

    code += "class " + wrap_class_name + "\n"
    code += "{\n"
    code += "public:\n"
    code += "    " + wrap_class_name + "() = default;\n"
    code += "    ~" + wrap_class_name + "() = default;\n"

    for e in events:
        code += "\n"
        code += "    void " + e.send_fn_name + "("

        use_pointer = use_obj_pointer
        if e.flags is not None and "fEventFlagUseObjectPointer" in e.flags:
            use_pointer = True

        first = True
        for p in e.params.items():
            if not first:
                code += ", "
            first = False
            if p[1].is_const_obj_array_with_dynamic_len():
                code += p[1].get_c_array_len_type() + " " + p[1].get_array_length_name()
            else:
                code += p[1].get_python_binding_param_type(class_name + "::", use_pointer) + " " + p[1].c_name

        code += ")\n"
        code += "    {\n"

        for p in e.params.items():
            code += p[1].python_binding_type_to_native(class_name + "::", "", "        ", use_pointer)

        code += "        OMNI_STRUCTURED_LOG(" + class_name + "::" + e.c_name

        if e.flags is not None and "fEventFlagExplicitFlags" in e.flags:
            code += ", 0"

        for p in e.params.items():
            code += ", "

            name = p[1].c_name
            if p[1].json_type == JsonType.OBJECT or (
                p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.OBJECT
            ):
                name += "_"
            if p[1].json_type == JsonType.OBJECT:
                if use_pointer:
                    name = "&" + name

            if p[1].is_const_obj_array_with_dynamic_len():
                code += p[1].get_array_length_name()
            elif p[1].need_length_parameter():
                code += name + ".size()"
                if p[1].json_type == JsonType.STRING:
                    code += " + 1"

            if not p[1].is_const_obj_array_with_dynamic_len():
                if p[1].enumeration is None and p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.STRING:
                    code += ", "
                    code += name + "_.get()"
                elif p[1].enumeration is None and p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.BOOL:
                    code += ", "
                    code += name + "_.get()"
                elif p[1].json_type == JsonType.STRING:
                    code += name
                elif p[1].need_length_parameter():
                    code += ", "
                    code += name + ".data()"
                else:
                    code += name

        code += ");\n"
        code += "    }\n"

    code += "};\n"
    code += "\n"

    # start the actual binding function
    code += "inline void definePythonModule_" + c_name + "(py::module& m)\n"
    code += "{\n"
    code += "    using namespace omni::structuredlog;\n"
    code += '    m.doc() = "bindings for structured log schema ' + client + '";\n'
    code += "\n"

    # generate the enums
    for e in events:
        for p in e.params.items():
            if p[1].enumeration is not None:
                code += p[1].generate_py_enum_definition(class_name + "::", "m", "    ")
                code += "\n"

    # generate the structs
    for e in events:
        for p in e.params.items():
            if p[1].json_type == JsonType.OBJECT or (
                p[1].json_type == JsonType.ARRAY and p[1].array_type == JsonType.OBJECT
            ):
                code += p[1].obj.generate_py_struct_code(class_name + "::", "", "m", "    ")
                code += "\n"

    code += "    // the main structured log class\n"
    code += "    py::class_<" + wrap_class_name + '>(m, "' + class_name + '")'
    code += "\n        .def(py::init<>())"  # default constructor
    for e in events:
        code += "\n        "
        code += '.def("' + e.send_fn_name + '", &' + wrap_class_name + "::" + e.send_fn_name
        # add the py::arg() calls for ease of calling
        for p in e.params.items():
            if not p[1].is_const_obj_array_with_dynamic_len():
                code += ', py::arg("' + p[1].c_name + '")'
            else:
                code += ', py::arg("' + p[1].get_array_length_name() + '")'
        code += ")"

    code += ";\n"

    code += "}\n"

    code += "\n"

    # namespace scoping
    for i in reversed(range(len(namespace))):
        code += "} // namespace " + namespace[i] + "\n"

    return code


def bake_properties(blob, event_name):
    output = {}
    required = []
    if "properties" not in blob:
        return output, required

    for prop in blob["properties"].items():
        p = {}
        if "type" in prop[1]:
            base_type = prop[1]["type"]
            if not isinstance(base_type, str):
                raise UnpackError("event '" + event_name + "' property '" + prop[0] + "' `type` field is not a string")

            # valid types for the parboiled schema
            valid_types = [
                "bool",
                "int32",
                "uint32",
                "int64",
                "uint64",
                "float32",
                "float64",
                "string",
                "binary",
                "object",
            ]

            # translations to JSON
            json_types = {
                "bool": "boolean",
                "int32": "integer",
                "uint32": "integer",
                "int64": "integer",
                "uint64": "integer",
                "float32": "number",
                "float64": "number",
                "string": "string",
                "binary": "string",
                "object": "object",
            }

            # omniverseFormat for some types
            omniverse_types = {
                "uint32": "uint32",
                "int64": "int64",
                "uint64": "uint64",
                "float32": "float32",
                "binary": "binary",
            }

            # add [] to the end of a type to make it an array
            array = False
            if base_type.endswith("[]"):
                array = True
                base_type = base_type[:-2]

            if base_type not in valid_types:
                raise UnpackError(
                    "event '"
                    + event_name
                    + "' property '"
                    + prop[0]
                    + "' `type` field has unknown value '"
                    + base_type
                    + "'"
                )

            type_dict = p
            if array:
                if base_type == "binary":
                    raise UnpackError(
                        "event '" + event_name + "' property '" + prop[0] + "' has type binary[], which is invalid"
                    )

                p["type"] = "array"
                p["items"] = {"type": json_types[base_type]}
                if base_type in omniverse_types:
                    p["items"]["omniverseFormat"] = omniverse_types[base_type]
                type_dict = p["items"]
            else:
                p["type"] = json_types[base_type]
                if base_type in omniverse_types:
                    p["omniverseFormat"] = omniverse_types[base_type]

            # add properties to objects
            if base_type == "object":
                if "properties" not in prop[1]:
                    p["properties"] = {}
                else:
                    if array:
                        base = p["items"]
                    else:
                        base = p

                    base["properties"], base["required"] = bake_properties(prop[1], event_name + "/" + prop[0])

        elif "enum" in prop[1]:
            try:
                if not isinstance(prop[1]["enum"], list):
                    raise UnpackError(
                        "event '" + event_name + "' enum tag was a '" + str(type(prop[1]["enum"])) + "' not a list"
                    )

                p["type"] = JsonType.fromArray(prop[1]["enum"]).getJsonNativeType().toString()
                if p["type"] == "object" or p["type"] == "array":
                    raise UnpackError("event '" + event_name + "' enum tag was invalid type '" + p["type"] + "'")

            except TypeError as e:
                raise UnpackError("event '" + event_name + "' enum tag is invalid: " + str(e))

        # copy everything else
        for q in prop[1].items():
            if q[0] not in ["type", "properties", "items"]:
                p[q[0]] = copy.deepcopy(q[1])

        required.append(prop[0])
        output[prop[0]] = p

    return output, required


def bake(filename):
    tup = os.path.splitext(filename)

    try:
        with open(in_file) as f:
            if tup[1] == ".json":
                blob = jsonbackend.load(f)
            elif tup[1] == ".py" or tup[1] == ".schema":
                blob = ast.literal_eval(f.read())
            else:
                raise UnpackError("unknown file extension '" + tup[1] + "' on file '" + filename + "'")
    except JsonError as e:
        raise UnpackError("failed to parse '" + filename + "': " + str(e))
    except json.decoder.JSONDecodeError as e:
        raise UnpackError("failed to parse '" + filename + "': " + str(e))
    except ValueError:
        raise
    except FileNotFoundError as e:
        raise UnpackError(str(e))
    except Exception as e:
        raise UnpackError(str(type(e)) + ": " + str(e))

    required = ["events", "name", "version", "namespace"]
    for r in required:
        if r not in blob:
            raise UnpackError("schema is missing the `" + r + "` element")

    output = {
        "generated": "This was generated from " + os.path.basename(sys.argv[1]) + ".",
        "anyOf": [],
        # we're sticking to the draft-07 because 2019-09 support in various JSON schema SDKs is rare.
        "$schema": "http://json-schema.org/draft-07/schema#",
        "schemaMeta": {
            "clientName": blob["name"],
            "schemaVersion": blob["version"],
            "eventPrefix": blob["namespace"],
            "definitionVersion": "1.0",
        },
        "definitions": {"events": {}},
    }

    if "flags" in blob and len(blob["flags"]) > 0:
        output["schemaMeta"]["omniverseFlags"] = copy.deepcopy(blob["flags"])

    if "description" in blob:
        output["schemaMeta"]["description"] = copy.deepcopy(blob["description"])

    if "oldEventsThreshold" in blob:
        output["schemaMeta"]["oldEventsThreshold"] = copy.deepcopy(blob["oldEventsThreshold"])

    banned = ["oneOf", "anyOf", "$schema", "definitions", "generated"]
    for b in banned:
        if b in blob:
            raise UnpackError("schema has element `" + b + "`, which is not allowed")

    # copy whatever else is here
    for entry in blob.items():
        if entry[0] not in ["events", "name", "version", "namespace", "flags", "oldEventsThreshold"]:
            output[entry[0]] = copy.deepcopy(entry[1])

    for ev in blob["events"].items():
        full_name = blob["namespace"] + "." + ev[0]
        obj = {
            "eventMeta": {"service": "telemetry", "privacy": {}},
            "type": "object",
            "additionalProperties": False,
            "required": [],
            "properties": {},
        }

        banned = ["eventMeta", "type", "additionalProperties", "required", "items"]
        for b in banned:
            if b in ev[1]:
                raise UnpackError("schema has element `events/" + ev[0] + "/" + b + "`, which is not allowed")

        if "privacy" not in ev[1]:
            raise UnpackError("privacy tag is missing from `events/" + ev[0] + "/" + b + "`")

        required = ["category", "description"]
        for r in required:
            if r not in ev[1]["privacy"]:
                raise UnpackError("event '" + ev[0] + "' is missing the `" + r + "` element from its privacy section")

        obj["eventMeta"]["privacy"]["category"] = ev[1]["privacy"]["category"]
        obj["eventMeta"]["privacy"]["description"] = ev[1]["privacy"]["description"]

        if "flags" in ev[1]:
            obj["eventMeta"]["omniverseFlags"] = copy.deepcopy(ev[1]["flags"])

        if "oldEventsThreshold" in ev[1]:
            obj["eventMeta"]["oldEventsThreshold"] = copy.deepcopy(ev[1]["oldEventsThreshold"])

        obj["properties"], obj["required"] = bake_properties(ev[1], ev[0])

        if len(obj["required"]) == 0:
            obj.pop("required", None)

        # copy everything else
        for q in ev[1].items():
            if q[0] not in [
                "eventMeta",
                "additionalProperties",
                "required",
                "properties",
                "privacy",
                "flags",
                "omniverseFlags",
                "oldEventsThreshold",
            ]:
                obj[q[0]] = copy.deepcopy(q[1])

        output["anyOf"].append({"$ref": "#/definitions/events/" + full_name})
        output["definitions"]["events"][full_name] = obj

    return output


def _compare_files(file1, file2):
    """
    Compares two files except for the first (copyright) line.

    Params:
        file1: The first file to compare.
        file2: The second file to compare.

    Returns:
        `True` if the two files match with the exception of the first line.
        `False` if the two files differ aside from the first line.
    """

    with open(file1, "r") as fd1:
        with open(file2, "r") as fd2:
            file1_lines = fd1.readlines()
            file2_lines = fd2.readlines()
            file1_count = len(file1_lines)

            # the line count doesn't match -> files don't match => fail.
            if file1_count != len(file2_lines) or file1_count <= 1:
                return False

            # compare all but the first line.
            for i in range(1, file1_count):
                if file1_lines[i] != file2_lines[i]:
                    return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="C++ binding generator for a structured log JSON schema\n"
        + "If no output is specified, this will just validate your schema.\n"
        + "This binding generator has the following limitations:\n"
        + "  1. All properties must be statically typed.\n"
        + "     Each non-constant schema property must have a 'type' field defined.\n"
        + "     Non-constant array properties must have a 'items/type' field defined.\n"
        + "     When specifying an object, any property of that object must have a\n"
        + "     'type' field defined.\n"
        + "     A 'type' field cannot be a list of types.\n"
        + "     This is required to allow primitive C++ types to be used in the binding function.\n"
        + "  2. Properties with type 'array' cannot set their 'items/type' field to 'array'.\n"
        + "  3. A 'const' field cannot contain an object type or an array of object type.\n"
        + "     This is legal from a schema perspective, but it complicates binding generation.\n"
        + "     To make a constant object type, you can specify the 'const' attribute in all\n"
        + "     properties of the object.\n"
        + "  4. All events and event properties must have a name that is a legal C++ identifier,\n"
        + "     with the exception of ' ', '-' and '.' which are replaced with '_' when they\n"
        + "     are translated into C names.\n"
        + "     This is necessary to avoid having to significantly mangle names to\n"
        + "     convert them to something legal in C++.\n"
        + "  5. Constant properties cannot be an empty 'array' or 'object'.\n"
        + "  6. Constant properties cannot be an array of multiple different types.\n"
        + '     For example "const": ["a", 3] will be rejected.\n'
        + "  7. Null typed properties will not be encoded.\n"
        + "  8. The required properties list is ignored. All properties are always required.\n"
        + "     A form of optional parameters does not fit into C++ well.\n"
        + "     The required properties list will still be used by the JSON validator that\n"
        + "     is used when these messages are consumed though.\n",
    )
    parser.add_argument("input", type=str, help="The schema to generate code for.")
    parser.add_argument("--cpp", type=str, help="Output file for the generated C++ header.")
    parser.add_argument(
        "--py-binding",
        type=str,
        help="Output file for the generated pybind11 bindings for the C++ header."
        + " A --cpp argument must have been specified for this to work.",
    )
    parser.add_argument(
        "--py",
        type=str,
        help="Output file for the generated python module that directly uses the structuredlog python bindings.",
    )
    parser.add_argument(
        "--namespace",
        type=str,
        help="Namespace for the generated code."
        + " This only takes effect for C++ code."
        + " Namespaces should be separated with :: (e.g. 'omni::structuredlog')."
        + " The default namespace is 'omni::structuredlog'",
    )
    parser.add_argument(
        "--bake-to",
        type=str,
        help="The path to the JSON schema file that can be used on the telemetry servers."
        " This is created from the basic schema that is specified for input."
        " This output will always be overwritten when the baking step succeeds.",
    )
    parser.add_argument(
        "--baked", action="store_true", help="Treat the input schema as a baked schema. Do not bake the schema."
    )
    parser.add_argument("--repo-root", type=str, help="Root of the repo, where repo.toml can be found.")
    parser.add_argument(
        "--fail-on-write", action="store_true", help="Fail the operation if a write to a file needs to occur."
    )

    args = parser.parse_args()

    in_file = args.input
    cpp_output = args.cpp
    py_binding_output = args.py_binding
    py_output = args.py
    repo_root = args.repo_root
    namespace = ["omni", "structuredlog"]

    print("Generating code for '" + in_file + "'.")

    if cpp_output is None and py_binding_output is not None:
        sys.exit("A --cpp argument must be specified when --py-binding is used")

    if args.namespace is not None:
        namespace = args.namespace.split("::")
        for name in namespace:
            if not name.isidentifier():
                sys.exit(
                    "namespace component '" + name + "' is not an identifier (namespace = '" + args.namespace + "')"
                )

    errors = 0

    # python 3.7 made insertion-ordered dictionaries a required part of the standard,
    # which is required for this code to function correctly
    if sys.version_info[1] < 7:
        sys.exit("this script was designed for python 3.7+")

    unpacked = []

    meta = None
    version = None
    client = None

    outputs = [cpp_output, py_binding_output, py_output, args.bake_to]
    need_rebuild = False
    for o in outputs:
        if o is not None and (
            not os.path.exists(o)
            or os.path.getmtime(o) < os.path.getmtime(in_file)
            or os.path.getmtime(o) < os.path.getmtime(__file__)
        ):
            need_rebuild = True
            break

    if not need_rebuild:
        print("no schema changes detected -> no regeneration needed", file=sys.stderr)
        sys.exit(0)

    if args.baked:
        # load the JSON data
        try:
            with open(in_file) as f:
                blob = jsonbackend.load(f)
        except JsonError as e:
            sys.exit("failed to parse '" + in_file + ": " + str(e))
        except json.decoder.JSONDecodeError as e:
            sys.exit("failed to parse '" + in_file + ": " + str(e))
        except FileNotFoundError as e:
            sys.exit(str(e))
    else:
        try:
            blob = bake(in_file)
        except UnpackError as e:
            sys.exit(str(e))

        if args.bake_to:
            with open(args.bake_to, "w") as fd:
                fd.write(json.dumps(blob, indent=4, sort_keys=False))
                fd.write("\n")

    try:
        meta = blob["schemaMeta"]
    except KeyError:
        sys.exit("no schemaMeta in the input schema (file = '" + in_file + "')")

    try:
        version = meta["schemaVersion"]
        if not isinstance(version, str) or len(version) == 0:
            sys.exit(
                "schemaMeta/schemaVersion has invalid value '"
                + str(version)
                + "' - value must be a non-empty string (file = '"
                + in_file
                + "')"
            )
    except KeyError:
        sys.exit("no schemaMeta/schemaVersion in the input schema (file = '" + in_file + "')")

    try:
        flags = meta["omniverseFlags"]
        if not isinstance(flags, list):
            sys.exit("schemaMeta/omniverseFlags isn't a list")
        for f in flags:
            if not isinstance(f, str):
                sys.exit("schemaMeta/omniverseFlags element (" + str(f) + ") isn't a string, it is a " + str(type(f)))
            if f not in [
                "fSchemaFlagKeepLogOpen",
                "fSchemaFlagPseudonymizeEvents",
                "fSchemaFlagAnonymizeEvents",
                "fSchemaFlagNoLogging",
                "fSchemaFlagLogWithProcessId",
                "fSchemaFlagIgnoreOldEvents",
                "fSchemaFlagPseudonymizeOldEvents",
                "fSchemaFlagUseObjectPointer",
                "fSchemaFlagOutputToStderr",
                "fSchemaFlagOutputToStdout",
                "fSchemaFlagSkipLog",
            ]:
                sys.exit("schemaMeta/omniverseFlags element (" + f + ") is not a known flag name")
    except KeyError:
        flags = None

    try:
        client = meta["clientName"]
        if not isinstance(client, str) or len(client) == 0:
            sys.exit(
                "schemaMeta/clientName has invalid value '"
                + str(version)
                + "' - value must be a non-empty string (file = '"
                + in_file
                + "')"
            )
    except KeyError:
        sys.exit("no schemaMeta/clientName in the input schema (file = '" + in_file + "')")

    try:
        eventPrefix = meta["eventPrefix"]
    except KeyError:
        eventPrefix = None

    # we need to generate bindings for the JSON objects in definitions/events
    try:
        defs = blob["definitions"]
    except KeyError:
        sys.exit("no definitions list in the input schema (file = '" + in_file + "')")

    if not isinstance(defs, dict):
        sys.exit("schema element 'definitions' was not a dict (file = '" + in_file + "')")

    try:
        events = defs["events"]
    except KeyError:
        sys.exit("no definitions list in the input schema (file = '" + in_file + "')")

    if not isinstance(events, dict):
        sys.exit(
            "schema element 'definitions' contained an element 'events' that was not a dict (file = '" + in_file + "')"
        )

    if "oneOf" in blob:
        oneOf = blob["oneOf"]
    elif "anyOf" in blob:
        oneOf = blob["anyOf"]
    else:
        sys.exit("no oneOf/oneOf array in the input schema (file = '" + in_file + "')")
    if not isinstance(oneOf, list):
        sys.exit("oneOf is not an array (file = '" + in_file + "')")
    if len(oneOf) != len(events.items()):
        sys.exit(
            "oneOf has "
            + str(len(oneOf))
            + " elements, but there are "
            + str(len(events.items()))
            + " events (file = '"
            + in_file
            + "')"
        )

    # unpack the schema
    for d in events.items():
        try:
            unpacked.append(unpack_event(eventPrefix, d))
        except UnpackError as e:
            print("ERROR: " + str(e))
            errors += 1

    if errors > 0:
        sys.exit("encountered " + str(errors) + " errors while parsing (file = '" + in_file + "')")

    random.seed()

    def _write_output(out_file, code, out_type):
        filename = out_file + "." + str(random.randrange(2**24 - 1)) + ".tmp"

        # steps in this process:
        #   1. first write out the code to a temp file in the target folder.
        #   2. format the code so we can compare it to the existing file (if any) later.
        #   3. compare the formatted code to the file on disk if it exists.
        #   4. if the file matches, succeed.  If the file is different, write it out.
        try:
            # assume that we'll be replacing the file.  This makes it easier to handle the case of
            # the file not already existing on disk.
            replace = True

            # write out the code to a temporary file.
            with open(filename, "w") as output:
                output.write(code)

            # format the code.
            if out_type != "python":
                format_cpp(filename, out_file, repo_root)

            if os.path.isfile(out_file):
                # compare the generated, formatted file to the existing file.  If they match, skip
                # replacing the file to avoid potential write permission errors if the file is
                # already in use by another build step.
                if _compare_files(filename, out_file):
                    replace = False

            # replace the existing file (if any) if it's changed.  Remove it otherwise.
            if replace:
                # first make sure we haven't been asked to fail if this write needs to occur.
                # Note that we intentionally don't touch the output file here so that it will
                # continue to fail the build until the problem has been remedied.
                if args.fail_on_write:
                    print(
                        "    A code change was detected in the generated file '" + out_file + "' but\n"
                        "    the '--fail-on-write' option was used.  This likely means that code changes\n"
                        "    occurred during a CI build step because a full local build was not performed\n"
                        "    before creating an MR.  This can occur when the Carbonite SDK or repoman versions\n"
                        "    are updated and a tool change caused some code to be regenerated.\n"
                        "\n"
                        "    To remedy this, please run a full local build after updating the Carbonite SDK,\n"
                        "    then commit all modified .gen.h files to your MR.  This will happen from time to\n"
                        "    time when changes are made to the tools or code generation bugs need to be fixed.\n"
                        "    Note that a change only to a copyright year at the top of the file won't be considered\n"
                        "    a code change that will fail this operation.\n"
                    )
                    os.remove(filename)
                    return 3

                os.replace(filename, out_file)

            else:
                print("    no changes detected.  Not replacing '" + out_file + "'.")
                pathlib.Path(out_file).touch()
                os.remove(filename)

        except FileNotFoundError:
            print("path is not valid for the " + out_type + " output (file = '" + fileame + "')")
            return 1
        except PermissionError:
            print(
                "permission denied when opening the " + out_type + " output file for write (file = '" + fileame + "')"
            )
            return 2

        return 0

    # write the outputs
    if cpp_output is not None:
        code = generate_code(unpacked, client, version, flags, namespace)
        errors = _write_output(cpp_output, code, "C++ header")
        if errors > 0:
            sys.exit(errors)

    if py_output is not None:
        code = generate_py_code(unpacked, client, version, flags, blob)
        errors = _write_output(py_output, code, "python")
        if errors > 0:
            sys.exit(errors * 8)

    if py_binding_output is not None:
        code = generate_py_binding(unpacked, cpp_output, py_binding_output, client, version, namespace, flags)
        errors = _write_output(py_binding_output, code, "python binding")
        if errors > 0:
            sys.exit(errors * 16)
