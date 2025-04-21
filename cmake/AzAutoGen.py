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
import fnmatch
import fileinput
import logging
import argparse
import hashlib
import pathlib
from xml.sax.saxutils import escape, unescape, quoteattr

logging.basicConfig(format='[%(levelname)s] %(name)s: %(message)s')
logger = logging.getLogger('AzAutoGen')
logger.setLevel(logging.INFO)

# Maximum number of errors before bailing on AutoGen
MAX_ERRORS = 100
errorCount = 0

class AutoGenConfig:
    def __init__(self, targetName, cacheDir, outputDir, projectDir, inputFiles, expansionRules, dryrun, verbose, pythonPaths):
        self.targetName = targetName
        self.cacheDir = cacheDir
        self.outputDir = outputDir
        self.projectDir = projectDir
        self.inputFiles = inputFiles
        self.expansionRules = expansionRules
        self.dryrun = dryrun
        self.verbose = verbose
        self.pythonPaths = pythonPaths

def SanitizeTargetName(targetName):
    return re.sub(r'[^\w]', '', targetName.lstrip('0123456789'))

def ParseInputFile(inputFilePath):
    result = []
    if inputFilePath:
        with open(inputFilePath, 'r') as file:
            # input files are expected to be separated by semicolon at first line
            inputFileContent = file.readline()
            inputFiles = inputFileContent.strip().split(";")
            result = inputFiles
    return result

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

def CreateAZHashValue64(btyes):
    hash = hashlib.new('sha256')
    hash.update(btyes)
    hashStr = hash.hexdigest()
    return ("AZ::HashValue64{ 0x" + hashStr[0:16] + " }")  # grab the first 64-bits of a sha256; any 64-bits of a sha256 are just as secure as any other 64.

def EtreeToString(xmlNode):
    return etree.tostring(xmlNode)

def EtreeToStringStripped(xmlNode):
    for elem in xmlNode.iter():
        if elem.text: elem.text = elem.text.strip()
        if elem.tail: elem.tail = elem.tail.strip()
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

