#!/usr/bin/env python

import json
import re
import sys

# TODO:
#
# - Parse descriptions into normal text
#   - Decide on format for multiple line strings
# - Parse function pointer arguments
# - Parse interface doc comments into structured data (including argument docs)
# - Include copyright info at the top of the file


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

        if line == "/* VARIANT TYPES */":
            description.clear()

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
                    "name": m.group(1),
                }

                member_value = m.group(2)
                if member_value:
                    member["value"] = int(member_value[3:])
                else:
                    member["value"] = len(members)

                if len(description) > 0:
                    member["doc"] = description[:]
                    description.clear()

                members.append(member)

        # Structs.
        if line.startswith("typedef struct {"):
            in_struct = True
            parent_description = description[:]
            description.clear()
        elif in_struct:
            m = re.match(r"^([a-zA-Z0-9_]+) (\S+);(?:\s+/[/*](.*))?$", line)
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
                    member["doc"] = description[:]
                    description.clear()

                members.append(member)

        # Typedefs.
        if line.startswith("typedef "):
            m = re.match(r"^typedef ([^(]*)\(\*([a-zA-Z0-9]*)\)\(([^)]*)\);", line)
            if m:
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

                fp["ret"] = m.group(1).strip()
                fp["args"] = m.group(3).strip()

                if len(description) > 0:
                    fp["doc"] = description[:]
                    description.clear()

                if interface_name:
                    # Remove this if we can!
                    fp["ctype"] = m.group(2)
                    # @todo Parse the doc string
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
                        "def": m.group(1),
                    }

                    if len(description) > 0:
                        stype["doc"] = description[:]
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
                        data["doc"] = parent_description[:]
                    types.append(data)
                    in_enum = False

                if in_struct:
                    data = {
                        "name": parent,
                        "type": "struct",
                        "members": members[:],
                    }
                    if len(parent_description) > 0:
                        data["doc"] = parent_description[:]
                    types.append(data)
                    in_struct = False
            else:
                print("*** NO PARENT: ", line)

            parent = ""
            members.clear()
            parent_description.clear()
            description.clear()

    api = {
        "types": types,
        "interface": interfaces,
    }

    print(json.dumps(api, indent=4))


if __name__ == "__main__":
    main()
