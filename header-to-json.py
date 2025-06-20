#!/usr/bin/env python

import json
import re
import sys


def main():
    fn = sys.argv[1]

    with open(fn, "rt") as fd:
        lines = fd.readlines()

    enums = {}
    typedefs = {}
    structs = {}
    interfaces = {}

    in_enum = False
    in_struct = False
    in_typedef = False
    in_interface = False

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

        if line.startswith("/*") or line.startswith("//"):
            description.append(line)
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
                    member["description"] = description[:]

                members.append(member)
                description.clear()

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
                    "type": member_type,
                    "name": member_name,
                }

                if len(description) > 0:
                    member["description"] = description[:]

                members.append(member)
                description.clear()

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
                        "members": members[:],
                    }
                    if len(parent_description) > 0:
                        data["description"] = parent_description[:]
                    enums[parent] = data
                    in_enum = False

                if in_struct:
                    data = {
                        "name": parent,
                        "members": members[:],
                    }
                    if len(parent_description) > 0:
                        data["description"] = parent_description[:]
                    structs[parent] = data
                    in_struct = False
            else:
                print("*** NO PARENT: ", line)

            parent = ""
            members.clear()
            parent_description.clear()
            description.clear()

    print(json.dumps(structs, indent=4))


if __name__ == "__main__":
    main()
