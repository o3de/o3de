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
from P4 import P4
import argparse
import boto3
import os
import sys
import shutil
from importlib import reload
from urllib.parse import urlparse
from git import Repo, RemoteProgress
from git.repo.base import InvalidGitRepositoryError, NoSuchPathError

THIS_SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_SCRIPT_DIRECTORY, ".."))  # Required for AWS_PyTools
from GitStaging import handle_remove_readonly, clean_replace_repo_contents
from GitOpsCodeCommit import custom_clone, init_git_repo
from GitMoveDetection import MoveDetection


class MyProgressPrinter(RemoteProgress):
    def update(self, op_code, cur_count, max_count=None, message=''):
        print(op_code, cur_count, max_count, cur_count / (max_count or 100.0), message or "NO MESSAGE")


def create_args():
    parser = argparse.ArgumentParser(description='Promotes a specified release from the \'SignedBuilds\' repository.')
    parser.add_argument('--sourceRepoURL',
                        help='The URL for the repository that we are taking the contents of the promotion from.',
                        required=True)

    parser.add_argument('--destinationRepoURL',
                        help='The URL for the repository that we are promoting content to.',
                        required=True)

    parser.add_argument('--commitRef',
                        help='A valid Git reference from the staging repo to use as a base for building the new commit.'
                             ' Usually a Perforce changelist number (the staging repo\'s tag names).',
                        required=True)

    parser.add_argument('--localRepoDirectory',
                        help='Path to the local repository where the Git work takes place. '
                             'If the directory does not exist, it will be created. '
                             'If the directory contains no repository, a new local clone will be created.',
                        required=True)

    parser.add_argument('--genRoot',
                        help='Directory for temp files.',
                        default="",
                        required=True)

    parser.add_argument('--dryRun',
                        help='Runs without pushing or modifying remotes.',
                        action="store_true",
                        required=False)

    parser.add_argument('--clean',
                        help='Runs with a clean slate. Deletes any pre-existing files that may cause conflicts.',
                        action="store_true",
                        required=False)

    # we should assume that the account containing staging repo is the same as the account containing promotion repo
    parser.add_argument('--awsProfile',
                        help='AWS credentials profile generated from AWS CLI. Defaults to \'default\'.',
                        required=False,
                        default='default')
    return parser.parse_args()


def validate_args(args):
    #ensure that source and dest repos aren't github repos
    github_domain = "github.com"
    if github_domain in args.sourceRepoURL.lower() or github_domain in args.destinationRepoURL.lower():
        abort_operation("Cannot promote to or from GitHub directly. Please use a git repo not on GitHub.")

    #ensure that repo urls don't have a trailing slash
    if args.sourceRepoURL.endswith('/'):
        args.sourceRepoURL = args.sourceRepoURL[:-1]
    if args.destinationRepoURL.endswith('/'):
        args.destinationRepoURL = args.destinationRepoURL[:-1]

    # ensure aws profile exists on the machine
    if args.awsProfile:
        if boto3.Session(profile_name=args.awsProfile) is None:
            abort_operation("The AWS CLI profile name specified does not exist on this machine. Please specify an existing AWS CLI profile.")


def get_repo_name(url):
    url_path = urlparse(url).path
    return os.path.split(url_path)[-1]


def generate_workspace_repo(local_repo_directory, aws_profile_name, source_repo_name, source_repo_url, dest_repo_url):
    repo_url = dest_repo_url
    init_git_repo(repo_url, aws_profile_name, local_repo_directory)
    repo = Repo(local_repo_directory)
    repo.git.fetch('origin')
    repo.git.fetch('origin', '--tags')

    # Add remote for 'signed release builds' repo
    repo.create_remote(source_repo_name, source_repo_url)
    repo.git.fetch(source_repo_name)
    repo.git.fetch(source_repo_name, '--tags')
    return repo


def branch_name_from_version_string(version_string):
    version_split = version_string.split(".")
    return f"{version_split[0]}.{version_split[1]}"


def rename_tag(old_name, new_name, remote_name, repo, dryRun):
    # Create copy of old tag
    repo.git.tag(new_name, old_name)
    if not dryRun:
        repo.git.push(remote_name, new_name)
    # Delete old tag
    repo.git.tag("-d", old_name)
    if not dryRun:
        repo.git.push(remote_name, ":" + old_name)


