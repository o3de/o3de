"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# One-time script to inspect pre-copyright script headers against the a particular branch@latest and optionally make
# corrections


import re
import os
import codecs
import stat
import subprocess
import argparse
import tempfile
import hashlib

# The path to beyond compare local to the machine
BEYOND_COMPARE_PATH="\"C:\\Program Files (x86)\\Beyond Compare 3\\BCompare.exe\""

# The P4 Path to mainline to get the previous revision
P4_MAINLINE = '//lyengine/dev'

# The revision number in mainline just before the original copyright header script was applied
PRECOPYRIGHT_REV = 130741

# The name of the file to keep track of files that were intentionally skipped
MANUAL_SKIP_FILE = 'manual_skipped.txt'

# The extension of files to analyze
DEFAULT_FILTERED_EXTENSIONS = ['CS','H','HPP','HXX','INL','C','CPP', 'EXT','PY', 'LUA', 'BAT', 'CFX', 'CFI']

# Words that we will ignore when analyzing the original copyright header for anything meaningful
TALLY_IGNORE_WORDS = ['COMPILERS',
                      'VISUAL',
                      'STUDIO',
                      'VERSION',
                      'CREATED',
                      'STUDIOS',
                      'FILE',
                      'SOURCE',
                      'COPYRIGHT',
                      'CRYTEK',
                      'C',
                      'CREATED',
                      'HISTORY']
# The word threshold to use to flag previous revisions for the number of non-ignore words that were detected.
WORD_THRESHOLD = 10

# Folders to skip during the process
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

# Particular files to skip
SKIP_FILES = ['resource.h',
              '__init__.py']


def calculate_hash(file_contents):
    m = hashlib.md5()
    for line in file_contents:
        m.update(line)
    return m.hexdigest()


def calculate_file_hash(file_path):
    with open(file_path,'r') as f:
        file_content = f.readlines()
    return calculate_hash(file_content)


# Compare 2 files with beyond compare
def compare_files(left,right):
    subprocess.call('{} \"{}\" \"{}\"'.format(BEYOND_COMPARE_PATH,left,right))


# Check the read-only flag of a file (assume it represents if a file is checked out or not)
def check_file_status(filepath):
    st = os.stat(filepath)
    return bool(st.st_mode & (stat.S_IWGRP|stat.S_IWUSR|stat.S_IWOTH))


# Checkout a file into a ChangeList
def checkout_file(root_code_path,filepath, p4_path_root, cl_number):
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


def replace_source_with_update_temp(source_original_file,source_temp_file):
    with open(source_temp_file,'r') as rf:
        file_content = rf.readlines()
    with open(source_original_file,'w') as wf:
        skipped_first = False
        for line in file_content:
            if not skipped_first:
                skipped_first = True
            else:
                wf.write(line)


def pull_source_revision(root_code_path, file_path, p4_path_root, rev_number):

    # Calculate the p4 root pathg
    p4_path = p4_path_root + '/' + file_path.replace(root_code_path, '')
    p4_path = p4_path.replace('\\','/')
    p4_path = '//' + p4_path[2:].replace('//','/')

    # If the rev number is greater than zero, then this is a specific version
    if rev_number>0:
        filename_only = os.path.split(file_path)[1] + '#{}'.format(rev_number)
        temp_file_path = os.path.join(tempfile.gettempdir(), filename_only)
        p4_command = ['p4','print','{}@{}'.format(p4_path, rev_number)]
    else:
        filename_only = os.path.split(file_path)[1]
        temp_file_path = os.path.join(tempfile.gettempdir(), filename_only)
        p4_command = ['p4','print','{}'.format(p4_path)]
    try:
        with open(temp_file_path,'w') as f:
            result = subprocess.call(p4_command, stdout=f)
        if result < 0:
            print('[ABORT] Process terminated by signal')
            return False
    except OSError as e:
        print('[ERROR] p4 call error:{}'.format(e))
        return False

    return temp_file_path


