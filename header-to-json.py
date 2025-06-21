#!/usr/bin/env python

import json
import re
import sys

COPYRIGHT = """
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/
"""

# TODO:
#
# - Parse "deprecated" in docs and mark structs or whatever with it
# - Should we remove p_ and r_ (and add a `is_return` to mark the returns?)


def clean_empty_strings(list_of_strings):
    out = []

    prev_was_empty = False
    for s in list_of_strings:
        if s == "":
            if not prev_was_empty:
                out.append(s)
                prev_was_empty = True
        else:
            out.append(s)
            prev_was_empty = False

    while len(out) > 0 and out[0] == "":
        out = out[1:]
    while len(out) > 0 and out[-1] == "":
        out = out[:-1]

    return out


def clean_description(description):
    out = []

    for line in description:
        if line.startswith("//") or line.startswith("/*"):
            line = line[2:]
        if line.endswith("*/"):
            line = line[:-2]
        if line.startswith("*"):
            line = line[1:]
        line = line.strip()

        out.append(line)

    out = clean_empty_strings(out)

    return out


def parse_args(text):
    args = []

    parts = filter(lambda x: x != "", map(lambda x: x.strip(), text.split(",")))
    for part in parts:
        words = list(filter(lambda x: x != "", map(lambda x: x.strip(), part.split(" "))))
        if len(words) > 2:
            words = [" ".join(words[:-1]), words[-1]]
        if len(words) == 2 and words[1][0] == "*":
            words[0] += " *"
            words[1] = words[1][1:]
        if len(words) == 0:
            print("Unable to parse arg: ", part)

        arg = {}
        if len(words) == 2:
            arg["name"] = words[1]
        arg["type"] = words[0]

        args.append(arg)

    return args


def update_interface_from_doc(interface):
    out = []

    for line in interface["doc"]:
        if line.startswith("@name"):
            # We've already pulled this in.
            continue

        if line.startswith("@since"):
            parts = line.split(" ", 1)
            interface["since"] = parts[1]
            continue

        if line.startswith("@deprecated"):
            parts = line.split(" ", 1)
            interface["deprecated"] = "Deprecated " + parts[1]
            continue

        if line.startswith("@param"):
            parts = line.split(" ", 2)
            arg_name = parts[1]
            arg_desc = parts[2]
            arg_found = False
            for arg in interface["args"]:
                if arg["name"] == arg_name:
                    arg_found = True
                    arg["doc"] = [arg_desc]
            if not arg_found:
                print("*** In", interface["name"], "- unable to find arg:", arg_name)
            continue

        if line.startswith("@return"):
            parts = line.split(" ", 1)
            if len(parts) > 1 and "ret" in interface:
                interface["ret"]["doc"] = [parts[1]]
            else:
                print("*** In", interface["name"], "- unable to find return value!")
            continue

        if line.startswith("@note"):
            parts = line.split(" ", 1)
            line = parts[1]

        out.append(line)

    out = clean_empty_strings(out)

    # We have a single line separating the brief from the long description. We can remove that in the JSON.
    if len(out) >= 2 and out[1] == "":
        del out[1]

    interface["doc"] = out


