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

import sys
import time
import re
import os
import stat
import subprocess
import logging
import argparse
import shutil

AMAZON_HEADER='''/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
'''

PY_AMAZON_HEADER='''#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
'''

BAT_AMAZON_HEADER='''@echo off
REM
REM  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM  its licensors.
REM
REM  REM  For complete copyright and license terms please see the LICENSE at the root of this
REM  distribution (the "License"). All use of this software is governed by the License,
REM  or, if provided, by the license below or the license accompanying this file. Do not
REM  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM
'''


AMAZON_EXT_HEADER_TOP='''////////////////////////////////////////////////////////////////////////////
//
// All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
// its licensors.
//
// For complete copyright and license terms please see the LICENSE at the root of this
// distribution (the "License"). All use of this software is governed by the License,
// or, if provided, by the license below or the license accompanying this file. Do not
// remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//
'''

AMAZON_EXT_HEADER_BOTTOM='''//
////////////////////////////////////////////////////////////////////////////
'''

AMAZON_LUA_HEADER_TOP='''----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
'''

AMAZON_LUA_HEADER_BOTTOM='''--
----------------------------------------------------------------------------------------------------
'''



AMAZON_DEPRECATED_HEADER='Copyright 2015 Amazon.com'

FORMER_CRYTEK_HEADER='// Original file Copyright Crytek GMBH or its affiliates, used under license.'

PY_FORMER_CRYTEK_HEADER='# Original file Copyright Crytek GMBH or its affiliates, used under license.'

LUA_FORMER_CRYTEK_HEADER='-- Original file Copyright Crytek GMBH or its affiliates, used under license.'

BAT_FORMER_CRYTEK_HEADER='REM  Original file Copyright Crytek GMBH or its affiliates, used under license.'

# BEYOND_COMPARE_PATH="\"C:\\Program Files (x86)\\Beyond Compare 4\\BCompare.exe\""
BEYOND_COMPARE_PATH="\"C:\\Program Files (x86)\\Beyond Compare 3\\BCompare.exe\""



DEFAULT_FILTERED_EXTENSIONS = ['CS','H','HPP','HXX','INL','C','CPP', 'EXT','PY', 'LUA', 'BAT', 'CFX', 'CFI']

# Check if this line had the original (non-amazon) copyright notice
def _is_former_crytek_header(line):
    if 'CRYTEK' in line.upper() and line != FORMER_CRYTEK_HEADER and line != PY_FORMER_CRYTEK_HEADER:
        return True
    else:
        return False


def _is_amazon_header(line):
    if 'AMAZON.COM' in line.upper() and not AMAZON_DEPRECATED_HEADER.upper() in line.upper():
        return True
    else:
        return False

def _is_amazon_deprecated_header(line):
    if AMAZON_DEPRECATED_HEADER.upper() in line.upper():
        return True
    else:
        return False

ORIGINAL_AMAZON_SOURCE_FOLDER_ROOTS = ['./Code/Framework',
                                       './Code/GameCore',
                                       './Code/GameCoreTemplate',
                                       './Gems',
                                       './Code/Tools/AzCodeGenerator']
SKIP_FOLDERS = ['/Code/SDKs',
                '/Code/Sandbox/SDKs',
                '/Code/Tools/SDKs',
                '/Code/Tools/waf-1.7.13',
                '/Code/Tools/MaxCryExport/Skin/12',
                '/Code/Tools/MaxCryExport/Skin/13',
                '/Code/Tools/MaxCryExport/Skin/14',
                '/Code/Tools/MaxCryExport/Skin/15',
                '/Code/Tools/MaxCryExport/Skin/16',
                '/Code/Tools/MaxCryExport/Skin/17',
                '/Code/Tools/MaxCryExport/Skin/18',
                '/Code/Tools/HLSLCrossCompiler',
                '/Code/Tools/HLSLCrossCompilerMETAL',
                '/BinTemp']


SKIP_FILES = ['resource.h',
              '__init__.py']

logging.basicConfig(filename='copyright_update.log',level=logging.INFO)

log_file_updated = open('copyright_updated.files','w')
log_file_updated.write('# Copyright Updated File List\n#\n')

log_file_skipped = open('copyright_skipped.files','w')
log_file_skipped.write('# Copyright Skipped File List\n#\n')