def init_mix_repo(local_repo_directory, aws_profile_name, source_repo_name, source_repo_url, dest_repo_url):
    # Acquire git repo with dependent remotes
    try:
        mix_repo = Repo(local_repo_directory)

        # Delete all the local tags before we fetch to ensure tags are synchronized.
        for tag in mix_repo.tags:
            mix_repo.delete_tag(tag)
        mix_repo.remote('origin').fetch(progress=MyProgressPrinter())
        mix_repo.remote(source_repo_name).fetch(progress=MyProgressPrinter())
    except InvalidGitRepositoryError:
        print("No local repo in specified directory. Deleting local contents & creating repo from remote.")
        shutil.rmtree(local_repo_directory, onerror=handle_remove_readonly)
        mix_repo = generate_workspace_repo(local_repo_directory, aws_profile_name, source_repo_name, source_repo_url, dest_repo_url)
    except NoSuchPathError:
        print("Local directory does not exist. Creating new directory and local repo within it...")
        mix_repo = generate_workspace_repo(local_repo_directory, aws_profile_name, source_repo_name, source_repo_url, dest_repo_url)
    return mix_repo


def ensure_tag_exists(repo, commit_ref, source_repo_url, dest_repo_url):
    if commit_ref not in repo.tags:
        available_tags = ('\n'.join(str(p) for p in repo.tags))
        raise Exception("ERROR: '{0}' tag does not exist in '{1}' or '{2}' repositories. \
                Has it already been promoted?\n\nTags available:\n{3}".format(
            commit_ref,
            source_repo_url,
            dest_repo_url,
            available_tags))


def get_ly_version_from_mirror_repo(mirror_repo):
    # Find tag for the specified commit of GitHubMirror repo.
    version_tag = get_tag_for_commit(mirror_repo, mirror_repo.head.commit)
    return str(version_tag)[1:]  # drop the 'v' from tag string.


def get_tag_for_commit(repo, commit):
    return next((tag for tag in repo.tags if tag.commit == commit), None)


def find_cl_for_ly_version_from_staging_repo(ly_version_string, mix_repo, repo_remote_name):
    """
    Get last promoted commit of version branch in SignedBuilds repo
    :param ly_version_string: string of a lumberyard version (X.X.X.X)
    :param mix_repo: repository object containing a remote to the SignedBuilds repo.
    :param repo_remote_name: Alias for the git remote repository representing the staging repo.
    :return: The string value of the changelist number.
    """
    refs = mix_repo.remotes[repo_remote_name].refs
    staging_repo_version_branch = refs[ly_version_string]
    staging_repo_version_branch_head = staging_repo_version_branch.commit

    # Traverse all commits in the branch to find a match for '*-Promoted'.
    commit = staging_repo_version_branch_head
    while commit:
        # If we have a promoted tag...
        commit_tag = get_tag_for_commit(mix_repo, commit)
        if commit_tag is not None and 'Promoted' in commit_tag.name:
                # Return the int value of the CL number
                return ''.join(filter(str.isdigit, commit_tag.name))

        if len(staging_repo_version_branch_head.parents) > 1:
            raise Exception(f'Commit {commit} has more than one parent: {staging_repo_version_branch_head.parents}')

        commit = staging_repo_version_branch_head.parents[0]

    raise Exception('Could not find tagged release. Dev Error')


