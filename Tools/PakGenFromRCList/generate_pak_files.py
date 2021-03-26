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
import os
import os.path
import glob
import sys
import optparse
import subprocess
import xml.etree.ElementTree as ET
import xml.dom.minidom
import glob

def RunCmd(cmd, useShell=False, workingDir=None, silent=False, printOutput=True, stripTrailing=True):
    if not silent:
        print ''
        print 'Running: %s' % ' '.join(cmd)

    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=useShell, cwd=workingDir)
    while True:
        output = process.stdout.readline()
        if output:
            if printOutput:
                if stripTrailing:
                    print output[0:-2]    # Print output stripping trailing \n
                else:
                    print output
        else:
            break

    process.stdout.close()

    rc = process.wait()
    assert rc != None

    if not silent:
        print 'Program finished with return code: %d \n' % rc
    
    return rc
def createJob(rootFolder, outputPak, jobPath):
    jobString = '''
    <RCJobs>
     <DefaultProperties
        rootFolder="%s"
        outputPak="%s"
        gamedata="Difficulty\*.*;Entities\*.*;Fonts\*.*;Libs\*.*;Materials\*.*;Prefabs\*.*;Levels\*.*xml"
        scripts="Scripts\*.*"
        geomcache_list="GameRyse\Animations\cinematics\GeomCachesCine1.txt"
     />
     <PakJob>
        <Job sourceroot="${rootFolder}" input="*.*" zip="${outputPak}" exclude="${gamedata};${scripts};Videos\*.*;Sounds\*.*;Music\*.*;*dds*;*.cax;Libs\UI\*.*;Localization\*.*;_levelpak\*.*;levels\*.*;Animations\Mannequin\*.*" zip_sizesplit="1" />
        <Job sourceroot="${rootFolder}" input="*.dds" zip="${outputPak}" listformat="{1}.dds;{1}.$dds;{1}.dds.1;{1}.dds.1a;{1}.dds.2;{1}.dds.2a;{1}.dds.3;{1}.dds.3a;{1}.dds.4;{1}.dds.4a;{1}.dds.5;{1}.dds.5a;{1}.dds.6;{1}.dds.6a;{1}.dds.7;{1}.dds.7a;{1}.dds.8;{1}.dds.8a;{1}.dds.9;{1}.dds.9a" exclude="Libs\UI\*.*;Localization\*.*" zip_sizesplit="1" />
        <Job sourceroot="${rootFolder}" input="*.dds" zip="${outputPak}" listformat="{1}.dds.0;{1}.dds.0a" exclude="Libs\UI\*.*;Localization\*.*" sourceminsize="4001" zip_sizesplit="1" />
        <Job sourceroot="${rootFolder}" input="*.cax" zip="${outputPak}" zip_compression="0" exclude_listfile="${rootFolder}\..\..\Build\${geomcache_list}" zip_sizesplit="1" />
        <Job sourceroot="${rootFolder}" input="Music\*.*" zip="${outputPak}" zip_compression="0" zip_sizesplit="1" />
        <Job sourceroot="${rootFolder}" input="Sounds\*.*" zip="${outputPak}" zip_compression="0" zip_sizesplit="1" />
        <Job sourceroot="${rootFolder}" input="Videos\*.*" exclude="Videos\Cinematics\Unlockable\*.*" zip="${outputPak}" zip_compression="0" zip_sizesplit="1" />
        <!-- Part of the RC Build proccess
        <Job sourceroot="${rootFolder}" input="Libs\UI\*.*" zip="${outputPak}" zip_compression="0" exclude="*.dat;Libs\UI\Menus\NewMenu\Screens\CollectiblesComics\*.*;Libs\UI\Menus\NewMenu\Screens\CollectiblesDogtags\*.*;Libs\UI\Menus\NewMenu\Screens\CollectiblesVistas\*.*" zip_sizesplit="1" /> 
        -->        
      </PakJob>

      <Run Job="PakJob"/>
    </RCJobs>
    ''' % (rootFolder, outputPak)
    
    f = open(jobPath, 'w')
    f.write(jobString)
    f.close()
    