log_file_open_source = open('opensource.files','w')
log_file_open_source.write('# Detected open source files\n#\n')

def log_update(path, detail):
    logging.info('Updating file {} : {}'.format(path,detail))
    log_file_updated.write('{}\n'.format(path))

def log_skipped(path, detail):
    logging.info('Skipping file {} : {}'.format(path,detail))
    log_file_skipped.write('{}\n'.format(path))

def log_opensource(path, license):
    logging.info('Skipping file {} : Licensed ({}) File detected'.format(path,license))
    log_file_open_source.write('{}\n'.format(path))

def close_logs():
    log_file_updated.close()
    log_file_skipped.close()
    log_file_open_source.close()

def _is_skip_folder(root_code_path,dirname):

    normalized = '/'+dirname.replace(root_code_path,'').replace('\\','/').upper()
    for skip_path in SKIP_FOLDERS:
        if normalized.startswith(skip_path.upper()):
            return True
    return False

def _is_skip_file(filepath):
    if filepath.endswith('.Designer.cs'):
        return True
    normalized = os.path.dirname(filepath).replace('\\','/').upper()
    for skip_file in SKIP_FILES:
        if normalized.endswith('/'+skip_file.upper()):
            return True
    return False

def _is_file_original_amazon(filepath):

    normalized = filepath.replace('\\','/').upper()
    for amazon_path in ORIGINAL_AMAZON_SOURCE_FOLDER_ROOTS:
        if normalized.startswith(amazon_path.upper()):
            return True
    return False

def _is_copyright_notice_tbd(line):
    if 'COPYRIGHT_NOTICE_TBD' in line.upper():
        return True
    else:
        return False

def _has_original_crytek_note(line):
    if FORMER_CRYTEK_HEADER in line:
        return True
    elif PY_FORMER_CRYTEK_HEADER in line:
        return True
    else:
        return False

def _is_cs_auto_generated(line):
    if 'auto-generated' in line:
        return True
    else:
        return False

def _is_open_source(line,filename):
    if 'Microsoft Public License' in line:
        log_opensource(filename,'Microsoft Public License')
        return True
    if '$QT_BEGIN_LICENSE:LGPL$' in line:
        log_opensource(filename,'QT Lesser General Public License')
        return True
    if 'LICENSE.LGPL' in line:
        log_opensource(filename,'GPL License')
        return True
    if 'Apache License' in line:
        log_opensource(filename,'Apache License')
        return True
    if 'BSD LICENSE' in line.upper():
        log_opensource(filename,'BSD License')
        return True
    if 'ADOBE SYSTEMS' in  line.upper():
        log_opensource(filename,'Adobe License')
        return True
    if 'Public License' in line:
        log_opensource(filename,'Public License')
        return True

    return False

def _is_amazon_old_header_line(line):
    if 'a third party where indicated' in line:
        return True
    else:
        return False

def check_file_status(filepath):
    st = os.stat(filepath)
    return bool(st.st_mode & (stat.S_IWGRP|stat.S_IWUSR|stat.S_IWOTH))

def checkout_file(root_code_path,filepath, p4_path_root,cl_number):
    p4_file_path = filepath.replace(root_path,'')
    p4_path = p4_path_root + p4_file_path
    p4_path = p4_path.replace('\\','/')

    try:
        result = subprocess.call('p4 edit -c {} \"{}\"'.format(cl_number,p4_path))
        if result < 0:
            print('[ABORT] Process terminated by signal')
            return False
    except OSError as e:
        print('[ERROR] p4 call error:{}'.format(e))
        return False

    return check_file_status(filepath)


def load_original_crytek_set(path, filtered_extensions):
    original_set = set()
    with open(path,'r') as f:
        file_content = f.readlines()
        for filename in file_content:
            if filename.startswith('#'):
                continue
            base_name = filename.replace('//crypristine/vendor_branch_3.8.1','').strip()
            orig_file, orig_ext = os.path.splitext(base_name)
            if orig_ext.upper()[1:] in filtered_extensions:
                original_set.add(base_name.upper())
    return original_set