def generate_move_commit(mirror_repo, branch_name_src, build_number_src, branch_name_dst, build_number_dst):
    """
    Performs a Git commit containing only file moves between two Lumberyard releases.

    :param mirror_repo:
        GitPython repository reference
    :param branch_name_src:
        Name of the SOURCE Perforce branch
    :param build_number_src:
        Build/Changelist number within the SOURCE branch
    :param branch_name_dst:
        Name of the DESTINATION Perforce branch
    :param build_number_dst:
        Build/Changelist number within the DESTINATION branch
    :return:
    """

    # We want to skip generating a move-commit if there is no prior history to create a range from. This condition can
    # be present in any branch, not just the repo as a whole.
    # At the time of execution, we expect at least 1 commit already present. The existing commit represents the previous
    # commit, whereas the incoming commit represents the next commit. In this function, we create the commit that is
    # lodged in the middle; the move-commit.
    rev_list_count = int(mirror_repo.git.rev_list('HEAD', '--count'))
    if rev_list_count < 1:
        raise Exception('Cannot promote an empty branch.')

    move_detection = MoveDetection()
    file_moves = move_detection.generate_list_files_moved_between_branches((branch_name_src, build_number_src),
                                                                           (branch_name_dst, build_number_dst))
    if len(file_moves) == 0:
        print('No files moved between releases. Skipping move-commit generation.')
    else:
        skipped_files = list()
        for move in file_moves:
            filename_before = os.path.join(mirror_repo.working_dir, move[0])
            filename_after = os.path.join(mirror_repo.working_dir, move[1])

            # If the old file is not found, it's likely due to a file move happening outside the repo's tracked directory.
            # Such a case occurs when files in the additive zip have moved/renamed. We don't care about these files.
            if not os.path.exists(filename_before):
                skipped_files.append(move)
                continue

            filename_after_dir = os.path.dirname(filename_after)
            if not os.path.exists(filename_after_dir):
                os.makedirs(filename_after_dir)
            
            # Git is attempting to move to an existing file. Skip this move.
            if os.path.exists(filename_after):
                continue

            mirror_repo.git.mv(filename_before, filename_after)

        print('Skipped processing the following moves not tracked by the git repository:')
        for entry in skipped_files:
            print(entry)

        mirror_repo.index.commit("Move Commit")


def format_p4_branch_from_ly_version(ly_version):
    split_version = ly_version.split('.')
    parsed_version = f'{split_version[0].zfill(2)}_{split_version[1].zfill(2)}'
    return f'//lyengine/releases/ver{parsed_version}/'


def find_ly_version_for_p4_cl(repo, p4_cl):
    tag_ref = repo.tag('refs/tags/' + p4_cl)
    branch_list = repo.git.branch('-r', '--contains', tag_ref.commit)
    branch_list = branch_list.split()

    # We assume the latest branch in the list is always the correct version.
    latest_branch = branch_list[0]

    # Return right-hand split of [remote]/[branch name].
    return latest_branch.split('/')[1]