# Auto insert descriptions into a file if the file doesnt already have a description section
def auto_insert_description(file_to_modify,description_lines):
    file_contents = []
    with open(file_to_modify,'r') as r:
        file_contents = r.readlines()

    line_count = len(file_contents)
    line_index = 0
    insert_index = -1
    comment_block_end_index = -1
    has_description = False
    while line_index<line_count:
        check_line = file_contents[line_index].strip()
        if check_line=='*/' and comment_block_end_index<0:
            comment_block_end_index = line_index
        if check_line=='// Original file Copyright Crytek GMBH or its affiliates, used under license.':
            insert_index=line_index+1
            break
        if 'Description :' in check_line:
            has_description = True
            break
        line_index += 1

    # Already has a description, do not insert
    if has_description:
        return file_to_modify

    # No appropriate place to insert the description, abort
    if comment_block_end_index<0 and insert_index<0:
        return file_to_modify

    # Cannot locate the 'Original file Copyright..' line.  Add it automatically and set the insert index to right after
    if insert_index < 0:
        insert_index = comment_block_end_index+1
        file_contents.insert(insert_index,'\n')
        insert_index += 1
        file_contents.insert(insert_index,'// Original file Copyright Crytek GMBH or its affiliates, used under license.\n')
        insert_index += 1

    # Insert the description line(s) if we have a place to insert it
    if insert_index>0:
        file_contents.insert(insert_index,'\n')
        insert_index += 1
        for insert_description_line in description_lines:
            file_contents.insert(insert_index,insert_description_line+'\n')
            insert_index += 1
        file_contents.insert(insert_index,'\n')

        # update the working file
        with open(file_to_modify,'w') as w:
            w.writelines(file_contents)

    return file_to_modify


def _is_skip_file(filepath):
    if filepath.endswith('.Designer.cs'):
        return True
    normalized = os.path.dirname(filepath).replace('\\','/').upper()
    for skip_file in SKIP_FILES:
        if normalized.endswith('/'+skip_file.upper()):
            return True
    return False


def _is_skip_folder(root_code_path,dirname):

    normalized = '/'+dirname.replace(root_code_path,'').replace('\\','/').upper()
    for skip_path in SKIP_FOLDERS:
        if normalized.startswith(skip_path.upper()):
            return True
    return False


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


BOM_ENCODINGS = [ (codecs.BOM_UTF32),
                  (codecs.BOM_UTF16),
                  (codecs.BOM_UTF8)]


def extract_header(original_file_content):
    def _clean_bom(line):
        for bom in BOM_ENCODINGS:
            if bom in line:
                return re.sub('[^\040-\176]','',line)
        return line

    header_content = []
    for original_line in original_file_content:
        bom_cleaned_line = _clean_bom(original_line).strip()
        if bom_cleaned_line.startswith('#'):
            break
        if bom_cleaned_line.startswith('using'):
            break
        if bom_cleaned_line.startswith('import'):
            break
        if bom_cleaned_line.startswith('namespace'):
            break
        header_content.append(bom_cleaned_line)

    return  header_content


