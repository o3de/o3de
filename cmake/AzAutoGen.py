#!/usr/bin/python

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

import io
import os
import re
import sys
import time
import errno
import shutil
import fnmatch
import filecmp
import fileinput
import importlib
import argparse
import hashlib
from xml.sax.saxutils import escape, unescape, quoteattr

# Maximum number of errors before bailing on AutoGen
MAX_ERRORS = 100
errorCount = 0

def PrintError(*objs):
    print(*objs, file=sys.stderr)
    global errorCount
    errorCount += 1
    if errorCount > MAX_ERRORS:
        print("Maximum errors exceeded (%d) please check the tty for errors" % MAX_ERRORS, file=sys.stderr)
        sys.exit(1)

def PrintUnhandledExcptionInfo():
    print("An unexpected error occurred, please report the error you encountered and include your build output", file=sys.stderr)

def TransformEscape(string):
    return escape(quoteattr(unescape(string)))

def BooleanTrue(string):
    testString = string.lower().strip()
    return testString == "true" or testString == "1"

def CamelToHuman(string):
    return string[0].upper() + re.sub(r'((?<=[a-z])[A-Z]|(?<!\A)[A-Z](?=[a-z]))', r' \1', string[1:])

def StripFloat(string):
    return re.sub(r'(\d+(\.\d*)?|\.\d+)f', r'\g<1>0', string)

def CreateHashGuid(string):
    hash = hashlib.new('md5')
    hash.update(string.encode('utf-8'))
    hashStr = hash.hexdigest()
    return ("{" + hashStr[0:8] + "-" + hashStr[8:12] + "-" + hashStr[12:16] + "-" + hashStr[16:20] + "-" + hashStr[20:] + "}").upper()

def EtreeToString(xmlNode):
    return etree.tostring(xmlNode)

def SanitizePath(path):
    return (path or '').replace('\\', '/').replace('//', '/')

def SearchPaths(filename, paths=[]):
    if len(paths) > 0:
        for path in paths:
            testFile = os.path.join(path, filename)
            if os.path.exists(testFile):
                return os.path.abspath(testFile)
    if os.path.exists(filename):
        return os.path.abspath(filename)
    return None

def ComputeOutputPath(inputFiles, projectDir, outputDir):
    commonInputPath = os.path.commonpath(inputFiles) # If we've globbed many source files, this finds the common path
    if os.path.isfile(commonInputPath): # If the commonInputPath resolves to an actual file, slice off the filename
        commonInputPath = os.path.dirname(commonInputPath)
    commonPath = os.path.commonpath([commonInputPath, projectDir]) # Finds the common path between the data source files and our project directory (//depot/dev/Code/Framework/AzCore/)
    inputRelativePath = os.path.relpath(commonInputPath, commonPath) # Computes the relative path for the project source directory (Code/Framework/AzCore/AutoGen/)
    return os.path.join(outputDir, inputRelativePath) # Returns a suitable output directory (//depot/dev/Generated/Code/Framework/AzCore/AutoGen/)

def ProcessTemplateConversion(dataInputSet, dataInputFiles, templateFile, outputFile, templateCache, dryrun, verbose):
    if dryrun or not dataInputFiles:
        return
    try:
        outputFile = os.path.abspath(outputFile)
        outputPath = os.path.dirname(outputFile)
        treeRoots = []
        for dataInputFile in sorted(dataInputFiles):
            try:
                if dataInputFile in dataInputSet.keys():
                    treeRoots.append(dataInputSet.get(dataInputFile))
                elif os.path.splitext(dataInputFile)[1] == ".xml":
                    xml = etree.parse(dataInputFile)
#                    xml.xinclude()
                    xmlroot = xml.getroot()
                    # look for an xml schema link for this document
