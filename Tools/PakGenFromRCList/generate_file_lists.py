#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
import os.path
import sys
import optparse
import xml.etree.ElementTree as ET
from fnmatch import fnmatch

def filter_files(old_list):
    new_list = set()
    
    for item in old_list:
        # split rc output
        if '=' in item:
            item_split = item.split('=')
            if not len(item_split) == 2:
                continue

            item = item_split[1]
                
            if not 'GameSDK' in item:
                continue
                
            item_split = item.split('GameSDK')
            if not len(item_split) == 2:
                continue
                
            item = item_split[1][1:]
            
            item = item.replace('\\', '/')
        
        # get rid of double slashes
        item = item.replace('//', '/')
                
        # transform any dds into wildcard
        if '.dds' in item:
            item_split = item.split('.dds')
            item = item_split[0] + '.dds\n'
        if '.$dds' in item:
            item_split = item.split('.$dds')
            item = item_split[0] + '.dds\n'
            
        # convert to lower case
        item = item.lower()
        new_list.add(item)
            
        if item.split('.')[-1].strip() in ['cgf', 'cga', 'skin']:
            newFile = item.strip() + 'm\n'
            new_list.add(newFile)
                
    return new_list

def read_file_mapping_rule(includeRules, excludeRules, node, levelTag):
    for ruleNode in node:
        rule = { 'levelTag' : levelTag }
        rule['path'] = ruleNode.attrib['Path']
        if ruleNode.tag.lower() == 'include':
            includeRules.append(rule)
        elif ruleNode.tag.lower() == 'exclude':
            excludeRules.append(rule)

def read_file_mapping_rules(file_mapping_xml, masterSet, levelList, resultList):
    fileMappingTree = ET.parse(file_mapping_xml)
    fileMappingRoot = fileMappingTree.getroot()
    
    levelIncludeRules = []
    levelExcludeRules = []
    globalIncludeRules = []
    globalExcludeRules = []
    launchIncludeRules = []    
    
    for node in fileMappingRoot:
        if node.tag.lower() == 'level': 
            levelName = node.attrib['Name'].lower()
            levelTag = levelList[levelName]
            read_file_mapping_rule(levelIncludeRules, levelExcludeRules, node, levelTag)                   
    
    for node in fileMappingRoot:
        if node.tag.lower() == 'global':
            read_file_mapping_rule(globalIncludeRules, globalExcludeRules, node, 'global')

    for node in fileMappingRoot:
        if node.tag.lower() == 'launch':
            read_file_mapping_rule(launchIncludeRules, [], node, 'launch')            
            
    return (levelIncludeRules, levelExcludeRules, globalIncludeRules, globalExcludeRules, launchIncludeRules)
    
def apply_file_mapping(masterSet, levels, resultList, fileMappingRules):
    levelIncludeRules = fileMappingRules[0]
    levelExcludeRules = fileMappingRules[1]
    globalIncludeRules = fileMappingRules[2]
    globalExcludeRules = fileMappingRules[3]
    launchIncludeRules = fileMappingRules[4]
    
    allLevelSet = ''
    for levelTag in levels:
        allLevelSet += levelTag    
    
    masterSetList = list(masterSet)
    for resource in masterSetList:
        levelSet = ''
        
        bLaunchInclude = False
        bGlobalInclude = False
        bGlobalExclude = False
        bLevelInclude = False
        bLevelExclude = False 
        
        resourceStripped = resource.strip()

        for includeRule in launchIncludeRules:                 
            if fnmatch(resourceStripped, includeRule['path']):
                bLaunchInclude = True
                levelSet = 'Launch'
                break
        
        if not bLaunchInclude:
            for levelTag in levels:
                bResourceInAutoList = resource in levels[levelTag]['resources']            
                bForceInclude = False

                for includeRule in levelIncludeRules:                
                    if (includeRule['levelTag'] == levelTag) and fnmatch(resourceStripped, includeRule['path']):
                        bLevelInclude = True
                        bForceInclude = True
                        break
                if not bLevelInclude:
                    for excludeRule in levelExcludeRules: 
                        if (excludeRule['levelTag'] == levelTag) and fnmatch(resourceStripped, excludeRule['path']):
                            bLevelExclude = True
                            break

                if bForceInclude or (bResourceInAutoList and not bLevelExclude):
                    levelSet += levelTag
                    
            if not bLevelInclude:
                for includeRule in globalIncludeRules:                 
                    if fnmatch(resourceStripped, includeRule['path']):
                        bGlobalInclude = True
                        levelSet = allLevelSet
                        break
            
            if not (bLevelInclude or bGlobalInclude):
                for excludeRule in globalExcludeRules: 
                    if fnmatch(resourceStripped, excludeRule['path']):
                        bGlobalExclude = True
                        break
             
        if levelSet != '' and not bGlobalExclude:
            if not levelSet in resultList:
                resultList[levelSet] = []                
            resultList[levelSet].append(resource)
            masterSet.remove(resource)
        elif bLevelExclude or bGlobalExclude:
            masterSet.remove(resource)  