def read_input_files_file(path,root_path):
    files_to_process = set()
    if not os.path.exists(path):
        print('Invalid input file:{}'.format(path))
    else:
        with open(path,'r') as f:
            file_content = f.readlines()
            for filename in file_content:
                if filename.startswith('#'):
                    continue
                base_name = filename.replace(root_path,'').strip()
                base_name = os.path.realpath(root_path + '/' + base_name)
                files_to_process.add(base_name.upper())
    return files_to_process




def process_file(orig_filepath, original_file_content, updated_file_content, is_original_crytek):

    def _script_non_printable(instr):
        return re.sub('[^\040-\176]','',instr)

    orig_file, orig_ext = os.path.splitext(orig_filepath)
    orig_ext = orig_ext[1:].lower()

    is_csharp = orig_ext == 'cs'
    is_cc = orig_ext in ['h','hpp','cxx','cpp','c','inl','cc']
    is_py = orig_ext in ['py']
    is_java = orig_ext in ['java']
    is_ext = orig_ext == 'ext'
    is_lua = orig_ext == 'lua'
    is_bat = orig_ext == 'bat'
    is_cf = orig_ext in ['cfx','cfi']

    # Is the header removable (Either Crytek or COPYRIGHT_NOTICE_TBD)7
    header_removable = False

    # Has a comment header, is it original crytek?
    is_original_crytek_header = False

    # Has a comment header, is it amazon?
    is_amazon_header = False

    has_original_crytek_note = False

    # Has a comment header, is it deprecated amazon?
    is_amazon_deprecated_header = False

    # Is this a placeholder for the copyright notice
    is_copyright_notice_tbd = False

    # No header block?
    missing_header_block = False

    # Comment header block starts with // (continue until first #)
    starts_with_cc = False  # Starts with //

    # Comment header block starts with /* (continue until */)
    starts_with_cs = False  # Starts with /*

    # Mark the start of where the source file should continue from after the new copyright(s)
    copyright_remove_index = -1

    # Is this an auto-generated csharp file
    is_cs_auto_generated = False

    # Does this contain the old/deprecated amazon copyright
    is_amazon_old_header_line = False

    # Does this file have the original crytek note
    has_original_crytek_note = False

    is_open_source = False

    line_index = 0

    # Examine the contents of the file
    file_content_len = len(original_file_content)

    # Special case.  There are some c-sharp files that start of with non-ascii characters for some reason. Strip it out
    # from only the first line
    if is_csharp:
        line = _script_non_printable(original_file_content[0])
        original_file_content[0] = line+'\n'

    # Flag to indicate the file starts with comments
    has_starting_comments = False

    # Flag to indicate we are inside a /* comment
    starting_code_index = 0

    # (EXT) flags
    ext_comment_divider_started = False
    has_ext_comment_divider = False
    has_ext_description = False
    ext_start_description_line = ext_end_description_line = 0

    # (PY) flags
    py_started_str_quotes = False

    # (LUA) flags
    lua_has_comment = False
    has_lua_description = False
    lua_start_description_line = lua_end_description_line = 0


    # First pass is to determine if there exists any comment blocks before any real code
    while line_index<file_content_len:

        line = original_file_content[line_index].strip()
        line_index += 1

        # Tally flags based on contents of each line

        # Check if the line has any original crytek copyright (non-amazon)
        if not is_original_crytek_header and _is_former_crytek_header(line):
            is_original_crytek_header = True

        # Check if the line has the current (correct) amazon header
        if not is_amazon_header and _is_amazon_header(line):
            is_amazon_header = True

        # Check if the line has the deprecated amazon header
        if not is_amazon_deprecated_header and _is_amazon_deprecated_header(line):
            is_amazon_deprecated_header = True

        # Check if the line has the TBD copyright placeholder
        if not is_copyright_notice_tbd and _is_copyright_notice_tbd(line):
            is_copyright_notice_tbd = True

        # Check if this is a c-sharp auto generated file
        if is_csharp and not is_cs_auto_generated and _is_cs_auto_generated(line):
            is_cs_auto_generated = True

        # Check if this has the previous amazon header (3rd party mention)
        if not is_amazon_old_header_line and _is_amazon_old_header_line(line) and is_amazon_header:
            is_amazon_old_header_line= True

        # Check if this has the original crytek file note
        if not has_original_crytek_note and _has_original_crytek_note(line):
            has_original_crytek_note = True

        if not is_open_source and _is_open_source(line,orig_filepath):
            is_open_source = True

        # C or CS or java uses C/C++ styled comments
        if is_cc or is_csharp or is_java or is_cf:

            if has_starting_comments and starts_with_cs:
                if '*/' in line:
                    starts_with_cs = False
                continue
            if line.startswith('//'):
                has_starting_comments = True
                continue
            elif line.startswith('/*'):
                has_starting_comments = True
                if not '*/' in line:
                    starts_with_cs = True
                continue
            elif line=='\n' or line.__len__()==0:
                continue
            elif (is_cc or is_cf) and line.startswith('#'):
                starting_code_index = line_index-1
                break
            elif is_csharp and (line.startswith('using') or line.startswith('namespace')):
                starting_code_index = line_index-1
                break
            elif is_java and (line.startswith('import') or line.startswith('namespace')):
                starting_code_index = line_index-1
                break
            else:
                log_skipped(orig_filepath,'CANNOT PARSE HEADER BLOCK')
                return False

        elif is_py:
            if line.startswith('"""'):
                has_starting_comments = True
                if py_started_str_quotes:
                    py_started_str_quotes = False
                else:
                    py_started_str_quotes = True
                continue
            elif line.endswith('"""'):
                if py_started_str_quotes:
                    py_started_str_quotes = False
                continue
            elif line.startswith('\'\'\''):
                has_starting_comments = True
                if py_started_str_quotes:
                    py_started_str_quotes = False
                else:
                    py_started_str_quotes = True
                continue
            elif line.endswith('\'\'\''):
                has_starting_comments = True
                if py_started_str_quotes:
                    py_started_str_quotes = False
                continue
            elif line.startswith('#'):
                has_starting_comments = True
                continue
            elif line == '\n' or line.__len__()==0:
                continue
            elif py_started_str_quotes:
                continue
            else:
                starting_code_index = line_index-1
                break
        elif is_ext:
            starting_code_index = 0
            if line.startswith('////'):
                # Check if we are between the comment dividers ////...
                if ext_comment_divider_started:
                    ext_comment_divider_started = False
                    has_ext_comment_divider = True
                    starting_code_index = line_index+1
                else:
                    ext_comment_divider_started = True
            elif line.startswith('//  Description:'):
                has_ext_description = True
                ext_start_description_line = ext_end_description_line = line_index-1
            elif has_ext_description and (line.startswith('// -----') or line=='//' or line.startswith('////')):
                ext_end_description_line = line_index-2
                has_ext_description = False
            elif line.startswith('//'):
                continue
            else:
                starting_code_index = line_index-1
                break
        elif is_lua:
            starting_code_index = 0
            if line == '\n' or line.__len__()==0:
                continue
            elif 'DESCRIPTION:' in line.upper():
                has_lua_description = True
                lua_start_description_line = lua_end_description_line = line_index-1
            elif has_lua_description and (line.startswith('-----') or line=='--'):
                lua_end_description_line = line_index-2
                has_lua_description = False
            elif line.startswith('--'):
                lua_has_comment = True
                continue
            else:
                starting_code_index = line_index-1
                break
        elif is_bat:
            if line == '\n' or line.__len__()==0:
                continue
            elif line.startswith('REM'):
                continue
            else:
                starting_code_index = line_index-1
                break

        else:
            return False

    # Determine if we need to update the file
    update_header = False

    # This has an original crytek copyright
    #   or deprecated amazon header or the copyright TBD header
    #   or TBD copyright
    #   AND is not the correct amazon header
    if (is_original_crytek or is_amazon_deprecated_header or is_copyright_notice_tbd or is_original_crytek_header) and not is_amazon_header:
        update_header = True
    # This has the right amazon header (initially)
    elif is_amazon_header:
        # This is an original crytek file but does not have the original crytek note
        if (is_original_crytek or is_original_crytek_header) and not has_original_crytek_note:
            update_header = True
        # The amazon header has the older 3rd party mention
        if is_amazon_old_header_line:
            update_header = True
    else:
        # This file either doesnt have a comment block, or there is no original crytek comment block.
        # We will just prepend from the beginning
        starting_code_index = 0
        update_header = True

    # If this is autogenerate cs, then never update the header
    if is_cs_auto_generated:
        log_skipped(orig_filepath,'AUTOGENERATED FILE')
        update_header = False

    # If this is open source, then never update the header
    if is_open_source:
        update_header = False

    if update_header:
        if is_csharp or is_cc or is_java or is_cf:
            updated_file_content.append(AMAZON_HEADER)
            if is_original_crytek:
                updated_file_content.append(FORMER_CRYTEK_HEADER+'\n\n')
        elif is_ext:
            updated_file_content.append(AMAZON_EXT_HEADER_TOP)
            if is_original_crytek:
                updated_file_content.append(FORMER_CRYTEK_HEADER+'\n')
                updated_file_content.append('//\n\n')
            if ext_start_description_line>0:
                desc_line = ext_start_description_line
                while desc_line <= ext_end_description_line:
                    updated_file_content.append(original_file_content[desc_line])
                    desc_line += 1
            updated_file_content.append(AMAZON_EXT_HEADER_BOTTOM)

        elif is_lua:
            updated_file_content.append(AMAZON_LUA_HEADER_TOP)
            if is_original_crytek or is_original_crytek_header:
                updated_file_content.append(LUA_FORMER_CRYTEK_HEADER+'\n')
                updated_file_content.append('--\n--\n')
            if lua_start_description_line>0:
                desc_line = lua_start_description_line
                while desc_line <= lua_end_description_line:
                    updated_file_content.append(original_file_content[desc_line])
                    desc_line += 1
            updated_file_content.append(AMAZON_LUA_HEADER_BOTTOM)


        elif is_py:
            updated_file_content.append(PY_AMAZON_HEADER)
            if is_original_crytek:
                updated_file_content.append(PY_FORMER_CRYTEK_HEADER+'\n')
                updated_file_content.append('#\n\n')

        elif is_bat:
            updated_file_content.append(BAT_AMAZON_HEADER+'REM\n')
            if is_original_crytek:
                updated_file_content.append(BAT_FORMER_CRYTEK_HEADER+'\n')
                updated_file_content.append('REM\n')
            updated_file_content.append('\n')
            if original_file_content[starting_code_index].lower().startswith('@echo off'):
                starting_code_index += 1

        line_index = starting_code_index
        while line_index<file_content_len:
            line = original_file_content[line_index]
            updated_file_content.append(line)
            line_index += 1

        if update_header:
            log_update(orig_filepath,'UPDATED')

    return update_header