def ProcessTemplateConversion(autogenConfig, dataInputSet, dataInputFiles, templateFile, outputFile, templateCache):
    if autogenConfig.dryrun or not dataInputFiles:
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
        templateEnv.filters['createAZHashValue64'] = CreateAZHashValue64
        templateEnv.filters['etreeToString' ] = EtreeToString
        templateEnv.filters['etreeToStringStripped' ] = EtreeToStringStripped
        templateJinja  = templateEnv.get_template(os.path.basename(templateFile))
        templateVars   = \
            { \
                "autogenTargetName": autogenConfig.targetName, \
                "dataFiles"        : treeRoots, \
                "dataFileNames"    : dataInputFiles, \
                "templateName"     : templateFile, \
                "outputFile"       : outputFile, \
                "filename"         : os.path.splitext(os.path.basename(outputFile))[0], \
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
                compareFD.write('<!-- Template Source {0};\n * XML Sources {1}-->\n'.format(templateFile, ', '.join(dataInputFiles)))
                compareFD.write('\n')
            elif outputExtension == ".lua":
                compareFD.write('-- Copyright (c) Contributors to the Open 3D Engine Project.\n')
                compareFD.write('-- For complete copyright and license terms please see the LICENSE at the root of this distribution.\n')
                compareFD.write('\n')
                compareFD.write('-- SPDX-License-Identifier: Apache-2.0 OR MIT\n')
                compareFD.write('\n')
                compareFD.write('-- This file is generated automatically at compile time, DO NOT EDIT BY HAND\n')
                compareFD.write('-- Template Source {0};\n * XML Sources {1}\n'.format(templateFile, ', '.join(dataInputFiles)))
                compareFD.write('\n')
            elif outputExtension == ".h" or outputExtension == ".hpp" or outputExtension == ".inl" or outputExtension == ".c" or outputExtension == ".cpp":
                compareFD.write('/*\n')
                compareFD.write(' * Copyright (c) Contributors to the Open 3D Engine Project.\n')
                compareFD.write(' * For complete copyright and license terms please see the LICENSE at the root of this distribution.\n')
                compareFD.write(' *\n')
                compareFD.write(' * SPDX-License-Identifier: Apache-2.0 OR MIT\n')
                compareFD.write(' *\n')
                compareFD.write(' * This file is generated automatically at compile time, DO NOT EDIT BY HAND\n')
                compareFD.write(' * Template Source {0};\n * Data Sources {1}\n'.format(templateFile, ', '.join(dataInputFiles)))
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
                    if autogenConfig.verbose == True:
                        print('Generated file %s is unchanged, skipping' % (outputFile))
                else:
                    currentFile.truncate()
                    with open(outputFile, 'w+') as currentFile:
                        currentFile.write(compareFD.getvalue())
                        if autogenConfig.verbose == True:
                            print(f'Generated {outputFile} with template {templateFile} and inputs, '.join(dataInputFiles))
                        else:
                            print('Generated %s' % (os.path.basename(outputFile)))
        else:
            with open(outputFile, 'w+') as outputFD:
                outputFD.write(compareFD.getvalue())
                if autogenConfig.verbose == True:
                    print(f'Generated {outputFile} using template {templateFile} and inputs, '.join(dataInputFiles))
                else:
                    print('Generated %s' % (os.path.basename(outputFile)))

    except IOError as e:
        PrintError('%s(%s) : error I/O(%s) accessing %s : %s' % (fileinput.filename(), str(fileinput.filelineno()), e.errno, e.filename, e.strerror))
    except:
        PrintError('%s(%s) : error Processing: %s' % (fileinput.filename(), str(fileinput.filelineno()), line))
        PrintUnhandledExcptionInfo()
        raise
    compareFD.close()

def ProcessExpansionRule(autogenConfig, sourceFiles, templateFiles, templateCache, expansionRule, dataInputSet, outputFiles):
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
        testSingle = os.path.join(autogenConfig.projectDir, inputFiles)
        if os.path.isfile(testSingle):
            # If we specified an *explicit* file to be processed (no wildcards for the data input file foo.json not *.foo.json), this is the branch that handles this case
            # This is explicitly one-to-one mapping
            dataInputFiles = [os.path.abspath(testSingle)]
            outputFileAbsolute = outputFile.replace("$path", ComputeOutputPath(dataInputFiles, autogenConfig.projectDir, autogenConfig.outputDir))
            outputFileAbsolute = outputFileAbsolute.replace("$fileprefix", os.path.splitext(os.path.basename(testSingle))[0].split(".")[0])
            outputFileAbsolute = outputFileAbsolute.replace("$file", os.path.splitext(os.path.basename(testSingle))[0])
            outputFileAbsolute = SanitizePath(outputFileAbsolute)
            ProcessTemplateConversion(autogenConfig, dataInputSet, dataInputFiles, templateFile, outputFileAbsolute, templateCache)
            outputFiles.append(pathlib.PurePath(outputFileAbsolute))
        else:
            # We've wildcarded the data input field, so we may have to handle one-to-one mapping of data files to output, or many-to-one mapping of data files to output
            if "$fileprefix" in outputFile or "$file" in outputFile:
                # Due to the wildcards in the output file, we've determined we'll do a one-to-one mapping of data files to output
                for filename in fnmatch.filter(sourceFiles, inputFiles):
                    dataInputFiles = [os.path.abspath(filename)]
                    outputFileAbsolute = outputFile.replace("$path", ComputeOutputPath(dataInputFiles, autogenConfig.projectDir, autogenConfig.outputDir))
                    outputFileAbsolute = outputFileAbsolute.replace("$fileprefix", os.path.splitext(os.path.basename(filename))[0].split(".")[0])
                    outputFileAbsolute = outputFileAbsolute.replace("$file", os.path.splitext(os.path.basename(filename))[0])
                    outputFileAbsolute = SanitizePath(outputFileAbsolute)
                    ProcessTemplateConversion(autogenConfig, dataInputSet, dataInputFiles, templateFile, outputFileAbsolute, templateCache)
                    outputFiles.append(pathlib.PurePath(outputFileAbsolute))
            else:
                # Process all matches in one batch
                # Due to the lack of wildcards in the output file, we've determined we'll glob all matching input files into the template conversion
                dataInputFiles = [os.path.abspath(file) for file in fnmatch.filter(sourceFiles, inputFiles)]
                if "$path" in outputFile:
                    outputFileAbsolute = outputFile.replace("$path", ComputeOutputPath(dataInputFiles, autogenConfig.projectDir, autogenConfig.outputDir))
                else: # if no relative $path, put one batch file under outputDir
                    outputFileAbsolute = os.path.join(autogenConfig.outputDir, outputFile)
                outputFileAbsolute = SanitizePath(outputFileAbsolute)
                ProcessTemplateConversion(autogenConfig, dataInputSet, dataInputFiles, templateFile, outputFileAbsolute, templateCache)
                outputFiles.append(pathlib.PurePath(outputFileAbsolute))
    except IOError as e:
        PrintError('%s : error I/O(%s) accessing %s : %s' % (expansionRule, e.errno, e.filename, e.strerror))
    except:
        PrintError('%s : error Processing expansion rule' % expansionRule)
        PrintUnhandledExcptionInfo()
        raise

def ExecuteExpansionRules(autogenConfig, dataInputSet, outputFiles, pruneNonGenerated):
    # Get Globals
    global MAX_ERRORS, errorCount
    currentPath = os.getcwd()
    startTime = time.time()
    # Ensure jinja2 template cache dir actually exists...
    try:
        os.makedirs(autogenConfig.cacheDir)
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise
    sourceFiles = []
    templateFiles = []
    for inputFile in autogenConfig.inputFiles:
        if inputFile.endswith(".xml") or inputFile.endswith(".json"):
            sourceFiles.append(os.path.join(autogenConfig.projectDir, inputFile))
        elif inputFile.endswith(".jinja"):
            templateFiles.append(os.path.join(autogenConfig.projectDir, inputFile))
    templateCache = jinja2.FileSystemBytecodeCache(autogenConfig.cacheDir)
    for expansionRule in autogenConfig.expansionRules:
        ProcessExpansionRule(autogenConfig, sourceFiles, templateFiles, templateCache, expansionRule, dataInputSet, outputFiles)
    if not autogenConfig.dryrun:
        if pruneNonGenerated:
            PruneNonGeneratedFiles(autogenConfig, outputFiles)
        elapsedTime = time.time() - startTime
        millis = int(round(elapsedTime * 10))
        m, s = divmod(elapsedTime, 60)
        h, m = divmod(m, 60)
        print('Total Time %d:%02d:%02d.%02d' % (h, m, s, millis))
    # Return true on success
    return errorCount == 0

def PruneNonGeneratedFiles(autogenConfig : AutoGenConfig, outputFiles : list[pathlib.PurePath]):
    '''
    Removes all files from the generated files output directories which was not generated during this invocation
    :param autogenConfig: Stores the configuration structure containing the output directory paths for generated files
    :param outputFiles: Contains the list of output files generated during the current run
    '''
    # First generate a set of output directories to iterate using the outputFiles
    generatedOutputDirs = set()
    for outputFile in outputFiles:
        generatedOutputDirs.add(pathlib.Path(outputFile.parent))

    # iterate over all the output directories where generated files are output
    # and gather a list of files that were not generated during the current invocation
    for outputDir in generatedOutputDirs:
        filesToRemove = []
        if outputDir.is_dir():
            for genFile in outputDir.iterdir():
                if genFile.is_file() and not genFile in outputFiles:
                    filesToRemove.append(genFile)
        if filesToRemove:
            logger.info(f'The following files will be pruned from the generated output directory "{outputDir}":\n' \
                f'{[str(path) for path in filesToRemove]}')
            for fileToRemove in filesToRemove:
                fileToRemove.unlink()


# Main Function
if __name__ == '__main__':
    # setup our command syntax
    parser = argparse.ArgumentParser()
    parser.add_argument("targetName", help="AzAutoGen build target name")
    parser.add_argument("cacheDir", help="location to store jinja template cache files")
    parser.add_argument("outputDir", help="location to output generated files")
    parser.add_argument("projectDir", help="location to build directory against")
    parser.add_argument("inputFilePath", help="input file which contains autogen required files to run azcg expansion rules against")
    parser.add_argument("expansionRules", help="set of azcg expansion rules for matching data files to template files")
    parser.add_argument("-n", "--dryrun", action='store_true', help="does not execute autogen, only outputs the set of files that autogen would generate")
    parser.add_argument("-v", "--verbose", action='store_true', help="output only the set of files that would be generated by an expansion run")
    parser.add_argument("-p", "--pythonPaths", action='append', nargs='+', default=[""], help="set of additional python paths to use for module imports")
    parser.add_argument("--prune", action='store_true', default=False,
        help="Prunes any files in the outputDir that was not generated by the current invocation")

    args = parser.parse_args()
    autogenConfig = AutoGenConfig(SanitizeTargetName(args.targetName),
                                  os.path.abspath(SanitizePath(args.cacheDir)),
                                  os.path.abspath(SanitizePath(args.outputDir)),
                                  os.path.abspath(SanitizePath(args.projectDir)),
                                  ParseInputFile(args.inputFilePath.strip()),
                                  args.expansionRules.split(";"),
                                  args.dryrun,
                                  args.verbose,
                                  args.pythonPaths)

    # Import 3rd party modules
    for pythonPath in autogenConfig.pythonPaths:
        sys.path.append(pythonPath)
    import jinja2
    #from lxml import etree
    import xml.etree.cElementTree as etree
    import json

    dataInputSet = {}
    outputFiles  = []
    autoGenResult = ExecuteExpansionRules(autogenConfig, dataInputSet, outputFiles, args.prune)
    if autogenConfig.dryrun:
        print("%s" % ';'.join([str(path) for path in outputFiles]))
    if autoGenResult:
        sys.exit(0)
    else:
        sys.exit(1)