def analyze_header_content(source_file, show_tally, orig_file_path,skip_log, original_description_lines):

    # Read the contents of the file and extract the header
    with open(source_file,'r') as r:
        original_file_content = r.readlines()
    header_content = extract_header(original_file_content)

    if '#' in source_file:
        filename_and_ext_only = os.path.splitext(os.path.split(source_file)[1].split('#')[0])
    else:
        filename_and_ext_only = os.path.splitext(os.path.split(source_file)[1])[0]
    filename_only = filename_and_ext_only[0]
    ext_only = filename_and_ext_only[1]
    filename_only_upper = filename_only.upper()

    # Clean the header of comment tokens and non work tokens
    clean_header_content = []
    has_description = False
    skipped_first = False
    description_label_added = False
    found_first_complete_description = False
    for line in header_content:
        if not skipped_first:
            skipped_first = True
        else:
            # see if we can pull original description line
            if not found_first_complete_description:
                m_original_desc_one_line = re.match('(//|\\*)?(\\s*)(Description|Desc|'+filename_only+ext_only+')\\s*\\:\\s*(.*)',line)
                if m_original_desc_one_line is not None:
                    desc_label_key = m_original_desc_one_line.group(3)
                    if m_original_desc_one_line.group(4) is not None:
                        has_description_header = True
                        extracted_description = m_original_desc_one_line.group(4)
                        if extracted_description.strip().__len__()>0:
                            original_description_lines.append('// Description : {}'.format(m_original_desc_one_line.group(4)))
                            description_label_added = True
                            has_description = True
                        # If the description label key is the filename, then this is one line only
                        if desc_label_key=='Description':
                            found_first_complete_description = True
            else:
                multiline_desc_check = re.sub(r'[\W]',' ',line).split(' ')
                multiline_desc_check_words = [w.upper() for w in multiline_desc_check if w!='']
                if len(multiline_desc_check_words)>0:
                    if not description_label_added:
                        original_description_lines.append('// Description : {}'.format(re.sub(r'[\s/\\]',' ',line).strip()))
                        description_label_added = True
                        has_description = True
                    else:
                        original_description_lines.append('//               {}'.format(re.sub(r'[\s/\\]',' ',line).strip()))
                        has_description = True
                else:
                    found_first_complete_description = False

            updated_line = re.sub(r'[\W]',' ',line)
            if show_tally:
                print(updated_line)

            clean_header_content.append(updated_line)

    # Count the words we care about
    word_count = 0

    # Analyze each line
    for line in clean_header_content:
        # Split each line into capitalized words
        words = [w.upper() for w in line.split(' ') if w != '']
        for word in words:

            # Skip the filename itself
            if word==filename_only_upper:
                continue

            # Skip any ignore words
            if word in TALLY_IGNORE_WORDS:
                continue

            # Skip any numbers
            if re.match('\d{1,4}',word):
                continue

            word_count+=1

    if show_tally:
        print('\n\nTotal Words:{}'.format(word_count))

    # Keep track of files that were skipped but had at least 4 unmatched words for futher analysis
    if word_count > 4 and not has_description:
        skip_log.write('***********************************************************************\n')
        skip_log.write('** Skipping file {}:\n'.format(orig_file_path))
        skip_log.write('***********************************************************************\n')
        for skip_line in header_content:
            skip_log.write(skip_line+'\n')
        skip_log.write('\n\n\n\n\n\n')

    return has_description or word_count > WORD_THRESHOLD


def process_file(cl_number,p4_path_root,root_code_path,orig_filepath, original_file_content, show_details, skip_log, manual_skip_files, index):

    # Pull the previous revision from mainline into a temp file
    prev_file_content_file = pull_source_revision(root_code_path,orig_filepath,P4_MAINLINE,PRECOPYRIGHT_REV)

    original_description_lines = []
    if not analyze_header_content(prev_file_content_file, show_details, orig_filepath, skip_log, original_description_lines):
        return

    # Pull the latest revision
    source_revision_file = pull_source_revision(root_code_path,orig_filepath,p4_path_root,-1)
    if not os.path.exists(source_revision_file):
        print('...File not in perforce {}:{}',p4_path_root,orig_filepath)
        return

    # Keep track of the hash of the original file
    original_file_hash = calculate_file_hash(source_revision_file)

    # If we detected any description lines in the original, attempt to insert it into the temp copy
    if len(original_description_lines)>0:
        source_revision_file = auto_insert_description(source_revision_file,original_description_lines)

    # Bring up the diff viewer
    compare_files(prev_file_content_file, source_revision_file)

    # If a changelist is supplied, checkout and update the source file into the CL
    if cl_number > 0:
        # Calculate the hash of the sourece file again to detect any changes
        check_file_hash = calculate_file_hash(source_revision_file)

        # If the hashes differ, then we need to check out the original file, update it with the changes
        if check_file_hash!=original_file_hash:

            # Checkout the file if necessary
            if not check_file_status(orig_filepath):
                if not checkout_file(root_code_path,orig_filepath, p4_path_root,cl_number):
                    print('...Unable to check out file {}'.format(orig_filepath))
                    os.remove(source_revision_file)
                    os.remove(prev_file_content_file)
                    return
            # Update the original with the temp
            replace_source_with_update_temp(orig_filepath,source_revision_file)

            print('Updated {}'.format(orig_filepath))
        else:
            with open(MANUAL_SKIP_FILE,'a') as update_file:
                update_file.write(orig_filepath.lower()+'\n')
            manual_skip_files.add(orig_filepath.lower())
            print('Manually Skipped {}'.format(orig_filepath))

    # Check out the file if needed
    os.remove(source_revision_file)
    os.remove(prev_file_content_file)