#                    xmlSchema = None
#                    if 'xsi' in xmlroot.nsmap:
#                        XMLSchemaNamespace = xmlroot.nsmap['xsi']
#                        schemaLink = xmlroot.get('{' + XMLSchemaNamespace + '}schemaLocation')
#                        if schemaLink is None:
#                            schemaLink = xmlroot.attrib['{' + XMLSchemaNamespace + '}noNamespaceSchemaLocation']
#                        if schemaLink:
#                            # if we have a schemaLink, then we need to strip off the relative pathing and use our search paths
#                            # relative pathing on the xml file itself is purely a nicety for Visual Studio to find the correct XSD for inline validation
#                            xmlSchema = os.path.basename(schemaLink)
#                    if xmlSchema:
#                        # check the template directory, the template include dir, and the folder that houses the nvdef file, and the xml's location for the xsd
#                        searchPaths = [os.path.dirname(templateFile)]
#                        searchPaths += [os.path.dirname(dataInputFile)] 
#                        xmlShemaLoc = SearchPaths(xmlSchema, searchPaths)
#                        try:
#                            xmlSchemaDoc = etree.parse(xmlShemaLoc)
#                            xmlSchemaObj = etree.XMLSchema(xmlSchemaDoc, attribute_defaults=True)
#                            xmlSchemaObj.assertValid(xmlroot)
#                        except etree.DocumentInvalid as e:
#                            for error in e.error_log:
#                                PrintError('%s(%d) : error InvalidXML %s' % (os.path.abspath(dataInputFile), error.line, error.message))
#                        except IOError as e:
#                            PrintError('%s(%s) : %s' % (os.path.abspath(dataInputFile), str(1), e.message))
                    xmlroot = xml.getroot()
                    dataInputSet[dataInputFile] = xml.getroot()
                    treeRoots.append(xml.getroot())
                else:
                    with open(dataInputFile) as jsonFile:
                        jsonData = json.load(jsonFile)
                        dataInputSet[dataInputFile] = jsonData
                        treeRoots.append(jsonData)
            except IOError as e:
                PrintError('%s(%s) : %s' % (fileinput.filename(), str(fileinput.filelineno()), e.message))