def merge_paks(target_game, levels, resultList, pakSizeThreshold, numFilesThreshold):
    allLevelList = ''
    for levelIndicator in levels:
        allLevelList += levelIndicator    
    
    smallList = []
    bigList = []
    for result in resultList.keys():
        if result == 'Launch' or result == 'EXTRA':
            continue        
        
        fileList = resultList[result]
        if len(fileList) <= numFilesThreshold:
            totalSize = 0
            for fileName in fileList:
                fullPath = target_game + '\\' + fileName.replace('/', '\\').strip()
                if fullPath[-4:] == '.dds':
                    fullPath = fullPath[:-4] + '.$dds'
                fileSize = os.path.getsize(fullPath) if os.path.exists(fullPath) else 0
                totalSize += fileSize
            
            if totalSize <= pakSizeThreshold:
                smallList.append(result)
            else:
                bigList.append(result)
        else:
            bigList.append(result)
                
    for small in smallList:
        bestMatch = allLevelList
        for big in bigList:
            bigSet = set(big)
            if all((c in bigSet) for c in small) and len(big) < len(bestMatch):
                bestMatch = big
        
        print '  Merged %s.pak with %s.pak' % (small, bestMatch)
        fileList = resultList[small]
        resultList[bestMatch].extend(fileList)
        del resultList[small]

def merge_paks_fixed(levels, resultList, availableList):
    allLevelList = ''
    for levelIndicator in levels:
        allLevelList += levelIndicator    
    
    invalidList = []
    for result in resultList.keys():
        if result == 'Launch' or result == 'EXTRA':
            continue        
        
        if not result in availableList:
            invalidList.append(result)
            
    for invalid in invalidList:
        bestMatch = allLevelList
        for available in availableList:
            availableSet = set(available)
            if all((c in availableSet) for c in invalid) and len(available) < len(bestMatch):
                bestMatch = available
        
        print '  Merged %s.pak with %s.pak' % (invalid, bestMatch)
        fileList = resultList[invalid]
        if not bestMatch in resultList:
            print '%s NOT FOUND IN RESULT LIST - ADDING IT NOW' % bestMatch
            resultList[bestMatch] = fileList
        else:
            resultList[bestMatch].extend(fileList)
        del resultList[invalid]

def filter_file_lists(level, levels, levelIndicator, rcFiles):
    for levelIndicator in levels:
        level = levels[levelIndicator]
        print '  Filtering %s' % level['name']
        levels[levelIndicator]['resources'] = filter_files(levels[levelIndicator]['resources'])
    
    print '  Filtering RC log'
    rcFiles = filter_files(rcFiles)    
    return rcFiles
    
def load_level_files(level, levels, levelIndicator):
    for levelIndicator in levels:
        level = levels[levelIndicator]
        print '  Loading %s (%s)' % (level['name'], level['filename'])
        if os.path.exists(level['filename']):
            with open(level['filename']) as f:
                levels[levelIndicator]['resources'] = set(f.readlines())
        else:
            print 'LEVEL %s NOT FOUND' % level['filename']

def create_extra_list(masterSet, resultList):
    resultList['EXTRA'] = []
    for resource in masterSet:
        resultList['EXTRA'].append(resource)

def write_result_lists(path_to, resultList):
    totalPath = os.path.join(path_to, 'files_TOTAL.txt')
    with open(totalPath, 'w') as totalFileHandle:
        for result in resultList:
            resultPath = os.path.join(path_to, 'files_%s_GFL.txt' % result)
            print '  %s -> %s' % (result, resultPath)
            with open(resultPath, 'w') as f:
                f.writelines(resultList[result])
            
            for resource in resultList[result]:
                totalFileHandle.write('%s - %s' % (result, resource))

