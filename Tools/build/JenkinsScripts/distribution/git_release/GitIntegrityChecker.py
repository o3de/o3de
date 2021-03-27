############################################################################################
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
# a third party where indicated.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#############################################################################################
import argparse
import errno
import json
import os
import shutil
import stat
import subprocess
import tempfile
import sys

THIS_SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_SCRIPT_DIRECTORY, ".."))  # Required for importing Git scripts
from GitOpsGitHub import create_authenticated_https_clone_url
from GitOpsCommon import get_revision_list

HASHLIST_KEY = 'Hashlist'


class IntegrityError(RuntimeError):
    """Exception type for failed integrity check."""
    def __init__(self, message, file_hash_list, repo_hash_list):
        self.message = message
        self.hash_list = file_hash_list
        self.repo_hash_list = repo_hash_list


def handleRemoveReadonly(func, path, exc):
    """
    Python has issues removing files and directories on Windows
    (even if we've just created them) if they were set to 'readonly'.
    This usually occurs when deleting a '.git' directory, because some internal
    git repository files become 'readonly' when initializing a new repo.
    The following function should override general permission issues when
    deleting.
    """
    excvalue = exc[1]
    if func in (os.rmdir, os.remove) and excvalue.errno == errno.EACCES:
        os.chmod(path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO) # 0777
        func(path)
    else:
        raise


def validate_args(args):
    all_github_args_valid = (args.githubUser is not None and
                             args.githubPassword is not None)
    assert (all_github_args_valid or args.gitLocation is not None), 'Please provide GitHub information, ' \
                                                                    'or a path to a git repository on disk.'
    if all_github_args_valid and args.gitLocation is not None:
        print("Warning: A GitHub username and a path to a git repository have been provided. These commands are not " \
              "compatible, and this script will default to using provided GitHub credentials.")

    # If a working directory was given, verify it exists and is empty.
    if args.workingDirectory is not None:
        assert (os.path.exists(args.workingDirectory)), 'If using the working directory argument, please provide a ' \
                                                        'directory that exists on disk.'
        assert (os.listdir(args.workingDirectory) == []), 'Please provide an empty working directory.'


def parse_args():
    parser = argparse.ArgumentParser(description="Compares the commit hashes of a git repository against"
                                                 "a known good hash list.")
    parser.add_argument('-gitRepoURL',
                        help='The URL for the repository that we are checking the integrity of.',
                        required=True)
    parser.add_argument('--hashFile',
                        help='Path to the file containing acceptable commit hashes.',
                        required=True)

    # Either a Github username and password need to be passed in, or a location of a git install on disk.
    parser.add_argument('--githubUser',
                        default=None,
                        help='Username for the Github account.')
    parser.add_argument('--githubPassword',
                        default=None,
                        help='Password for the Github account.')
    parser.add_argument('--preserveGithubClone',
                        help='If using Github, preserves the cloned data on disk for inspection by the user.',
                        required=False,
                        action="store_true")

    parser.add_argument('--gitLocation',
                        default=None,
                        help='Path to a git repository on disk. Cloning will not occur if this is set.')

    parser.add_argument('--workingDirectory',
                        default=None,
                        help='Path to a temporary working directory. If not supplied, tempfile.mkdtemp will be used.')
    args = parser.parse_args()
    validate_args(args)
    return args


def load_json_hashes(json_file_path):
    file_data = open(json_file_path, 'r')
    json_file_data = json.load(file_data)
    file_data.close()
    file_hashes = json_file_data[HASHLIST_KEY]
    return file_hashes


def clone_from_url(github_user, github_password, github_repo, root_dir=None):
    if root_dir is None:
        root_dir = os.getcwd()
    if os.path.exists(root_dir) and os.path.isdir(root_dir):
        print("Cloning from GitHub into directory:\n" + root_dir)
        authenticated_clone_url = create_authenticated_https_clone_url(github_user,github_password, github_repo)
        subprocess.call(["git", "clone", "--no-checkout", authenticated_clone_url, root_dir])
    else:
        raise Exception(root_dir, "Provided path is not a valid directory on disk.")


def validate_hash_counts_match(git_hashes, json_hashes):
    return len(git_hashes) == len(json_hashes)


def validate_hashes_match(git_hashes, json_hashes):
    for git_hash, json_hash in list(zip(git_hashes, json_hashes)):
        if git_hash != json_hash:
            return False, git_hash, json_hash
    return True, None, None


def check_integrity(working_directory, hash_file, preserve_github_clone, git_clone_function, *extra_args):
    """
    Checks the integrity of a specified repo. Will raise an exception if integrity fails.

    :param working_directory: Where the repo will be cloned.
    :param hash_file: The hashfile to compare against for verifying integrity.
    :param preserve_github_clone: True, if we want to keep the repo on disk after integrity check.
    :param git_clone_function: A clone operation function. GitHub & CodeCommit clone differently due to authentication.
    :param extra_args: Arguments for the clone operation function.
    :return:
    """

    # Create a sub-folder for easy cleanup.
    if working_directory is not None:
        local_git_location = os.path.join(working_directory, "temp_git_repo")
        os.makedirs(local_git_location)
    else:
        local_git_location = tempfile.mkdtemp()

    # Change directory to the intended location before cloning. Regardless of success or fail,
    # we must change back to inital directory and delete the temp repo, if necessary. It is important
    # that we return to the initial directory because this function may be called from other Python modules.
    # We will use try/finally to ensure we always return to the initial directory.
    try:
        initial_dir = os.getcwd()
        os.chdir(local_git_location)
        git_clone_function(*extra_args)

        json_hashes = load_json_hashes(hash_file)

        # This git logging function results in all hashes for the repository printed out, one per line.
        git_hashes = get_revision_list(local_git_location)

    finally:
        # No more Git operations to be made, restore CWD
        os.chdir(initial_dir)

        # Once we have the list of hashes, we can clean up all of the temp files that were created.
        if not preserve_github_clone:
            print("Deleting cloned Git repository.")
            shutil.rmtree(local_git_location, ignore_errors=False, onerror=handleRemoveReadonly)

    if not validate_hash_counts_match(git_hashes, json_hashes):
        exception_message = "ERROR: Length of hash lists do not match. There are " + \
                            str(len(git_hashes)) + " git commits, and " + str(len(json_hashes)) + \
                            " hashes in the passed in JSON file."
        raise IntegrityError(exception_message, json_hashes, git_hashes)

    hash_result, git_hash, json_hash = validate_hashes_match(git_hashes, json_hashes)

    if not hash_result:
        exception_message = "ERROR: Hashes do not match. Git hash '" + git_hash + "'. JSON hash '" + json_hash + "'"
        raise IntegrityError(exception_message, json_hashes, git_hashes)

    print("All hashes match.")


def main():
    args = parse_args()
    check_integrity(
        args.workingDirectory,
        args.hashFile,
        args.preserveGithubClone,
        clone_from_url,
        args.githubUser,
        args.githubPassword,
        args.gitRepoURL
    )
if __name__ == "__main__":
    main()