def compare_files(left,right):
    subprocess.call('{} \"{}\" \"{}\"'.format(BEYOND_COMPARE_PATH,left,right))


def process(cl_number, p4_path, root_path, code_path, eval_only, show_diff, filtered_extensions, is_code_path_input_file):


    original_crytek_source = load_original_crytek_set('crytek_3.8.1_source.txt',filtered_extensions)

    # Collect the files to process
    files_to_process = set()
    if is_code_path_input_file:
        files_to_process = read_input_files_file(code_path,root_path)
    else:
        if os.path.isdir(code_path):
            for (dirpath, dirnames, filenames) in os.walk(code_path):
                if _is_skip_folder(root_path, dirpath):
                    continue
                for file in filenames:
                    file_name,file_ext = os.path.splitext(file)
                    file_ext = file_ext.upper()[1:]
                    if file_ext in filtered_extensions and not _is_skip_file(file):
                        files_to_process.add(dirpath + "/" + file)
        else:
            files_to_process.add(code_path)

    for file_to_process in files_to_process:
        file_to_process_normalized = file_to_process.replace('\\','/').upper()
        file_to_process_normalized = '/' + file_to_process_normalized.replace(root_path.replace('\\','/').upper(),'')

        # Determine if this is an original crytek file
        is_original_crytek = file_to_process_normalized.upper() in original_crytek_source

        # Open each file and handle in memory
        with open(file_to_process,'r') as f:
            file_content = f.readlines()
        updated_file_content = []

        # Process each file
        if process_file(file_to_process, file_content, updated_file_content, is_original_crytek):

            # If the file needs a header update, prepare a temp file for this and write the contents of the updated
            # file to the temp file
            temp_file_path = os.path.realpath(file_to_process + '_tmp')
            with open(temp_file_path, 'w') as w:
                for updated_file_line in updated_file_content:
                    w.write(updated_file_line)

            # Optional: Show a diff on the file before (optional) committing
            if show_diff:
                compare_files(file_to_process,temp_file_path)

            # If not just eval, check out the file and commit
            if not eval_only:
                if not check_file_status(file_to_process):

                    if not checkout_file(root_path,file_to_process,p4_path,cl_number):
                        logging.error('...Unable to check out file {}'.format(file_to_process))
                        print('...Unable to check out file {}'.format(file_to_process))
                        os.remove(temp_file_path)
                        continue
                os.remove(file_to_process)
                shutil.copyfile(temp_file_path,file_to_process)

            print('Processed {}'.format(file_to_process))

        else:
            print('Skipped {}'.format(file_to_process))

    close_logs()


