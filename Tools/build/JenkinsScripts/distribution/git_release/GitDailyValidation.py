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
import tempfile
import shutil
import sys
import os

THIS_SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_SCRIPT_DIRECTORY, ".."))  # Required for importing Git scripts
from GitRelease import mirror_repo_from_local
from GitIntegrityChecker import check_integrity, clone_from_url, IntegrityError, handleRemoveReadonly
from GitOpsCodeCommit import custom_clone
from GitOpsGitHub import create_authenticated_https_clone_url, are_credentials_valid

def parse_args():
    parser = argparse.ArgumentParser(description="Performs validation of Git repos, and restores if necessary.")
    parser.add_argument('--publicRepoURL',
                        help='The URL for the repository that we are validating the integrity of (expects a GitHub URL).',
                        required=True)
    parser.add_argument('--backupRepoURL',
                        help='The URL for the repository that we are baselining our check of the public repository against. (expects a CodeCommit URL).',
                        required=True)
    parser.add_argument('--hashFile',
                        help='Path to the file containing acceptable commit hashes.',
                        required=True)
    parser.add_argument('--githubUser',
                        required=True,
                        help='Username for the Github account.')
    parser.add_argument('--githubPassword',
                        required=True,
                        help='Password for the Github account.')
    parser.add_argument('--ccUser',
                        required=True,
                        help='Username for the CodeCommit account.')
    parser.add_argument('--ccPassword',
                        required=True,
                        help='Password for the CodeCommit account.')
    parser.add_argument('--dryRun',
                        help='Execute without performing any restore.',
                        action="store_true")
    args = parser.parse_args()
    return args


def validate_args(args):
    # ensure that backup repo isn't a github repo
    github_domain = 'github.com'
    if github_domain in args.backupRepoURL.lower():
        raise Exception('Cannot backup repo to another GitHub repo. Please use a git repo not on GitHub.')
    # ensure that destination repo is a github repo
    if github_domain not in args.publicRepoURL.lower():
        raise Exception('Cannot release to any repo other than one hosted on GitHub. Please use a git repo on GitHub.')

    if not os.path.exists(args.hashFile):
        raise Exception(f'Hash file not found: {args.hashFile}')

    if not are_credentials_valid(args.githubUser, args.githubPassword):
        raise Exception('Provided GitHub credentials are invalid.')


def restore_repo_from_backup(user, password, src_repo_url, dst_repo_url):
    # Make a temporary workspace for cloning the CodeCommit repo.
    temp_dir_path = tempfile.mkdtemp()
    try:
        clone_from_url(user, password, src_repo_url, temp_dir_path)
        mirror_repo_from_local(temp_dir_path, dst_repo_url)
    except:
        raise
    finally:
        # Remove the temporary cloning workspace.
        shutil.rmtree(temp_dir_path, ignore_errors=False, onerror=handleRemoveReadonly)

def main():
    args = parse_args()
    validate_args(args)

    github_repo_url = args.publicRepoURL
    github_backup_repo_url = args.backupRepoURL
    github_integrity_valid = True
    github_hash_list = []  # init as empty list.
    codecommit_integrity_valid = True
    codecommit_hash_list = []  # init as empty list

    #
    # Obtain integrity status
    #
    try:
        print("Checking GitHub repo integrity...")
        check_integrity(None, args.hashFile, False,
                        clone_from_url, args.githubUser, args.githubPassword, github_repo_url)
    except IntegrityError as error:
        print(error)
        github_integrity_valid = False
        github_hash_list = error.repo_hash_list

    try:
        print("Checking CodeCommit repo integrity...")
        check_integrity(None, args.hashFile, False,
                        clone_from_url, args.ccUser, args.ccPassword, github_backup_repo_url)
    except IntegrityError as error:
        print(error)
        codecommit_integrity_valid = False
        codecommit_hash_list = error.repo_hash_list

    #
    # Validate integrity and restore if necessary.
    #
    print("Inspecting repo integrity results...")
    if not codecommit_integrity_valid and not github_integrity_valid and codecommit_hash_list == github_hash_list:
        raise Exception("Internal and external mirrors are identical, but the hashlist is different. "
                        "Internal hashlist has been compromised! Intervene manually.")

    if not codecommit_integrity_valid:
        raise Exception("Internal Git mirror has been compromised. Intervene manually.")

    if not github_integrity_valid:
        print("GitHub repository has been compromised! Executing restore operation.")
        if args.dryRun:
            print("Dry run: Skip restore operation")
        else:
            authenticated_github_repo_url = create_authenticated_https_clone_url(args.githubUser, args.githubPassword, github_repo_url)
            restore_repo_from_backup(args.ccUser, args.ccPassword, github_backup_repo_url, authenticated_github_repo_url)

    # If we are here, all possible exceptions have been evaluated.
    # It is safe to assume all is well.
    print("Integrity validation succeeded.")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e)
        sys.exit(1)
