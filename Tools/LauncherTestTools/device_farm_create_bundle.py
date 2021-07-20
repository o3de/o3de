"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Device Farm Create Bundle
"""

import argparse
import logging
import os
import shutil
import stat

logger = logging.getLogger(__name__)


def on_rm_error( func, path, exc_info):
    # path contains the path of the file that couldn't be removed
    # let's just assume that it's read-only and unlink it.
    os.chmod( path, stat.S_IWRITE )
    os.unlink( path )
    
def copy_python_code_tree(src, dest):
    shutil.copytree(src, dest, ignore=shutil.ignore_patterns('*.pyc', '__pycache__'))

def create_test_bundle(project, project_launcher_tests_folder, python_test_tools_folder):
    
    temp_folder = os.path.join('temp', project)
    
    # Place all artifacts to send to device farm in this output folder
    zip_output_folder = os.path.join(temp_folder, 'zip_output')
        
    # clear the old virtual env folder
    logger.info("deleting old zip folder ...")
    if os.path.isdir(zip_output_folder):
        logger.info("Removing virtual env folder \"{}\" ...".format(zip_output_folder))
        shutil.rmtree(zip_output_folder, onerror = on_rm_error)
    
    # create the output folder where we dump everything to be zipped up.
    os.makedirs(zip_output_folder)
    
    # core files to add (iOS won't be referenced on Android, but it won't hurt anything)
    core_files = [
        'run_launcher_tests.py',
        'run_launcher_tests_ios.py',
        'run_launcher_tests_android.py',
        os.path.join('..', '..', project, 'project.json')]
    for file in core_files:
        shutil.copy2(file, os.path.join(zip_output_folder, os.path.basename(file)))
        
    logger.info("Including test code ...")
    test_output_folder = os.path.join(zip_output_folder, 'tests')
    copy_python_code_tree(project_launcher_tests_folder, test_output_folder)

    # Copy remote console from PythonTestTools
    logger.info("Including python PythonTestTools remote console ...")
    shutil.copy2(
        os.path.join(python_test_tools_folder, 'shared', 'remote_console_commands.py'),
        os.path.join(test_output_folder, 'remote_console_commands.py'))
        
    # Zip the tests/ folder, wheelhouse/ folder, and the requirements.txt file into a single archive:    
    test_bundle_path = os.path.join(temp_folder, 'test_bundle')
    logger.info("Generating test bundle zip {} ...".format(test_bundle_path))
    shutil.make_archive(test_bundle_path, 'zip', zip_output_folder)
    
def main():

    parser = argparse.ArgumentParser(description='Create the test bundle zip file for use on the Device Farm.')
    parser.add_argument('--project', required=True, help='Lumberyard Project')
    parser.add_argument('--project-launcher-tests-folder', required=True, help='Absolute path of the folder that contains the test code source.')
    parser.add_argument('--python-test-tools-folder', required=True, help='Absolute path of the PythonTestTools folder.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
    
    create_test_bundle(args.project, args.project_launcher_tests_folder, args.python_test_tools_folder)

if __name__== "__main__":
    main()
