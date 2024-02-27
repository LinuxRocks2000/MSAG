## convert a JSON object to a C header file.
## useful for dealing with definitions that need to be shared to a high-level language (like JavaScript)
## When dealing with nested objects, it just prepends, so {"TEST" : 3, "TEST2": {"TEST": 4}} would yield
## #define TEST 3
## #define TEST2_TEST 4
import json, sys


def treewrite(file, jsondata, prep = ""):
    for key in jsondata.keys():
        if type(jsondata[key]) == dict:
            treewrite(file, jsondata[key], prep + key + "_")
        else:
            file.write("#define " + prep + key + " " + str(jsondata[key]) + "\n")


if len(sys.argv) >= 3:
    inputs = open(sys.argv[1])
    outputs = open(sys.argv[2], "w+")
    jsondata = json.loads(inputs.read())
    inputs.close() # usually a good practice
    x = 3
    prefix = ""
    while x < len(sys.argv):
        if sys.argv[x] == '-p' or sys.argv[x] == '--prefix':
            x += 1
            prefix = sys.argv[x] + "_"
        x += 1
    treewrite(outputs, jsondata, prep = prefix)
else:
    print("Invalid arguments. Usage: json2header [source] [target].")