def main():
    args = create_args()
    validate_args(args)

    if args.clean:
        print("Running clean. Deleting pre-existing local repo.")
        if os.path.exists(args.localRepoDirectory):
            shutil.rmtree(args.localRepoDirectory, onerror=handle_remove_readonly)

    previous_lumberyard_version = None
    next_lumberyard_version = None
    source_repo_name = get_repo_name(args.sourceRepoURL)
    mix_repo = init_mix_repo(args.localRepoDirectory, args.awsProfile, source_repo_name, args.sourceRepoURL, args.destinationRepoURL)
    remote_origin_refs = mix_repo.remote().refs

    ensure_tag_exists(mix_repo, args.commitRef, args.sourceRepoURL, args.destinationRepoURL)

    # Checkout the to-be-promoted commit
    mix_repo.git.checkout(args.commitRef)

    # Import Lumberyard version data.
    sys.path.append(os.path.join(mix_repo.working_dir, 'dev'))
    import waf_branch_spec
    next_lumberyard_version = waf_branch_spec.LUMBERYARD_VERSION
    mirror_repo_branch_name = branch_name_from_version_string(next_lumberyard_version)
    should_create_version_branch = False
    empty_master_branch = False

    # We always delete the local repository, regardless of the '--clean' flag because reusing a git repo requires
    # extensive sanitation to guarantee safe usage. It's easier to just clone a new one specifically for our purposes.
    if os.path.exists(args.genRoot):
        print(f"Path: {args.genRoot}\nFound pre-existing temp directory. Clearing contents before proceeding...")
        shutil.rmtree(args.genRoot, onerror=handle_remove_readonly)

    # We want to move all files of the to-be-promoted commit in a separate directory. This leaves the working directory
    # bare.
    print ("Copying repo contents to temp directory.")
    os.makedirs(args.genRoot)
    exclude_git_dir = os.path.join(args.localRepoDirectory, ".git")
    clean_replace_repo_contents(args.localRepoDirectory, args.genRoot, [exclude_git_dir])

    # Checkout the corresponding (mirror repo) branch which our new commit will be based off of.
    # We must determine if we branch off from a canonical version branch or from 'master'.
    if hasattr(mix_repo.heads, mirror_repo_branch_name):
        print(f"Checking out local branch '{mirror_repo_branch_name}'")
        mix_repo.heads[mirror_repo_branch_name].checkout()
    elif hasattr(remote_origin_refs, mirror_repo_branch_name):
        print(f"Checking out remote branch '{mirror_repo_branch_name}'")
        mix_repo.create_head(mirror_repo_branch_name, remote_origin_refs[mirror_repo_branch_name]) \
            .set_tracking_branch(remote_origin_refs[mirror_repo_branch_name]) \
            .checkout()
    else:
        print(f"'{mirror_repo_branch_name}' branch not found. Creating version branch after committing in 'master'.")
        should_create_version_branch = True

        if hasattr(mix_repo.heads, 'master'):
            print("Performing checkout on 'master'.")
            mix_repo.heads.master.checkout()
        else:
            print("'master' does not exist locally.")
            if hasattr(remote_origin_refs, 'master'):
                print("'origin/master' found. Performing checkout from 'origin' remote.")
                mix_repo.create_head('master', remote_origin_refs.master) \
                    .set_tracking_branch(remote_origin_refs.master) \
                    .checkout()
            else:
                print("'master' neither exist locally or remotely. Using default-empty 'master' branch.")
                mix_repo.git.checkout("--orphan", "master")
                mix_repo.git.reset(".")
                mix_repo.git.clean("-df")
                empty_master_branch = True

    # Before performing any new commits, we must set the stage for the incoming commit. That means we must generate a
    # move-commit to track where the new Lumberyard version's files are destined to exist.
    print ("Generating Move-Commit...")
    mix_repo.git.reset('--hard')

    # Importing this file before v1.24 (python 2.7) will raise errors. This can be safely ignored.
    try:
        reload(waf_branch_spec)
    except TypeError:
        pass

    previous_lumberyard_version = waf_branch_spec.LUMBERYARD_VERSION
    # Try to decode in case waf_branch_spec is still using the old format.
    try:
        previous_lumberyard_version = previous_lumberyard_version.decode('utf-8', 'ignore')
    except (UnicodeDecodeError, AttributeError):
        pass

    previous_CL = find_cl_for_ly_version_from_staging_repo(previous_lumberyard_version, mix_repo, source_repo_name)
    previous_branch = format_p4_branch_from_ly_version(previous_lumberyard_version)
    next_CL = ''.join(filter(str.isdigit, args.commitRef))
    next_branch = format_p4_branch_from_ly_version(find_ly_version_for_p4_cl(mix_repo, args.commitRef))

    generate_move_commit(mix_repo, previous_branch, previous_CL, next_branch, next_CL)

    # Replace repo contents with the temp files we created early on.
    exclude_git_dir = os.path.join(args.localRepoDirectory, ".git")
    clean_replace_repo_contents(args.genRoot, args.localRepoDirectory, [exclude_git_dir])
    mix_repo.git.add("--all", "--force")
    promoted_commit_object = mix_repo.commit(args.commitRef)
    mix_repo.index.commit(promoted_commit_object.message,
                             author=promoted_commit_object.author,
                             committer=promoted_commit_object.committer)

    # Tag for customer release
    print("Tagging new commit.")
    version_tag_string = "v{0}".format(next_lumberyard_version)
    mix_repo.create_tag(version_tag_string, force=True)

    # Rename staging repo tag, appending '-Promoted'
    rename_tag(args.commitRef, args.commitRef + "-Promoted",
               source_repo_name, mix_repo, args.dryRun)

    # Push commit & tags
    if args.dryRun:
        print("Performing dry run. No changes will be pushed to remotes.")
    else:
        print("Pushing tag & commit")
        if empty_master_branch:
            mix_repo.git.push("-u", "origin", "master")
        else:
            mix_repo.git.push("--all")
        mix_repo.remote().push(version_tag_string)

    # Create version branch, if necessary
    if should_create_version_branch:
        print(f"Creating branch '{mirror_repo_branch_name}' off new master head.")
        mix_repo.create_head(mirror_repo_branch_name)
        if args.dryRun:
            print("Performing dry run. No changes will be pushed to remotes.")
        else:
            mix_repo.git.push("-u", "origin", "{0}:{0}".format(mirror_repo_branch_name))


if __name__ == "__main__":
    main()
    sys.exit()