if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument('-e','--eval', action='store_true', default=False, help='Evaluate only without processing')
    parser.add_argument('-d','--diff', action='store_true', default=False, help='Show diff window')
    parser.add_argument('-i','--input_files', action='store_true', default=False, help='Option to use code_path as an input file of files to process')

    parser.add_argument('-p','--py', action='store_true', default=False, help='Python files only (.py)')
    parser.add_argument('-c','--cpp', action='store_true', default=False, help='C/C++ files only')
    parser.add_argument('--cs', action='store_true', default=False, help='C# files only')
    parser.add_argument('--ext', action='store_true', default=False, help='.EXT files only')
    parser.add_argument('--lua', action='store_true', default=False, help='.LUA files only')
    #parser.add_argument('--js', action='store_true', default=False, help='.JS files only')
    parser.add_argument('--bat', action='store_true', default=False, help='.BAT files only')
    parser.add_argument('--cf', action='store_true', default=False, help='CFI/CFX files only')

    parser.add_argument('cl', type=int, help='Change list number to checkout the files to')
    parser.add_argument('p4_path', type=str, help='The base p4 path')
    parser.add_argument('root_path', help='The base root path')
    parser.add_argument('code_path', help='The root code path process on', default='.')

    args = parser.parse_args()

    print('P4PORT={}'.format(os.environ["P4PORT"]))
    print('P4CLIENT={}'.format(os.environ["P4CLIENT"]))
    print('Changelist #{}'.format(args.cl))

    if not os.path.exists(args.root_path):
        print('[ERROR]: Root path \'{}\' is invalid'.format(args.root_path))
        exit()
    else:
        root_path = args.root_path

    if not os.path.isdir(root_path):
        print('[ERROR]: Root path \'{}\' cannot be a file'.format(args.root_path))
        exit()

    print('Root path {}'.format(args.root_path))
    print('p4 path {}'.format(args.p4_path))

    if args.input_files:
        code_path = args.code_path
        if not os.path.exists(code_path) or not os.path.isfile(code_path):
            print('[ERROR]: Code input file \'{}\' is invalid'.format(args.code_path))
            exit()
    else:
        code_path = os.path.normpath(root_path+'/'+args.code_path)
        if not os.path.exists(code_path):
            print('[ERROR]: Code path \'{}\' is invalid'.format(args.code_path))
            exit()

    root_path = args.root_path

    if os.path.isdir(code_path):
        print('Working on code folder {}'.format(code_path))
    else:
        print('Working on a single code file {}'.format(code_path))

    filtered_extensions = []
    if args.py:
        print('Filtering on Python files')
        filtered_extensions.append('PY')
    if args.cpp:
        print('Filtering on C/C++ files')
        filtered_extensions.append('H')
        filtered_extensions.append('HPP')
        filtered_extensions.append('HXX')
        filtered_extensions.append('INL')
        filtered_extensions.append('C')
        filtered_extensions.append('CPP')
    if args.cs:
        print('Filtering on C# files')
        filtered_extensions.append('CS')
    if args.ext:
        print('Filtering on EXT files')
        filtered_extensions.append('EXT')
    if args.lua:
        print('Filtering on LUA files')
        filtered_extensions.append('LUA')
    if args.bat:
        print('Filtering on BAT files')
        filtered_extensions.append('BAT')
    if args.cf:
        print('Filtering on CFI/CFX files')
        filtered_extensions.append('CFI')
        filtered_extensions.append('CFX')
    if len(filtered_extensions)==0:
        filtered_extensions = DEFAULT_FILTERED_EXTENSIONS


    process(args.cl, args.p4_path, root_path, code_path, args.eval, args.diff,filtered_extensions,args.input_files)





