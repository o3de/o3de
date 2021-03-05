#!/usr/bin/env python
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

import argparse
from datetime import datetime, timezone
import hashlib
import os
import pathlib
import platform
import re
import sys
import subprocess
import zipfile

'''
Creates zip file of <BuildDir>/BenchmarkResults folder and sends the the zip via email to team email
'''

def compute_sha256(input_filepath):
    '''
    Computes a SHA-2 hash using a digest that is 256 bits

    Args:
        input_filepath: File whose content will be hashed using the SHA-2 hash function

    Returns:
        bytes: byte array containing hash digest in hex
    '''
    hasher = hashlib.sha256()
    hash_result = None

    CHUNK_SIZE = 128 * (1 << 10) # Chunk Size for Sha256 hashing reads file in chunks of 128 KiB
    with open(input_filepath, 'rb') as hash_file:
        buf = hash_file.read(CHUNK_SIZE)
        hasher.update(buf)
        hash_result = hasher.hexdigest()

    return hash_result

def create_sha256sums_file(input_filepath):
    '''
    Create a sha256sums file from the contents of the input_filepath
    The sha256sums file will be named using the input_filepath path with an added
    extension of .sha256sums

    Args:
        input_filepath: File whose content will be hashed using the SHA-2 hash function

    Returns:
        string: Path to sha256sums file
    '''
    sha256_hash = compute_sha256(input_filepath)
    if not sha256_hash:
        print(f'Unable to compute sha256 hash for file {input_filepath}')
        return None

    hash_filepath = f'{input_filepath}.sha256sums'
    with open(hash_filepath, "wb") as archive_hash_file:
        new_hash_contents = f'{sha256_hash} *{os.path.basename(input_filepath)}\n'
        archive_hash_file.write(new_hash_contents.encode("utf8"))

    return hash_filepath

def get_files_to_archive(base_dir, regex):
    '''
    Gathers list of filepaths to add to archive file
    Files are looked up with the base directory and cross checked against the supplied regular expression
    which acts as an inclusion filter

    Args:
        base_dir: Directory to scan for files
        regex: Regular expression that is matched against each filename to determine if the file should be
               added to the archive
    '''
    # Get all file names in base directory
    with os.scandir(base_dir) as dir_entry:
        filepaths = [pathlib.PurePath(entry.path) for entry in dir_entry if entry.is_file()]
        # Get all file names matching the regular expression, those file will be added to zip archive
        archive_files = [str(filepath) for filepath in filepaths if re.match(regex, filepath.as_posix())]
        return archive_files
    return None


def create_archive_file(archive_file_prefix, input_filepaths, base_dir):
    '''
    Creates a zip file using the supplied input files
    LZMA compression is used by default for the zip file compression

    Args:
        archive_file_prefix: Prefix to use as the name of the zip file that should be created
        input_filepaths: List of input file paths that will be added to zip file
        base_dir: Directory which is used to create relative paths for each input file path from
    '''
    try:
        zipfile_name = '{}-{:%Y%m%d_%H%M%S}.zip'.format(archive_file_prefix,datetime.now(timezone.utc))
        # The lzma shared library isn't installed by default on Mac.
        compression_type = zipfile.ZIP_LZMA if platform.system() != 'Darwin' else zipfile.ZIP_BZIP2
        with zipfile.ZipFile(zipfile_name, mode='w', compression=compression_type) as benchmark_archive:
            zipfile_name = benchmark_archive.filename
            for input_filepath in input_filepaths:
                # Make input files relative to base_dir when storing them as archived names
                input_filepath_relpath = os.path.relpath(input_filepath, start=base_dir)
                benchmark_archive.write(input_filepath, input_filepath_relpath)
    except OSError as err:
        print(f'Failed to write benchmark files to zip archive with error {err}')
        sys.exit(1)
    except RuntimeError as zip_err:
        print(f'Runtime Error in zipfile module {zip_err}')
        sys.exit(1)
    return zipfile_name

def upload_to_s3(upload_script_path, base_dir, path_regex, bucket, key_prefix):
    '''
    Uploads files which located within the base directory using the upload_to_s3.py script

    Args:
        base_dir: The directory to pass as the --base-dir value to the upload_to_s3.py script
        path_regex: The regular expression to pass to the upload_to_s3.py script --file-regex parameter
        bucket: The s3 bucket to use for the --bucket argument for upload_to_s3.py 
        key_prefix: The prefix to store the uploaded files to within the s3 bucket, 
                    It is passed --key-prefix argument to upload_to_s3.py
    '''
    try:
        subprocess.run(['python', upload_script_path, '--base_dir',
            base_dir, '--file_regex', path_regex,
            '--bucket', bucket, '--key_prefix', key_prefix],
            check=True)
    except subprocess.CalledProcessError as err:
        print(f'{upload_script_path} failed with error {err}')
        sys.exit(1)

def upload_benchmarks(args):
    '''
    Main function responsible for determine which files to add to the output zip file and uploading
    the results to s3

    Args:
        args: Parse argument list of python command line parameters using the argparse module
    '''
    files_to_archive = get_files_to_archive(args.base_dir, args.file_regex)
    archive_zip_path = create_archive_file(args.output_prefix, files_to_archive, args.base_dir)
    # Create Sha256sum hash file of zip
    create_sha256sums_file(archive_zip_path)
    
    upload_dir = str(pathlib.Path(archive_zip_path).parent)
    upload_regex = fr'{pathlib.Path(archive_zip_path).name}.*'
    upload_to_s3(args.upload_to_s3_script_path, upload_dir, upload_regex, args.bucket, args.key_prefix)

def parse_args():
    cur_dir = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument("--base_dir", default=os.getcwd(), help="Base directory to files which should be archived, If not given, then current directory is used.")
    parser.add_argument("--upload-to-s3-script-path", default=os.path.join(cur_dir, 'upload_to_s3.py'), help="Path to upload_to_s3.py script. Script is used for uploading benchmarks to s3")
    parser.add_argument("--file_regex", default=r'.*BenchmarkResults/.+\.json', help="Regular expression that used to match file names to archive.")
    parser.add_argument("-o", "--output-prefix", default='benchmarks_results', help="Prefix to use to construct the name of the zip file where the benchmark results are zipped."
        " A timestamp will be added to the end of filename")
    parser.add_argument("--bucket", dest="bucket", default='ly-jenkins-cmake-benchmarks', help="S3 bucket the files are uploaded to.")
    parser.add_argument("-k", "--key_prefix", default='user_build', dest="key_prefix", help="Object key prefix.")
    args = parser.parse_args()

    return args

if __name__ == '__main__':
    args = parse_args();
    upload_benchmarks(args)