def read_chunk_xmls(input):
    chunks = []
    
    try:
        tree = ET.parse(input)
        package = tree.getroot()
        
        for chunk in package:
            for k, v in chunk.attrib.items():
                if k == 'CryLevels':
                    chunks.append(v)

    except Exception as e:
        print "Couldn't load chunk XML"
        print e
        
    return chunks
    
def main():
    parser = optparse.OptionParser('usage: %prog source_game target_game path_to path_rc_log level_mapping_xml file_mapping_xml install_chunks_source_xml')

    (_, args) = parser.parse_args()
    if len(args) < 6:
        parser.error("incorrect number of arguments")
        return 1

    print 'Generate fileName lists from level requirements'

    # Get arguments
    source_game = args[0]
    target_game = args[1]
    path_to = args[2]
    path_rc_log = args[3]
    level_mapping_xml = args[4]
    file_mapping_xml = args[5]
    install_chunks_source_xml = args[6]
    
    patching = False
    
    if not os.path.exists(source_game):
        print 'Input path not valid'
        return 1
        
    if not os.path.exists(path_rc_log):
        print 'RC log not found'
        return 1
        
    if not os.path.exists(path_to):
        print 'Output path doesn\'t exist, creating it now'
        os.makedirs(path_to)
        
    print 'Cleaning output folder'
    for fileName in os.listdir(path_to):
        os.remove(path_to + '\\' + fileName)
        
    testResultPath = source_game + '\Levels'
    
    levelMappingTree = ET.parse(level_mapping_xml)
    levelsRoot = levelMappingTree.getroot()
    
    levelList = {}    
    for level in levelsRoot:
        levelList[level.attrib['Name']] = level.attrib['tag']
    
    if not os.path.exists(testResultPath):
        print 'No results in build, exiting'
        return 1
        
    if len(levelList) == 0:
        print 'Level list, exiting'
        return 1
        
    print 'Levels containing resources list:'
    
    levels = {}
    for level in levelList:    
        levelIndicator = levelList[level]
        if levelIndicator in levels:
            print 'Duplicate found, no support for this. Exiting'
            return 1
            
        levels[levelIndicator] = {'name': level, 'filename': os.path.join(testResultPath, level, 'auto_resourcelist_total.txt'), 'resources': set()}
        print '  [%s] -> %s' % (levelIndicator, level)
    
    print ''
    print 'Loading files in memory'    
    load_level_files(level, levels, levelIndicator)
        
    print ''
    print 'Loading RC log in memory'
    with open(path_rc_log, 'r') as f:
        rcFiles = f.readlines()
    
    print ''
    print 'Filtering files'
    rcFiles = filter_file_lists(level, levels, levelIndicator, rcFiles)    
        
    print ''
    print 'Creating master set'
    
    masterSet = set()    
    for levelIndicator in levels:
        level = levels[levelIndicator]
        masterSet |= level['resources']    
    masterSet |= set(rcFiles)
    
    print 'Done, %s items' % len(masterSet)
    
    resultList = {}
    
    print ''
    print 'Reading static rules...'
    fileMappingRules = read_file_mapping_rules(file_mapping_xml, masterSet, levelList, resultList)  
    
    print 'Going through list to find matches per level...'
    apply_file_mapping(masterSet, levels, resultList, fileMappingRules)

    print 'Creating extra list...'    
    create_extra_list(masterSet, resultList)

    print 'Done, %s combinations' % len(resultList)

    pakSizeThreshold = 20000000
    numFilesThreshold = 500
    
    print ''
    print 'Merging PAKs under threshold size of %d files and %d bytes' % (numFilesThreshold, pakSizeThreshold)
    if patching:
        print '  DISABLED'
    else:
        merge_paks(target_game, levels, resultList, pakSizeThreshold, numFilesThreshold)

    
    if patching:
        print ''
        print 'Merging PAKs using fixed set of chunks'
        availableList = read_chunk_xmls(install_chunks_source_xml)
        merge_paks_fixed(levels, resultList, availableList)

    print ''
    print 'Dumping results in %s' % path_to
    write_result_lists(path_to, resultList)    
 
    print ''
    print 'Done, %d PAKs' % len(resultList)
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
