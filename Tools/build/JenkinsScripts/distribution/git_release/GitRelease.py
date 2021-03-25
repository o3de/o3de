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
import boto3
import sys
import shutil
import json
import os.path
import re
from subprocess import Popen, PIPE, check_output
from git import Repo

THIS_SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_SCRIPT_DIRECTORY, '..'))  # Required for importing Git scripts
from GitStaging import handle_remove_readonly, abort_operation
from GitOpsCodeCommit import custom_clone
from GitIntegrityChecker import HASHLIST_KEY
from GitOpsGitHub import create_authenticated_https_clone_url, are_credentials_valid
from GitOpsCommon import get_revision_list

def create_args():
    parser = argparse.ArgumentParser(description='Mirrors all content from the internal to external GitHubMirror.')
    parser.add_argument('--sourceRepoURL',
                        help='The URL for the repository that we are mirroring to GitHub (expects a CodeCommit URL).',
                        required=True)
    parser.add_argument('--destinationRepoURL',
                        help='The URL for the repository that we are mirroring on to (expects a GitHub URL).',
                        required=True)
    parser.add_argument('--backupRepoURL',
                        help='The URL for the repository that we are backing up the mirrored content to (expects a CodeCommit URL).',
                        required=True)
    parser.add_argument('--hashFile',
                        help='Hash file to update when release succeeds.',
                        required=True)
    parser.add_argument('--genRoot',
                        help='Directory for temp files.',
                        required=True)
    parser.add_argument('--githubUser',
                        help='Username for the Github account.',
                        required=True)
    parser.add_argument('--githubPassword',
                        help='Password for the Github account.',
                        required=True)
    parser.add_argument('--awsProfile',
                        help='AWS credentials profile generated from AWS CLI. Defaults to \'default\'.',
                        required=False,
                        default='default')
    parser.add_argument('--keep',
                        help='Keeps all intermediary files after operation completes.',
                        action='store_true',
                        required=False)
    parser.add_argument('--SkipP4Submit',
                        help='Skips the submission to P4 history.',
                        action='store_true',
                        required=False)
    return parser.parse_args()


def validate_args(args):
    # ensure that source and backup repos aren't github repos
    github_domain = 'github.com'
    if github_domain in args.sourceRepoURL.lower() or github_domain in args.backupRepoURL.lower():
        abort_operation('Cannot mirror from GitHub directly. Please use a git repo not on GitHub.')

    if github_domain in args.backupRepoURL.lower():
        abort_operation('Cannot backup repo to another GitHub repo. Please use a git repo not on GitHub.')

    # ensure that destination repo is a github repo
    if github_domain not in args.destinationRepoURL.lower():
        abort_operation('Cannot release to any repo other than one hosted on GitHub. Please use a git repo on GitHub.')

    # ensure aws profile exists on the machine
    if args.awsProfile:
        if boto3.Session(profile_name=args.awsProfile) is None:
            abort_operation('The AWS CLI profile name specified does not exist on this machine. Please specify an existing AWS CLI profile.')

    if not os.path.exists(args.hashFile):
        abort_operation(f'Hash file not found: {args.hashFile}. If using perforce, please make sure that the file is mapped to your workspace, is checked out, and is at the lastest revision.')

    if not are_credentials_valid(args.githubUser, args.githubPassword):
        abort_operation('Provided GitHub credentials are invalid. If you are using an account with Two-Factor Authentication enabled, your password should be replaced with the proper acces token.')


def mirror_repo_from_local(working_dir, dest_repo_url):
    repo = Repo(working_dir)

    # Perform a mirroring operation by pushing all refs into the remote repo.
    # This operation will delete stale refs and overwrite outdated ones on the remote repo.
    print(f'Pushing to remote: {dest_repo_url}')
    repo.git.push('--mirror', dest_repo_url)

    repo.close()


