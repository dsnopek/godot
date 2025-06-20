import json

import methods


def run(target, source, env):
    buffer = methods.get_buffer(str(source[0]))
    data = json.loads(buffer)

    with methods.generated_wrapper(str(target[0])) as file:
        file.write("""\
#ifndef __cplusplus
#include <stddef.h>
#include <stdint.h>

typedef uint32_t char32_t;
typedef uint16_t char16_t;
#else
#include <cstddef>
#include <cstdint>

extern "C" {
#endif

""")

        for type in data["types"]:
            if "doc" in type:
                write_doc(file, type["doc"])
            if type["type"] == "simple":
                write_simple_type(file, type)
            elif type["type"] == "enum":
                write_enum_type(file, type)
            elif type["type"] == "function":
                write_function_type(file, type)
            elif type["type"] == "struct":
                write_struct_type(file, type)
            else:
                raise Exception(f"Unknown type: {type['type']}")

        for interface in data["interface"]:
            write_interface(file, interface)

        file.write("""\
#ifdef __cplusplus
}
#endif
""")


def write_doc(file, doc, indent=""):
    if len(doc) == 1:
        file.write(f"{indent}/* {doc[0]} */\n")
        return

    first = True
    for line in doc:
        if first:
            file.write(indent + "/* ")
            first = False
        else:
            file.write(indent + " * ")

        file.write(line)
        file.write("\n")
    file.write(indent + " */\n")


def make_deprecated_note(type):
    if "deprecated" not in type:
        return ""
    return f" /* {type['deprecated']} */"


def write_simple_type(file, type):
    file.write(f"typedef {type['def']}")
    if type["def"][-1] != "*":
        file.write(" ")
    file.write(f"{type['name']};{make_deprecated_note(type)}\n")


def write_enum_type(file, enum):
    file.write("typedef enum {\n")
    for member in enum["members"]:
        if "doc" in member:
            write_doc(file, member["doc"], "\t")
        file.write(f"\t{member['name']} = {member['value']},\n")
    file.write(f"}} {enum['name']};{make_deprecated_note(enum)}\n\n")


def make_args_text(args):
    combined = []
    for arg in args:
        arg_text = arg["type"]
        if "name" in arg:
            if arg["type"][-1] != "*":
                arg_text += " "
            arg_text += arg["name"]
        combined.append(arg_text)
    return ", ".join(combined)


def write_function_type(file, fn):
    file.write(f"typedef {fn['ret']['type']}")
    if fn["ret"]["type"][-1] != "*":
        file.write(" ")
    args_text = make_args_text(fn["args"]) if ("args" in fn) else ""
    file.write(f"(*{fn['name']})({args_text});{make_deprecated_note(fn)}\n")


def write_struct_type(file, struct):
    file.write("typedef struct {\n")
    for member in struct["members"]:
        if "doc" in member:
            write_doc(file, member["doc"], "\t")
        file.write(f"\t{member['type']}")
        if member["type"][-1] != "*":
            file.write(" ")
        file.write(f"{member['name']};\n")

    file.write(f"}} {struct['name']};{make_deprecated_note(struct)}\n\n")


def write_interface(file, interface):
    doc = [
        f"@name {interface['name']}",
        f"@since {interface['since']}",
    ]

    if "deprecated" in interface:
        if interface["deprecated"].lower().startswith("deprecated "):
            parts = interface["deprecated"].split(" ", 1)
            interface["deprecated"] = parts[1]
        doc.append(f"@deprecated {interface['deprecated']}")

    doc += [
        "",
        interface["doc"][0],
    ]

    if len(interface["doc"]) > 1:
        doc.append("")
        doc += interface["doc"][1:]

    if "args" in interface:
        doc.append("")
        for arg in interface["args"]:
            if "doc" not in arg:
                raise Exception(f"Interface function {interface['name']} is missing docs for {arg['name']} argument")
            arg_doc = " ".join(arg["doc"])
            doc.append(f"@param {arg['name']} {arg_doc}")

    if "ret" in interface and interface["ret"]["type"] != "void":
        if "doc" not in interface["ret"]:
            raise Exception(f"Interface function {interface['name']} is missing docs for return value")
        ret_doc = " ".join(interface["ret"]["doc"])
        doc.append("")
        doc.append(f"@return {ret_doc}")

    if "see" in interface:
        doc.append("")
        for see in interface["see"]:
            doc.append(f"@see {see}")

    file.write("/**\n")
    for d in doc:
        file.write(f" * {d}\n")
    file.write(" */\n")

    fn = interface.copy()
    if "deprecated" in fn:
        del fn["deprecated"]
    fn["name"] = "GDExtensionInterface" + "".join(word.capitalize() for word in interface["name"].split("_"))
    write_function_type(file, fn)

    file.write("\n")