#            except etree.XMLSyntaxError as e:
#                for error in e.error_log:
#                    PrintError('%s(%s) : error XMLSyntaxError %s' % (os.path.abspath(dataInputFile), error.line, error.message))
        compareFD = io.StringIO()

        searchPaths = [os.path.dirname(templateFile)]
        templateLoader = jinja2.FileSystemLoader(searchpath = searchPaths)
        templateEnv    = jinja2.Environment(bytecode_cache = templateCache, loader = templateLoader, trim_blocks = True, extensions = ["jinja2.ext.do",])
        templateEnv.filters['relpath'       ] = lambda x: os.path.relpath(x, outputPath)
        templateEnv.filters['dirname'       ] = os.path.dirname
        templateEnv.filters['basename'      ] = os.path.basename
        templateEnv.filters['splitext'      ] = os.path.splitext
        templateEnv.filters['split'         ] = os.path.split
        templateEnv.filters['startswith'    ] = str.startswith
        templateEnv.filters['int'           ] = int
        templateEnv.filters['str'           ] = str
        templateEnv.filters['escape'        ] = TransformEscape
        templateEnv.filters['len'           ] = len
        templateEnv.filters['range'         ] = range
        templateEnv.filters['stripFloat'    ] = StripFloat
        templateEnv.filters['camelToHuman'  ] = CamelToHuman
        templateEnv.filters['booleanTrue'   ] = BooleanTrue
        templateEnv.filters['createHashGuid'] = CreateHashGuid
        templateEnv.filters['etreeToString' ] = EtreeToString
        templateJinja  = templateEnv.get_template(os.path.basename(templateFile))
        templateVars   = \
            { \
                "dataFiles"     : treeRoots, \
                "dataFileNames" : dataInputFiles, \
                "templateName"  : templateFile, \
                "outputFile"    : outputFile, \
                "filename"      : os.path.splitext(os.path.basename(outputFile))[0], \
            }
        try:
            outputExtension = os.path.splitext(outputFile)[1]
            if outputExtension == ".xml" or outputExtension == ".xhtml" or outputExtension == ".xsd":
                compareFD.write('<?xml version="1.0"?>\n')
                compareFD.write('<!-- Copyright (c) Contributors to the Open 3D Engine Project.                                         -->\n')
                compareFD.write('<!-- For complete copyright and license terms please see the LICENSE at the root of this distribution. -->\n')
                compareFD.write('\n')
                compareFD.write('<!-- SPDX-License-Identifier: Apache-2.0 OR MIT                                -->\n')
                compareFD.write('\n')
                compareFD.write('<!-- This file is generated automatically at compile time, DO NOT EDIT BY HAND -->\n')
                compareFD.write('<!-- Template Source {0}; XML Sources {1}-->\n'.format(templateFile, ', '.join(dataInputFiles)))
                compareFD.write('\n')
            elif outputExtension == ".lua":
                compareFD.write('-- Copyright (c) Contributors to the Open 3D Engine Project.\n')
                compareFD.write('-- For complete copyright and license terms please see the LICENSE at the root of this distribution.\n')
                compareFD.write('\n')
                compareFD.write('-- SPDX-License-Identifier: Apache-2.0 OR MIT\n')
                compareFD.write('\n')
                compareFD.write('-- This file is generated automatically at compile time, DO NOT EDIT BY HAND\n')
                compareFD.write('-- Template Source {0}; XML Sources {1}\n'.format(templateFile, ', '.join(dataInputFiles)))
                compareFD.write('\n')
            elif outputExtension == ".h" or outputExtension == ".hpp" or outputExtension == ".inl" or outputExtension == ".c" or outputExtension == ".cpp":
                compareFD.write('/*\n')
                compareFD.write(' * Copyright (c) Contributors to the Open 3D Engine Project.\n')
                compareFD.write(' * For complete copyright and license terms please see the LICENSE at the root of this distribution.\n')
                compareFD.write(' *\n')
                compareFD.write(' * SPDX-License-Identifier: Apache-2.0 OR MIT\n')
                compareFD.write(' *\n')
                compareFD.write(' * This file is generated automatically at compile time, DO NOT EDIT BY HAND\n')
                compareFD.write(' * Template Source {0}; Data Sources {1}\n'.format(templateFile, ', '.join(dataInputFiles)))
                compareFD.write(' */\n')
                compareFD.write('\n')
            compareFD.write(templateJinja.render(templateVars))
            compareFD.write('\n')
        except jinja2.exceptions.TemplateNotFound as e:
            PrintError('%s(1) : error TemplateNotFound %s' % (os.path.abspath(templateFile), e.message))
        except IOError as e:
            PrintError('%s(%s) : error I/O(%s) accessing %s : %s' % (fileinput.filename(), str(fileinput.filelineno()), e.errno, e.filename, e.strerror))
    except jinja2.exceptions.TemplateSyntaxError as e:
        PrintError('%s(%s) : error Template processing error: %s' % (os.path.abspath(e.filename), e.lineno, e.message))
    except jinja2.exceptions.UndefinedError as e:
        # Sadly, jinja doesn't provide the exact line of the template that had this error since the template is compiled directly to python code
        PrintError('%s(1) : error Template processing error: %s with %s' % (os.path.abspath(templateFile), e.message, ', '.join([os.path.basename(dataInputFile) for dataInputFile in dataInputFiles])))
    try:
        os.makedirs(os.path.dirname(outputFile))
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise
    try:
        if os.path.isfile(outputFile):
            with open(outputFile, 'r+') as currentFile:
                currentFileStringData = currentFile.read()
                if currentFileStringData == compareFD.getvalue():
                    if verbose == True:
                        print('Generated file %s is unchanged, skipping' % (outputFile))                    
                else:
                    currentFile.truncate()
                    with open(outputFile, 'w+') as currentFile:
                        currentFile.write(compareFD.getvalue())
                        print('Generating %s with template %s and inputs %s' % (outputFile, templateFile, ", ".join(dataInputFiles)))
        else:
            with open(outputFile, 'w+') as outputFD:
                outputFD.write(compareFD.getvalue())                
                print('Generating %s using template %s and inputs %s' % (outputFile, templateFile, ", ".join(dataInputFiles)))
    except IOError as e:
        PrintError('%s(%s) : error I/O(%s) accessing %s : %s' % (fileinput.filename(), str(fileinput.filelineno()), e.errno, e.filename, e.strerror))
    except:
        PrintError('%s(%s) : error Processing: %s' % (fileinput.filename(), str(fileinput.filelineno()), line))
        PrintUnhandledExcptionInfo()
        raise
    compareFD.close()

