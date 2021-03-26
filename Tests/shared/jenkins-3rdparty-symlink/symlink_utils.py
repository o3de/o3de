"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import argparse
import logging
import os
import subprocess            

logger = logging.getLogger(__name__)


def create_symlink(path, name, reference):
    """
    Checks if the defined symlink exists and returns True if it does. If it does not, it will create a
    new symlink and return True. Unsuccessful commands will return False.
    :param path: Path to where symlink should be created.
    :param name: Name of the symlink.
    :param reference: Source path of directory.
    """
    sym_path = os.path.join(path, name)
    
    if not os.path.exists(sym_path):
        if os.path.isfile(reference):
            proc = subprocess.Popen('cmd /c mklink "{}" "{}"'.format(name, reference), cwd=path,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        elif os.path.isdir(reference):
            proc = subprocess.Popen('cmd /c mklink /J "{}" "{}"'.format(name, reference), cwd=path,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        exit_code = proc.wait()

        if exit_code == 0:
            logger.info("Successfully created symlink from {} to {}".format(reference, sym_path))
            return True
        elif "You do not have sufficient privilege to perform this operation" in proc.stderr.read():
            raise AssertionError("Permissions denied. You should be running this script with Admin rights.")

        else:
            raise AssertionError("The command could not run successfully.")
            
    else:
        logger.info("Directory or file already exists: {}".format(os.path.join(path, name)))
        return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Creates 3rdParty symlink.')
    parser.add_argument('-symlink_path', metavar='symlink_path', type=str, required=True,
                        help='The path to create the 3rdParty symlink. If folder exists, symlinks will be'
                             'created inside the 3rdParty folder.')
    parser.add_argument('-name', metavar='symlink_name', type=str, default='3rdParty',
                        help='Name of symlink created. Defaults to 3rdParty.')
    parser.add_argument('-source_path', metavar='source_path', type=str, required=True,
                        help='The path where the source 3rdParty is located.')        
    args = parser.parse_args()
    
    sym_path = os.path.join(args.symlink_path, args.name)

    # If there is an existing directory, copy all source contents as symlinks.
    if not create_symlink(args.symlink_path, args.name, args.source_path):
        for file_dir in os.listdir(args.source_path):
            create_symlink(sym_path, file_dir, os.path.join(args.source_path, file_dir))
