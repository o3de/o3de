########################################################################################
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
########################################################################################

import argparse
import getopt
import jinja2
import json
import string
import os

# Creates the folder at the given path is it doesn't already exist.
def CreateOutputFolder(path):
    folderPath = os.path.dirname(path)
    if not os.path.exists(folderPath):
        try:
            os.mkdir(folderPath)
        except OSError:
            print("Failed to create path '{0}'".format(folderPath))
            return False
    return True

# Function to check if a type is derived from one of the provided classes.
def IsDerivedFrom(context, classInfo, derivedType):
    bases = classInfo.get('Bases', [])
    for base in bases:
        baseUuid = base.get('Uuid')
        if baseUuid is not None:
            if baseUuid == derivedType:
                return True
            else:
                baseClassInfo = context.get(baseUuid)
                if baseClassInfo is not None:
                    if IsDerivedFrom(context, baseClassInfo, derivedType):
                        return True
    return False

# A Jinja2 filter function that filters any type that's not one of the provided types.
def FilterByBase(context, type):
    result = {}
    for uuid, classInfo in context.items():
        if uuid == type or IsDerivedFrom(context, classInfo, type):
            result[uuid] = classInfo
    return result
    
def ParseAdditionalArguments(dict, arguments):
    currentArg = None
    for arg in arguments:
        if arg[:2] == '--' or arg[:1] == '-':
            if currentArg is not None:
                dict[currentArg] = True

            if arg[:2] == '--':
                currentArg = arg[2:]
            elif arg[:1] == '-':
                currentArg = arg[1:]
        else:
            if currentArg is not None:
                dict[currentArg] = arg
                currentArg = None
    if currentArg is not None:
        dict[currentArg] = True

# Convert the json formatted Serialize Context through the provided Jinja2 template.
def Convert(source, output, template, split, additionalArgs):
    with open(source) as jsonFile:  
        data = json.load(jsonFile)
    
    jinjaEnvironment = jinja2.Environment(loader=jinja2.FileSystemLoader(os.path.dirname(template)), trim_blocks=True, lstrip_blocks=True)
    jinjaEnvironment.filters['FilterByBase'] = FilterByBase
    renderer = jinjaEnvironment.get_template(os.path.basename(template))
    
    path, extension = os.path.splitext(output);
    file = os.path.basename(path)
    
    data['Config'] = {}
    data['Config']['Document'] = file
    ParseAdditionalArguments(data['Config'], additionalArgs)
    print("Template arguments: {}".format(data['Config']))

    if split:
        for c in string.ascii_lowercase:
            data['Config']['Filter'] = c
            outputFilePath = "{0} {1}{2}".format(path, c, extension)
            rendering = renderer.render(data)
            with open(outputFilePath, 'w') as outputFile:
                outputFile.write(rendering)
    else:
        rendering = renderer.render(data)
        with open(output, 'w') as outputFile:
            outputFile.write(rendering)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Takes a json-formatted dump of the Serialize Context and runs it through the provided Jinja2 templates.')
    parser.add_argument('--source', help='Path to the json file containing the output of the SerializeContext.', required=True)
    parser.add_argument('--output', help='Path to the file to write the converted result to.', required=True)
    parser.add_argument('--template', help='Path to the Jinja2 template to use for the conversion.', required=True)
    parser.add_argument('--split', help='Only include entries that start with this letter.', action='store_true')
    args, remainingArgs = parser.parse_known_args()

    sourcePath = args.source
    if not os.path.isabs(sourcePath):
        sourcePath = os.path.abspath(sourcePath)
        
    outputPath = args.output
    if not os.path.isabs(outputPath):
        outputPath = os.path.abspath(outputPath)

    templatePath = args.template
    if not os.path.isabs(templatePath):
        templatePath = os.path.abspath(templatePath)

    if CreateOutputFolder(outputPath):
        print('Converting from "{0}" to "{1}" using template "{2}".'.format(sourcePath, outputPath, templatePath))
        Convert(sourcePath, outputPath, templatePath, args.split, remainingArgs)