def ProcessExpansionRule(sourceFiles, templateFiles, templateCache, outputDir, projectDir, expansionRule, dryrun, verbose, dataInputSet, outputFiles):
    try:
        # should be of the format inputFile(s),templateFile,outputFile, where inputFile and outputFile are subject to wildcarding and substitutions
        expansionRuleSet = expansionRule.split(",")
        inputFiles = expansionRuleSet[0]
        templateFile = None
        outputFile = expansionRuleSet[2]
        for fullPathTemplate in templateFiles:
            if expansionRuleSet[1] in fullPathTemplate:
                templateFile = fullPathTemplate
                break
        if templateFile is None:
            print("No matching template file found for %s, template may be missing from your _files.cmake" % expansionRuleSet[1])
            return
        # We have a few potential modes of input to output mapping that we'll have to handle depending on how the user formatted their azdef expansion rule
        # if the data input file was explicit
        #    then output a single file for that explicit data
        # else the data is wildcarded
        #    if the output contains $file or $fileprefix
        #       then we can generate a *unique* name for each data input, we're in one-to-one mapping mode, create a unique output for each input
        #    else if the output contains $path
        #       then we can generate a unique name for each *directory* of data inputs, we're in many-to-one mapping mode, create a unique output for each directory
        #    else the output is explicit, not wildcarded
        #       generate a single output file containing all matching data file's
        #    endif
        # endif
        testSingle = os.path.join(projectDir, inputFiles)
        if os.path.isfile(testSingle):
            # If we specified an *explicit* file to be processed (no wildcards for the data input file foo.json not *.foo.json), this is the branch that handles this case
            # This is explicitly one-to-one mapping
            dataInputFiles = [os.path.abspath(testSingle)]
            outputFileAbsolute = outputFile.replace("$path", ComputeOutputPath(dataInputFiles, projectDir, outputDir))
            outputFileAbsolute = outputFileAbsolute.replace("$fileprefix", os.path.splitext(os.path.basename(testSingle))[0].split(".")[0])
            outputFileAbsolute = outputFileAbsolute.replace("$file", os.path.splitext(os.path.basename(testSingle))[0])
            outputFileAbsolute = SanitizePath(outputFileAbsolute)
            ProcessTemplateConversion(dataInputSet, dataInputFiles, templateFile, outputFileAbsolute, templateCache, dryrun, verbose)
            outputFiles.append(outputFileAbsolute)
        else:
            # We've wildcarded the data input field, so we may have to handle one-to-one mapping of data files to output, or many-to-one mapping of data files to output
            if "$fileprefix" in outputFile or "$file" in outputFile:
                # Due to the wildcards in the output file, we've determined we'll do a one-to-one mapping of data files to output
                for filename in fnmatch.filter(sourceFiles, inputFiles):
                    dataInputFiles = [os.path.abspath(filename)]
                    outputFileAbsolute = outputFile.replace("$path", ComputeOutputPath(dataInputFiles, projectDir, outputDir))
                    outputFileAbsolute = outputFileAbsolute.replace("$fileprefix", os.path.splitext(os.path.basename(filename))[0].split(".")[0])
                    outputFileAbsolute = outputFileAbsolute.replace("$file", os.path.splitext(os.path.basename(filename))[0])
                    outputFileAbsolute = SanitizePath(outputFileAbsolute)
                    ProcessTemplateConversion(dataInputSet, dataInputFiles, templateFile, outputFileAbsolute, templateCache, dryrun, verbose)
                    outputFiles.append(outputFileAbsolute)
            else:
                # Process all matches in one batch
                # Due to the lack of wildcards in the output file, we've determined we'll glob all matching input files into the template conversion 
                for filename in fnmatch.filter(sourceFiles, inputFiles):
                    dataInputFiles = [os.path.abspath(file) for file in fnmatch.filter(sourceFiles, inputFiles)]
                outputFileAbsolute = outputFile.replace("$path", ComputeOutputPath(dataInputFiles, projectDir, outputDir))
                outputFileAbsolute = SanitizePath(outputFileAbsolute)
                ProcessTemplateConversion(dataInputSet, dataInputFiles, templateFile, outputFileAbsolute, templateCache, dryrun, verbose)
                outputFiles.append(outputFileAbsolute)
    except IOError as e:
        PrintError('%s : error I/O(%s) accessing %s : %s' % (expansionRule, e.errno, e.filename, e.strerror))
    except:
        PrintError('%s : error Processing expansion rule' % expansionRule)
        PrintUnhandledExcptionInfo()
        raise

