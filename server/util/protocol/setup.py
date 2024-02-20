# By Tyler Clarke
# Builds C++ protocol frame definitions and source from manifest.json.
import json

def disclaim(file):
    file.write("""// AUTOGENERATED BY setup.py! DO NOT EDIT MANUALLY!
// By Tyler Clarke.
// This software is protected by GPLv3. That means you have to provide the source for anything that uses my code, and you have to give me credit.
// You can make money off this if you want, but that would be dumb.
""")

def argAdd(file, argument):
    if argument["packing"] == "static": # static packing is very simple. It doesn't require any extra logic; sizeof(type) is sufficient to write it into a buffer.
        # this is the standard packing style for all numbers.
        file.write(argument["type"] + " " + argument["name"] + ";\n")
    elif argument["packing"] == "vector": # vector packing is NOT sanely sized; it contains a pointer to the heap (or really just anywhere else).
        # vector-packed items also don't have sane typenames; like the "string" type will become "std::string" and such.
        if argument["type"] == "string":
            file.write("std::string " + argument["name"] + ";\n")

def argSize(argument):
    if argument["packing"] == "static":
        return "sizeof(" + argument["type"] + ")"
    elif argument["packing"] == "vector":
        if argument["type"] == "string":
            return "(" + argument["name"] + ".size() + (" + argument["name"] + ".size() < 255 ? 1 : 5))"

## TODO: convert to little-endian! I don't even know why we're using BE. screw it.
manifest = json.loads(open("manifest.json").read())
for key in manifest.keys():
    header = open(key + ".hpp", "w+")
    source = open(key + ".cpp", "w+")
    disclaim(header)
    header.write("\n// This is the declaration file for the " + key + " protocol.\n\n")
    disclaim(source)
    source.write("\n// This is the definition file for the " + key + " protocol.\n\n#include <util/protocol/" + key + ".hpp>\n")
    header.write("#pragma once\n#include <util/protocol/protocol.hpp>\nnamespace protocol::" + key + " {\n")
    for form in manifest[key]["formats"]:
        header.write("struct " + form["className"] + " : ProtocolFrameBase {\n")
        for argument in form["arguments"]:
            if not "packing" in argument:
                argument["packing"] = "static"
            argAdd(header, argument)
            header.write(form["className"] + "(char* data);\n")
        header.write("void load(char* buffer);\nsize_t getSize();\n")
        header.write("};\n")
        # constructor
        source.write("protocol::" + key + "::" + form["className"] + "::" + form["className"] + "(char* data) {\n")
        for argument in form["arguments"]:
            
        source.write("}")
        # load function
        source.write("void protocol::" + key + "::" + form["className"] + "::load(char* buffer) {\n")
        source.write("buffer[0] = " + str(form["opcode"]) + ";\nbuffer ++; // clever C hack: rather than worrying about current index, we can just consume a byte of the buffer.\n// This is very fast and makes life a lot easier.\n")
        for argument in form["arguments"]:
            if argument["packing"] == "static":
                source.write("for (uint8_t i = 0; i < sizeof(" + argument["type"] + "); i ++){buffer[i] = ((char*)&" + argument["name"] + ")[i];}\n")
                source.write("buffer += sizeof(" + argument["type"] + "); // see above\n")
            elif argument["packing"] == "vector":
                if argument["type"] == "string":
                    source.write("""buffer[0] = 's'; buffer ++;
size_t size = """ + argument["name"] + """.size();
if (size < 255) {
    buffer[0] = size; buffer++;
}
else {
    buffer[0] = 255;
    buffer ++;
    ((uint32_t*)&buffer)[0] = size;
    buffer += 4;
}
for (size_t i = 0; i < size; i ++) {buffer[i] = """ + argument["name"] + """[i];}
buffer += size;\n
""")
        source.write("}\n\n")
        source.write("size_t protocol::" + key + "::" + form["className"] + "::getSize() {\nreturn ")
        source.write("1") ## opcode byte
        for argument in form["arguments"]:
            source.write(" + ")
            source.write(argSize(argument))
        source.write(";\n}\n")
    header.write("}\n")