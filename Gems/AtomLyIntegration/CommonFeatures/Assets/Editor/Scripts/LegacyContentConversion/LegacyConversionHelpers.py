"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Helper classes for legacy conversion scripts
"""

import os
import platform
import xml.etree.ElementTree

class Stats_Collector(object):
    """
    Stuff any extra info you want to keep track of during conversion into this object
    """
    def __init__(self):
        self.materialOverrideCount = 0
        self.noMaterialOverrideCount = 0


class File_Info(object):
    """
    Keep track of both the filename and the directory of the project
    (not the directory the file is in, but the LY project itself)
    """
    def __init__(self, filename, projectDir):
        self.filename = filename
        self.normalizedProjectDir = os.path.normpath(projectDir)


class Log_File(object):
    """
    Simple class to create, open & save a log file.
    """
    def __init__(self, filename, include_previous = True):
        self.filename = filename
        self.logFile = None
        self.include_previous = include_previous
        self.lines = []
        self.create_log_file()

    def create_log_file(self):
        """
        Will either generate a new log file or open the existing log file and read its contents
        """
        if os.path.exists(self.filename):
            self.logFile = open(self.filename, 'r+')
            if self.include_previous:
                for line in self.logFile.readlines():
                    self.add_line(line)
        else:
            self.logFile = open(self.filename, 'w')

    def save(self):
        """
        Saves the log file back to disk.

        To ensure that filenames are not added twice, this method first saves an empty
        log to disc. Then saves it again with the list of filenames (self.lines)
        stored in this class.
        """
        # First clear the log file's contents so they can be written back in
        self.logFile.close()
        open(self.filename, 'w').close()
        self.logFile = open(self.filename, 'w')

        for line in self.lines:
            self.logFile.write("{0}\n".format(line))

        self.logFile.close()

    def get_lines(self):
        return self.lines

    def add_line(self, line):
        self.lines.append(line)

    def add_line_no_duplicates(self, line):
        """
        Adds a line to the log, and prevents adding duplicates
        Useful for keeping track of files that have already been converted
        The line will be all lowercase for simplicity
        """
        # rstrip with no arguments will remove trailing whitespace
        lowercase = line.lower().rstrip()
        if not lowercase in self.lines:
            self.lines.append(lowercase)

    def has_line(self, line):
        """
        Checks to see if a specific line already exists in the log
        """
        # rstrip with no arguments will remove trailing whitespace
        lowercase = line.lower().rstrip()
        if lowercase in self.lines:
            return True
        return False

class Common_Command_Line_Options(object):
    """
    Some common options/parsing
    """
    def __init__(self, args):
        self.projectName = ""
        self.assetCatalogOverridePath = ""
        self.includeGems = False
        self.useP4 = False
        self.endsWithStr = ""
        self.isHelp = False        
        self.helpString = f"usage: {args[0]} -project=<project name> -include_gems -ends_with=<filter> -use_p4 -assetCatalogOverridePath=<path>\n\
            E.g.:\n\
              -project=StarterGame -include_gems\n\
              -project is required. Path is relative\n\
              -include_gems is optional, and by default gems will not be included.\n\
              -ends_with is optional. It could be used to filter for a specific file (--ends_with=default.mtl)\n\
              -use_p4 is optional. It will use the p4 command line to check out files that are edited in your default changelist\n\
              -assetCatalogOverridePath is optional. It will use asset ids from this file instead of using the current project asset catalog\n\
               (match via relative asset path to the project root folder)"
              

        for argument in args:
            argument = argument.rstrip(" ").lstrip("-")
            if argument == "h" or argument == "help" or argument == "?":
                self.isHelp = True
            elif argument.startswith("project"):
                projectArgs = argument.split("=")
                if len(projectArgs) > 1:
                    self.projectName = projectArgs[1]
            elif argument.startswith("assetCatalogOverridePath"):
                projectArgs = argument.split("=")
                if len(projectArgs) > 1:
                    self.assetCatalogOverridePath = projectArgs[1]
            elif argument == "include_gems":
                self.includeGems = True
            elif argument == "use_p4":
                self.useP4 = True
            elif argument.startswith("ends_with"):
                endsWithArgs = argument.split("=")
                if len(endsWithArgs) > 1:
                    self.endsWithStr = endsWithArgs[1]



def get_file_list(projectName, includeGems, extensionList, buildPath, gemsPath):
    """
    The main difference between this and any other way to walk a directory
    looking for files is that it keeps track of the lumberyard project
    or gems folder the file is in, which can later be used to figure out
    the relative path that is used by the engine for the file
    """

    print("Gathering Files...")
    # Lower the project name for easier matching
    projectName = projectName.lower()

    # First, gather a list of all project folders in the dev root.
    # This will help to reduce the amount of files that the script
    # has to walk through when searching for files.
    projectFolders = []
    for root, dirs, files in os.walk(buildPath):
        if root != buildPath:
            break
        for d in dirs:
            projectFile = "{0}\\{1}\\project.json".format(root, d)
            if os.path.exists(projectFile) and d.lower() == projectName:
                projectFolders.append(os.path.join(root, d))

    # Add all gems to the list of project folders.
    if includeGems:
        for root, dirs, files in os.walk(gemsPath):
            if root != gemsPath:
                break
            while len(dirs) >= 1:
                d = dirs[0]
                gemsFile = "{0}\\{1}\\gem.json".format(root, d)
                if os.path.exists(gemsFile):
                    projectFolders.append(os.path.join(os.path.join(root, d), "Assets"))
                for subroot, subdirs, subfiles in os.walk(os.path.join(gemsPath, d)):
                    if subroot != os.path.join(gemsPath, d):
                        break
                    for subd in subdirs:
                        dirs.append(os.path.join(d, subd))
                dirs.remove(d)

    fileInfoList = []
    for projPath in projectFolders:
        for root, dirs, files in os.walk(projPath):
            if 'Cache' in root.split(os.sep):
                continue
                            
            for f in files:
                for extension in extensionList:
                    if f.endswith(extension):
                        fileInfoList.append(File_Info("{0}\\{1}".format(root, f), projPath))
    return fileInfoList

class Asset_Catalog_Dictionaries(object):
    """
    Some dictionaries from the asset catalog
    """
    def __init__(self, relativePathToAssetIdDict, assetIdToRelativePathDict, assetUuidToAssetIdsDict):
        self.relativePathToAssetIdDict = relativePathToAssetIdDict
        self.assetIdToRelativePathDict = assetIdToRelativePathDict
        self.assetUuidToAssetIdsDict = assetUuidToAssetIdsDict

    def get_asset_id_from_relative_path(self, relativePath):
        # parse the asset catalog and find the assetId
        if relativePath in self.relativePathToAssetIdDict:
            return self.relativePathToAssetIdDict[relativePath]
        return ""

def get_asset_catalog_dictionaries(assetCatalogPath):
    """
    This function pre-supposes that you have modified AssetCatalog::SaveRegistry_Impl
    in Code/Tools/AssetProcessor/native/AssetManager/AssetCatalog.cpp
    to use AZ::ObjectStream::ST_XML instead of AZ::ObjectStream::ST_BINARY
    and subsequently deleted Cache/<project name>/pc/<project name>/assetcatalog.xml
    and let it re-build so that it is a parseable xml file
    """

    print("Parsing Asset Catalog...")
    relativePathToAssetIdDict = {}
    relativePathToAssetIdDict[""] = "{00000000-0000-0000-0000-000000000000}:0"
    assetIdToRelativePathDict = {}
    assetUuidToAssetIdsDict = {}
    assetCatalogXml = xml.etree.ElementTree.parse(assetCatalogPath)
    for possibleAssetInfo in assetCatalogXml.getroot().iter('Class'):
        if "name" in possibleAssetInfo.keys() and possibleAssetInfo.get("name") == "AssetInfo":
            # We found some AssetInfo
            relativePath = ""
            assetId = ""
            for child in possibleAssetInfo:
                if "field" in child.keys() and child.get("field") == "relativePath" and "value" in child.keys():
                    relativePath = child.get("value")
                if "name" in child.keys() and child.get("name") == "AssetId":
                    guid = ""
                    subId = ""
                    for assetIdPart in child:
                        if "field" in assetIdPart.keys() and assetIdPart.get("field") == "guid" and "value" in assetIdPart.keys():
                            guid = assetIdPart.get("value")
                        if "field" in assetIdPart.keys() and assetIdPart.get("field") == "subId" and "value" in assetIdPart.keys():
                            subId = assetIdPart.get("value")
                    assetId = "".join((guid, ":", subId))
            relativePathToAssetIdDict[relativePath] = assetId
            assetIdToRelativePathDict[assetId] = relativePath
            if not guid in assetUuidToAssetIdsDict:
                assetUuidToAssetIdsDict[guid] = []
            assetUuidToAssetIdsDict[guid].append(assetId)
    return Asset_Catalog_Dictionaries(relativePathToAssetIdDict, assetIdToRelativePathDict, assetUuidToAssetIdsDict)

def create_xml_element_from_string(xmlString):
    # copy paste a line from a .slice or other serialization file from lumberyard, add a \ before the " marks, and this function will turn it into an xml element
    # e.g. <Class name=\"EditorMaterialComponent\" field=\"element\" version=\"3\" type=\"{02B60E9D-470B-447D-A6EE-7D635B154183}\">
    # does not protect against malformed strings.
    # relies heavily on things like starting with <, no space between < and the tag, etc.
    # but works in a pinch
    rawXmlList = xmlString.split()
    # strip the < from the first element to get the tag
    element = xml.etree.ElementTree.Element(rawXmlList[0].lstrip("<"))
    for itemIndex in range(len(rawXmlList)):
        item = rawXmlList[itemIndex]
        if itemIndex == 0:
            # ignore the tag because we've already handled it
            continue
        elif itemIndex == (len(rawXmlList) - 1):
            # strip the end tag if it exists
            item = item.rstrip('>')
            item = item.rstrip('/')

        #parse the item
        itemList = item.split('=')
        attribute = itemList[0]
        value = itemList[1].lstrip('\"').rstrip('\"')
        element.set(attribute, value)
    return element

def get_uuid_from_assetId(assetId):
        separatorIndex = assetId.find(":")
        return assetId[:separatorIndex]
    
def get_subid_from_assetId(assetId):
    separatorIndex = assetId.find(":") + 1
    return assetId[separatorIndex:]

def get_default_asset_platform():
    host_platform_to_asset_platform_map = { 'windows': 'pc',
                                            'linux':   'linux',
                                            'darwin':  'mac' }
    return host_platform_to_asset_platform_map.get(platform.system().lower(), "")

class Component_Converter(object):
    """
    Converter base class
    """
    def __init__(self, assetCatalogHelper, statsCollector):
        self.assetCatalogHelper = assetCatalogHelper
        self.statsCollector = statsCollector

    def is_this_the_component_im_looking_for(self, xmlElement, parent):
        pass

    def gather_info_for_conversion(self, xmlElement, parent):
        pass

    def convert(self, xmlElement, parent):
        pass

    def reset(self):
        pass