def main():
    fn = sys.argv[1]

    with open(fn, "rt") as fd:
        lines = fd.readlines()

    types = []
    interfaces = []

    in_enum = False
    in_struct = False
    in_comment = False

    parent = ""
    parent_description = []
    description = []
    members = []

    for line in lines:
        line = line.strip()

        if line == "":
            continue
        if line.startswith("#"):
            continue
        if line.startswith("extern"):
            continue

        if line == "/* VARIANT TYPES */":
            description.clear()

        if line.startswith("//"):
            description.append(line)
        elif line.startswith("/*"):
            description.append(line)
            if "*/" not in line:
                in_comment = True
        elif "*/" in line:
            description.append(line)
            in_comment = False
        elif in_comment:
            description.append(line)
            continue

        # Enums.
        if line.startswith("typedef enum {"):
            in_enum = True
            parent_description = description[:]
            description.clear()
        elif in_enum:
            m = re.match(r"^(\S+)( = \d+)?,?(?:\s+/[/*](.*))?$", line)
            if m:
                inline_doc = m.group(3)
                if inline_doc:
                    description.append(inline_doc.strip())

                member = {
                    "name": m.group(1).rstrip(",").strip(),
                }

                member_value = m.group(2)
                if member_value:
                    member["value"] = int(member_value[3:])
                else:
                    member["value"] = len(members)

                if len(description) > 0:
                    member["doc"] = clean_description(description)
                    description.clear()

                members.append(member)

        # Structs.
        if line.startswith("typedef struct {"):
            in_struct = True
            parent_description = description[:]
            description.clear()
        elif in_struct:
            m = re.match(r"^([a-zA-Z0-9_ ]+) (\S+);(?:\s+/[/*](.*))?$", line)
            if m:
                inline_doc = m.group(3)
                if inline_doc:
                    description.append(inline_doc.strip())

                member_type = m.group(1)
                member_name = m.group(2)

                # Handle pointer types.
                if member_name.startswith("*"):
                    member_type += " "
                    while member_name.startswith("*"):
                        member_type += "*"
                        member_name = member_name[1:]

                member = {
                    "name": member_name,
                    "type": member_type,
                }

                if len(description) > 0:
                    member["doc"] = clean_description(description)
                    description.clear()

                members.append(member)

        # Typedefs.
        if line.startswith("typedef "):
            m = re.match(r"^typedef ([^(]*)\(\*([a-zA-Z0-9]*)\)\(([^)]*)\);(?:\s+/[/*](.*))?$", line)
            if m:
                inline_doc = m.group(4)
                if inline_doc:
                    description.append(inline_doc.strip())

                fp = {}

                interface_name = None
                if len(description) > 0:
                    for d in description:
                        if "@name" in d:
                            interface_name = d.split("@name")[1].strip()
                            break
                if interface_name:
                    fp["name"] = interface_name
                else:
                    fp["name"] = m.group(2)
                    fp["type"] = "function"

                fp["ret"] = {
                    "type": m.group(1).strip(),
                }
                fp["args"] = parse_args(m.group(3).strip())

                if len(description) > 0:
                    fp["doc"] = clean_description(description)
                    description.clear()

                if interface_name:
                    update_interface_from_doc(fp)

                    ctype_name = "GDExtensionInterface" + "".join(
                        word.capitalize() for word in interface_name.split("_")
                    )
                    if ctype_name != m.group(2):
                        fp["legacy_type_name"] = m.group(2)

                    interfaces.append(fp)
                else:
                    fp["name"] = m.group(2)
                    types.append(fp)

            else:
                m = re.match(r"^typedef (.*[ \*]) ?([a-zA-Z0-9]*);(?:\s+/[/*](.*))?$", line)
                if m:
                    inline_doc = m.group(3)
                    if inline_doc:
                        description.append(inline_doc.strip())

                    stype = {
                        "name": m.group(2),
                        "type": "simple",
                        "def": m.group(1).strip(),
                    }

                    if len(description) > 0:
                        stype["doc"] = clean_description(description)
                        description.clear()

                    types.append(stype)

        # Closing curly brace.
        if line.startswith("}"):
            m = re.match(r"^\} (\S+);(?:\s+/[/*](.*))?$", line)
            if m:
                parent = m.group(1)
                inline_doc = m.group(2)
                if inline_doc:
                    parent_description.append(inline_doc.strip())

            if parent:
                if in_enum:
                    data = {
                        "name": parent,
                        "type": "enum",
                        "members": members[:],
                    }
                    if len(parent_description) > 0:
                        data["doc"] = clean_description(parent_description)
                    types.append(data)
                    in_enum = False

                if in_struct:
                    data = {
                        "name": parent,
                        "type": "struct",
                        "members": members[:],
                    }
                    if len(parent_description) > 0:
                        data["doc"] = clean_description(parent_description)
                    types.append(data)
                    in_struct = False
            else:
                # print("*** NO PARENT: ", line)
                pass

            parent = ""
            members.clear()
            parent_description.clear()
            description.clear()

    copyright = list(filter(lambda x: x != "", map(lambda x: x.strip(), COPYRIGHT.split("\n"))))

    api = {
        "_copyright": copyright,
        "format_version": 1,
        "types": types,
        "interface": interfaces,
    }

    print(json.dumps(api, indent=4))


if __name__ == "__main__":
    main()