def process(cl_number, p4_path, root_path, code_path, filtered_extensions, is_code_path_input_file,show_details,skip_log, manual_skip_files, skip_to):

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

    file_count = len(files_to_process)
    file_progress = 0

    for file_to_process in files_to_process:

        # Open each file and handle in memory
        with open(file_to_process,'r') as f:
            file_content = f.readlines()

        file_progress += 1

        # Process each file
        if file_progress<skip_to and skip_to>0:
            print('({}/{}) Skipping {}.  (Forced)'.format(file_progress,file_count,file_to_process))
            continue

        # Skip files that were already skipped previously
        if file_to_process.lower() in manual_skip_files:
            print('({}/{}) Skipping {}.  Already Manually Skipped'.format(file_progress,file_count,file_to_process))
            continue
        # Skip files that are already checked out
        if check_file_status(file_to_process):
            print('({}/{}) Skipping {}.  Already checked out'.format(file_progress,file_count,file_to_process))
            continue
        if process_file(cl_number, p4_path,root_path,file_to_process, file_content, show_details, skip_log, manual_skip_files, file_progress):
            print('({}/{}) Processed {}'.format(file_progress,file_count,file_to_process))
        else:
            print('({}/{}) Skipped {}'.format(file_progress,file_count,file_to_process))


if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument('-i','--input_files', action='store_true', default=False, help='Option to use code_path as an input file of files to process')

    parser.add_argument('-p','--py', action='store_true', default=False, help='Python files only (.py)')
    parser.add_argument('-c','--cpp', action='store_true', default=False, help='C/C++ files only')
    parser.add_argument('--cs', action='store_true', default=False, help='C# files only')
    parser.add_argument('--ext', action='store_true', default=False, help='.EXT files only')
    parser.add_argument('--lua', action='store_true', default=False, help='.LUA files only')
    parser.add_argument('--bat', action='store_true', default=False, help='.BAT files only')
    parser.add_argument('--cf', action='store_true', default=False, help='CFI/CFX files only')
    parser.add_argument('--details', action='store_true', default=False, help='Show details of the header analysis')

    parser.add_argument('-s','--skip',nargs='?',type=int,default=-1,help='The entry number to skip forward to')
    parser.add_argument('--cl',nargs='?',type=int,default=0,help='Change list number to apply updates to.  If not set (or zero), then modified files will not be checked out')

    parser.add_argument('p4_path', type=str, help='The base p4 path')
    parser.add_argument('root_path', help='The base root path')
    parser.add_argument('code_path', help='The root code path process on', default='.')

    args = parser.parse_args()

    print('Using Perforce environment:')
    print(' P4PORT={}'.format(os.environ["P4PORT"]))
    print(' P4CLIENT={}'.format(os.environ["P4CLIENT"]))

    if not os.path.exists(args.root_path):
        print('[ERROR]: Root path \'{}\' is invalid'.format(args.root_path))
        exit()
    else:
        input_root_path = args.root_path

    if not os.path.isdir(input_root_path):
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
        code_path = os.path.normpath(input_root_path+'/'+args.code_path)
        if not os.path.exists(code_path):
            print('[ERROR]: Code path \'{}\' is invalid'.format(args.code_path))
            exit()

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

    manual_skipped_files = set()
    if os.path.exists(MANUAL_SKIP_FILE):
        with open('manual_skipped.txt','r') as msf:
            manual_skipped_files_list = msf.readlines()
            for manual_skipped_file in manual_skipped_files_list:
                manual_skipped_files.add(manual_skipped_file.lower().strip())

    with open('skipped_logs','w') as skip_log:
        process(args.cl, args.p4_path, input_root_path, code_path, filtered_extensions,args.input_files, args.details,
                skip_log, manual_skipped_files,args.skip)