def mirror_repo_from_remote(working_dir, aws_profile, keep, source_repo_url, dest_repo_url):
    # We begin by 'cloning' the git repo from the internal GitHub mirror at CodeCommit. Normally a git clone would
    # elegantly set everything ready to mirror, but CodeCommit access is done via AWS CLI, meaning that the git repo
    # needs a particular configuration in order to communicate with CodeCommit. Said configuration can be setup on the
    # user's machine, but we inject the configuration directly into the repo initialization in order for the scripts to
    # be portable across machines.
    # A drawback with this method is that there are more manual steps to do (such as creating local branches for each
    # branch remote) in order to correctly mirror the repo to the external GitHub site.
    if os.path.exists(working_dir):
        shutil.rmtree(working_dir, onerror=handle_remove_readonly)
    os.makedirs(working_dir)

    # Clone the remote repo to mirror.
    custom_clone(source_repo_url, aws_profile, working_dir, True)

    # Perform the actual mirrorring operation.
    # Shortcut:
    #   Use alternate mirror function for code re-use.
    mirror_repo_from_local(working_dir, dest_repo_url)

    if not keep:
        print('Cleaning up temp files.')
        shutil.rmtree(working_dir, onerror=handle_remove_readonly)


def get_last_commit_info(repo_directory):
    initial_dir = os.getcwd()
    os.chdir(repo_directory)
    commit_info = check_output(['git', 'log', '-1', '--all', '--date-order', '--oneline', '--pretty=format:\'%H %s\''])
    os.chdir(initial_dir)

    # Sanitize string from stdout for Python usage
    commit_info = re.sub('[\'\\\]', '', commit_info)

    commit_info_split = commit_info.split(' ', 1)
    commit_hash = commit_info_split[0]
    commit_title = commit_info_split[1]
    print(f'Last commit info found from directory {repo_directory}:\nCommit hash: {commit_hash}\nCommit Title: {commit_title}')
    return commit_hash, commit_title


def create_git_release_p4_changelist(changelist_description):
    changelist_config = check_output(['p4', 'change', '-o'])
    changelist_config = changelist_config.replace('<enter description here>', changelist_description, 1)

    p = Popen(['p4', 'change', '-i'], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    result_stdout = p.communicate(input=changelist_config)[0]

    # Successful result prints changelist number (example: 'Change 452982 created.\r\n')
    changelist_number = result_stdout.split()[1]
    return changelist_number


def p4_update_hashlist(commit_hashes_filepath, hash_list, changelist_number):
    print(f'Checking out hashlist file for edit: {commit_hashes_filepath}')
    print(check_output(['p4', 'edit', '-c', changelist_number, commit_hashes_filepath]))

    print('Loading hashlist from file.')
    with open(commit_hashes_filepath) as json_data:
        json_obj_hashes = json.load(json_data)
    json_obj_hashes[HASHLIST_KEY] = hash_list

    # Replace the hashlist file with the new updated version
    print('Writting new hash list to file')
    os.remove(commit_hashes_filepath)
    with open(commit_hashes_filepath, 'w') as f:
        json.dump(json_obj_hashes, f, indent=4)


def submit_p4_changelist(changelist_number):
    print(f'Submitting P4 CL{changelist_number}...')
    print(check_output(['p4', 'submit', '-c', changelist_number]))


def main():
    args = create_args()
    validate_args(args)

    https_authenticated_url = \
        create_authenticated_https_clone_url(args.githubUser, args.githubPassword,
                                             args.destinationRepoURL)

    working_dir = os.path.join(args.genRoot, 'git_repo_release')

    # Mirror the repo to GitHub.
    mirror_repo_from_remote(working_dir, args.awsProfile, True,
                            args.sourceRepoURL,
                            https_authenticated_url)

    # Reuse the cloned repo to mirror once more to our internal backup
    mirror_repo_from_local(working_dir, args.backupRepoURL)

    # Do a rev-list to update the hashlist
    hash_list = get_revision_list(working_dir)

    # Get the last git commit to parse the version number. We need this number to fill the description of the Perforce
    # changelist used to update the hashlist. Ignore the 'commit_hash'.
    commit_hash, commit_title = get_last_commit_info(working_dir)

    p4_cl_number = create_git_release_p4_changelist('GitHub ' + commit_title)

    p4_update_hashlist(args.hashFile, hash_list, p4_cl_number)

    if not args.SkipP4Submit:
        submit_p4_changelist(p4_cl_number)
    else:
        print('Skipping P4 Submit.')

    if not args.keep:
        # Delete the repo
        shutil.rmtree(working_dir, onerror=handle_remove_readonly)

if __name__ == '__main__':
    main()
    sys.exit()