def generate_chunk_xmls(pakList, input, outputInstall, outputMakePkg):
    try:
        tree = ET.parse(input)
        package = tree.getroot()
        
        launch_chunk_index = 100
        chunk_index = 10000
        
        for result in pakList:
            pattern, pak = result
        
            chunk = ET.Element('Chunk')            
            if pattern != 'Launch':    
                chunk.attrib['Id'] = str(chunk_index)
                chunk_index += 1                
                chunk.attrib['CryMarker'] = 'Level data'
                chunk.attrib['CryLevels'] = pattern
            else:
                chunk.attrib['Id'] = str(launch_chunk_index)
                launch_chunk_index += 1
                chunk.attrib['CryMarker'] = 'Launch data'      
            
            file_group = ET.Element('FileGroup')
            file_group.attrib['DestinationPath'] = '\\GameSDK'
            file_group.attrib['SourcePath'] = 'GameSDK'
            file_group.attrib['Include'] = pak

            chunk.append(file_group)            

            if pattern != 'Launch':
                package.append(chunk)
            else:
                package.insert(0, chunk)
                
        # Insert update alignment chunk
        chunk = ET.Element('Chunk')  
        chunk.attrib['Id'] = "1073741823"
        file_group = ET.Element('FileGroup')
        file_group.attrib['DestinationPath'] = '\\'
        file_group.attrib['SourcePath'] = '.\\'
        file_group.attrib['Include'] = "Update.AlignmentChunk"
        chunk.append(file_group)
        package.append(chunk)        
           
        with open(outputInstall,'w+b') as out_handle:
            out_handle.write(xml.dom.minidom.parseString(ET.tostring(package)).toprettyxml())
            
        for chunk in package:
            for k, v in chunk.attrib.items():
                if k[:3] == 'Cry':
                    del chunk.attrib[k]

        with open(outputMakePkg,'w+b') as out_handle:
            out_handle.write(xml.dom.minidom.parseString(ET.tostring(package)).toprettyxml())
            
    except Exception as e:
        print "Couldn't generate chunk XML"
        print e    
    
def main():
    parser = optparse.OptionParser('usage: %prog filelist_path game_path rc_path archive_path install_chunks_source_xml install_chunks_target_xml makepkg_chunks_target_xml')

    (options, args) = parser.parse_args()
    if len(args) < 7:
        parser.error("incorrect number of arguments")
        return 1

    print 'Generate PAK files from file lists and game folder into archive path'

    # Get arguments
    path_from = args[0]
    path_game = args[1]
    path_rc = args[2]
    path_to = args[3]
    install_chunks_source_xml = args[4]
    install_chunks_target_xml = args[5]
    makepkg_chunks_target_xml = args[6]    
    
    rcExecutable = os.path.join(args[2], 'rc.exe')

    if not os.path.exists(path_from):
        print 'Input path not valid'
        return 1
        
    if not os.path.exists(path_from):
        print 'Input path not valid'
        return 1
        
    if not os.path.exists(rcExecutable):
        print 'RC path not valid'
        return 1
        
    if not os.path.exists(path_to):
        print 'Output path doesn\'t exist, creating it now'
        os.makedirs(path_to)
        
    fileList = glob.glob(os.path.join(path_from, '*GFL.txt'))
    
    print ''
    print 'Total number of files: %s' % len(fileList)
    
    print ''
    print 'Creating PAK files and storing them in %s' % path_to
    
    pakList = []    
    for file in fileList:
        pattern = file[len(path_from) + 7: -8]
        pak_path = os.path.join(path_to, '%s.pak' % pattern)
        pak_glob_path = os.path.join(path_to, '%s-part*.pak' % pattern)
        
        print ' %s -> %s' % (pattern, pak_path)
        
        createJob(path_game, pak_path, '%s\job_temp.xml' % path_from)
        
        rcCmd = [rcExecutable, '/threads=8', '/logprefix=..\\%s_' % pattern, '/quiet', '/job=job_temp.xml', '/listfile=%s' % file]
        
        RunCmd(rcCmd, workingDir=path_from, silent=False, printOutput=True)
        
        if pattern.lower() != "extra":
            patternPakList = glob.glob(pak_glob_path)
            for patternPak in patternPakList:
                pakList.append((pattern, os.path.basename(patternPak)))            
            if os.path.exists(pak_path):
                pakList.append((pattern, pattern + '.pak'))
    
    patching = False
    
    print ''
    print 'Writing chunk XMLs %s and %s' % (install_chunks_target_xml, makepkg_chunks_target_xml)
    
    if patching:
        print '  DISABLED'
    else:
        generate_chunk_xmls(pakList, install_chunks_source_xml, install_chunks_target_xml, makepkg_chunks_target_xml)
        
    print ''
    print 'Done!'


if __name__ == '__main__':
    sys.exit(main())