def ExecuteExpansionRules(cacheDir, outputDir, projectDir, inputFiles, expansionRules, dryrun, verbose, dataInputSet, outputFiles):
    # Get Globals
    global MAX_ERRORS
    global errorCount
    currentPath = os.getcwd()
    startTime = time.time()
    # Ensure jinja2 template cache dir actually exists...
    try:
        os.makedirs(cacheDir)
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise
    sourceFiles = []
    templateFiles = []
    for inputFile in inputFiles:
        if inputFile.endswith(".xml") or inputFile.endswith(".json"):
            sourceFiles.append(os.path.join(projectDir, inputFile))
        elif inputFile.endswith(".jinja"):
            templateFiles.append(os.path.join(projectDir, inputFile))
    templateCache = jinja2.FileSystemBytecodeCache(cacheDir)
    for expansionRule in expansionRules:
        ProcessExpansionRule(sourceFiles, templateFiles, templateCache, outputDir, projectDir, expansionRule, dryrun, verbose, dataInputSet, outputFiles)
    if not dryrun:
        elapsedTime = time.time() - startTime
        millis = int(round(elapsedTime * 10))
        m, s = divmod(elapsedTime, 60)
        h, m = divmod(m, 60)    
        print('Total Time %d:%02d:%02d.%02d' % (h, m, s, millis))
    # Return true on success
    return errorCount == 0

# Main Function
if __name__ == '__main__':
    # setup our command syntax
    parser = argparse.ArgumentParser()
    parser.add_argument("cacheDir", help="location to store jinja template cache files")
    parser.add_argument("outputDir", help="location to output generated files")
    parser.add_argument("projectDir", help="location to build directory against")
    parser.add_argument("inputFiles", help="set of files to run azcg expansion rules against")
    parser.add_argument("expansionRules", help="set of azcg expansion rules for matching data files to template files")
    parser.add_argument("-n", "--dryrun", action='store_true', help="does not execute autogen, only outputs the set of files that autogen would generate")
    parser.add_argument("-v", "--verbose", action='store_true', help="output only the set of files that would be generated by an expansion run")
    parser.add_argument("-p", "--pythonPaths", action='append', nargs='+', default=[""], help="set of additional python paths to use for module imports")
    
    args = parser.parse_args()
    pythonPaths = args.pythonPaths
    cacheDir  = args.cacheDir
    outputDir = args.outputDir
    projectDir = args.projectDir
    inputFiles = args.inputFiles.split(";")
    expansionRules = args.expansionRules.split(";")
    dryrun = args.dryrun
    verbose = args.verbose
    cacheDir = os.path.abspath(SanitizePath(cacheDir))
    outputDir = os.path.abspath(SanitizePath(outputDir))
    projectDir = os.path.abspath(SanitizePath(projectDir))

    # Import 3rd party modules
    for pythonPath in pythonPaths:
        sys.path.append(pythonPath)
    import jinja2
    #from lxml import etree
    import xml.etree.cElementTree as etree
    import json

    dataInputSet = {}
    outputFiles  = []
    autoGenResult = ExecuteExpansionRules(cacheDir, outputDir, projectDir, inputFiles, expansionRules, dryrun, verbose, dataInputSet, outputFiles)
    if dryrun:
        print("%s" % ';'.join(outputFiles))
    if autoGenResult:
        sys.exit(0)
    else:
        sys.exit(1)
