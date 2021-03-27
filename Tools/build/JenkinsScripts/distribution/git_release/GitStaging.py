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

# Requires following installs via pip:
# - Boto3
# Requires following software pre-installed:
# - Git
# - Python 3.7.6 x64
# - Apache Ant
from distutils import spawn
from distutils.version import StrictVersion
from git import Repo, RemoteProgress
from urllib.parse import urljoin, urlparse
import argparse
import boto3
import botocore.exceptions
import datetime
import errno
import GitOpsCodeCommit
import glob
import json
import locale
import os
import shutil
import stat
import subprocess
import sys
import textwrap
import time

THIS_SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_SCRIPT_DIRECTORY, ".."))  # Required for AWS_PyTools
sys.path.append(os.path.join(THIS_SCRIPT_DIRECTORY, "..", "Installer"))  # Required for BuildInstallerUtils
from AWS_PyTools import LyCloudfrontOps
from AWS_PyTools import LyChecksum
from Installer import SignTool


URL_KEY = "URL"
CHECKSUM_KEY = "Checksum"
SIZE_KEY = "Uncompressed Size"
BOOTSTRAP_CONFIG_FILENAME = "bootstrap_config.json"

HASH_FILE_NAME = "filehashes.json"
class ExitCodes(object):
    # Starting with higher numbers to avoid default system error collisions.
    INVALID_ARGUMENT = 11
    UNSIGNED_PACKAGE = 12


def print_status_message(message):
    print("-----------------------------")
    print(f"{message}")
    print("-----------------------------\n")


def bytes_to_megabytes(size_in_bytes):
    bytes_in_megabytes = 1048576
    return size_in_bytes / bytes_in_megabytes


def bytes_to_gigabytes(size_in_bytes):
    megabytes_in_gigabytes = 1024
    return bytes_to_megabytes(size_in_bytes) / megabytes_in_gigabytes


def appendTrailingSlashToUrl(url):
    if not url.endswith(tuple(['/', '\\'])):
        url += '/'
    return url


def get_empty_subdirectories(path):
    empty_directories = []
    for dirpath, dirnames, filenames in os.walk(path):
        for cur_dir in dirnames:
            full_path = os.path.join(dirpath, cur_dir)
            if not os.listdir(full_path):
                empty_directories.append(full_path)
    return empty_directories


def get_directory_size_in_bytes(start_path):
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(start_path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            total_size += os.path.getsize(fp)
    return total_size


# Python has issues removing files and directories on Windows
# (even if we've just created them) if they were set to 'readonly'.
# This usually occurs when deleting a '.git' directory, because some internal
# git repository files become 'readonly' when initializing a new repo.
# The following function should override general permission issues when
# deleting.
def handle_remove_readonly(func, path, exc):
    os.chmod(path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)  # 0777
    func(path)


# Ensures a specified directory is existent and empty.  If clean is 'True',
# the directory will delete all contents within.  The clean argument is
# intended to be used for retrying script operation from scratch.  Use with
# care.
def ensure_directory_is_usable(directory, clean):
    if os.path.exists(directory):
        print(f"'{directory}' already exists.")
        if clean is True:
            print(f"Clean mode enabled. Deleting contents of '{directory}'")
            dir_entries = os.listdir(directory)
            for entry in dir_entries:
                entry_full_path = os.path.join(directory, entry)
                if os.path.isfile(entry_full_path):
                    os.chmod(entry_full_path, stat.S_IWRITE)
                    os.remove(entry_full_path)
                elif os.path.isdir(entry_full_path):
                    shutil.rmtree(entry_full_path,
                                  ignore_errors=False,
                                  onerror=handle_remove_readonly)
            if len(os.listdir(directory)) > 0:
                print("Required directory for operation is not empty.")
                print(f"Failed to delete contents of: {directory}")
                sys.exit()
        else:
            print("Reusing contents of existing directory.")
    else:
        print(f"'{directory}' does not exist. Creating.")
        os.makedirs(directory)


def parse_script_arguments():
    parser = argparse.ArgumentParser(description='Creates a git commit from a signed Lumberyard release.')

    parser.add_argument('--gitURL',
                        help="The url for the repo you would like to push to.",
                        default=None)
    parser.add_argument('--gitBranch',
                        help="The branch which the commit will be made to.",
                        required=False,
                        default="master")
    parser.add_argument('--packagePath',
                        help='Filepath to the signed Lumberyard package.',
                        required=True)
    parser.add_argument('--cloudfrontURL',
                        help='Cloudfront base URL.',
                        required=False,
                        default=None)
    parser.add_argument('--zipDescriptor',
                        help='Name of the zip file as appears on a commit message when describing the URL.',
                        required=True)
    parser.add_argument('--binZipSuffix',
                        help='Suffix appended to the newly created binary zip name.',
                        required=True)
    # is it safe to assume that the profile being used for S3 and CodeCommit are the same?
    parser.add_argument('--awsProfile',
                        help='AWS credentials profile generated from AWS CLI. The codecommit repo and cloudfront distribution should both be able to be access by this profile. Defaults to "default".',
                        required=False,
                        default='default')
    parser.add_argument('--uploadProfile',
                        help='AWS CLI profile to use to upload to cloudfront',
                        required=False,
                        default='default')
    parser.add_argument('--genRoot',
                        help='Path to where the entire script operation will take place.',
                        required=True)
    parser.add_argument('--binDownloader',
                        help='Path to the binary downloader which is stored in Perforce.',
                        required=False,
                        default=os.path.join(THIS_SCRIPT_DIRECTORY, "inject", "git_bootstrap.exe"))
    parser.add_argument('--gitReadme',
                        help="The readme to be displayed on the Git repo page.",
                        required=False,
                        default=os.path.join(THIS_SCRIPT_DIRECTORY, "inject", "README.md"))
    parser.add_argument('--gitGuidelines',
                        help="The contribution guidelines for customers submitting changes.",
                        required=False,
                        default=os.path.join(THIS_SCRIPT_DIRECTORY, "inject", "CONTRIBUTING.md"))
    parser.add_argument('--gitIgnore',
                        help="The default '.gitignore' for the repo.",
                        required=False,
                        default=os.path.join(THIS_SCRIPT_DIRECTORY, "inject", ".gitignore"))
    parser.add_argument('--gitBugTemplate',
                        help="The bug issue template.",
                        required=False,
                        default=os.path.join(THIS_SCRIPT_DIRECTORY, "inject", ".github", "ISSUE_TEMPLATE", "bug_report.md"))
    parser.add_argument('--gitFeatureTemplate',
                        help="The feature issue template.",
                        required=False,
                        default=os.path.join(THIS_SCRIPT_DIRECTORY, "inject", ".github", "ISSUE_TEMPLATE", "feature_request.md"))
    parser.add_argument('--gitQuestionTemplate',
                        help="The question issue template.",
                        required=False,
                        default=os.path.join(THIS_SCRIPT_DIRECTORY, "inject", ".github", "ISSUE_TEMPLATE", "question.md"))
    parser.add_argument('--clean',
                        help='Clear out existing temp directories before executing this operation.',
                        required=False,
                        action="store_true")
    parser.add_argument('--keep',
                        help='Skips the clean-up process at the end of the '
                             'operation, keeping temp files in the working directory.',
                        required=False,
                        action="store_true")
    parser.add_argument('--performUpload',
                        help='Performs the upload of the Lumberyard build binaries zip. '
                             'If specified, artifacts are deleted upon successful upload.',
                        required=False,
                        action="store_true")
    parser.add_argument('--performPush',
                        help='Performs the a Git push of the new repo changes created by this process.',
                        required=False,
                        action="store_true")
    parser.add_argument('--allowUnsignedPackages',
                        help='Allows unsigned Lumberyard packages to be processed.',
                        required=False,
                        action="store_true")
    parser.add_argument('--engineDefaultSettingsPath',
                        help="Path to 'default_settings.json' relative to the engine root.",
                        required=False,
                        default='dev/_WAF_/default_settings.json')
    parser.add_argument('--zipOnly',
                        help='Only generate the zip. Do not actually do anything with a git repo.',
                        required=False,
                        action="store_true")
    return parser.parse_args()


# Takes a repository directory and replaces it's contents with the contents of
# another directory, all while preserving the state of the repository.
# The incoming contents are MOVED, not copied.
def clean_replace_repo_contents(incoming_content_directory, repo_directory, excludes):
    repo_dir_entries = os.listdir(repo_directory)
    for entry in repo_dir_entries:
        src_entry_full_path = os.path.join(repo_directory, entry)
        if src_entry_full_path in excludes:
            continue
        if os.path.isfile(src_entry_full_path):
            os.remove(src_entry_full_path)
        elif os.path.isdir(src_entry_full_path):
            print(f"Removing directory '{src_entry_full_path}'")
            shutil.rmtree(src_entry_full_path,
                          ignore_errors=False,
                          onerror=handle_remove_readonly)

    # Add the incoming content to the repo directory
    src_dir_entries = os.listdir(incoming_content_directory)
    for entry in src_dir_entries:
        src_entry_full_path = os.path.join(incoming_content_directory, entry)
        dst_entry_full_path = os.path.join(repo_directory, entry)
        if src_entry_full_path in excludes:
            continue
        shutil.move(src_entry_full_path, dst_entry_full_path)


# Checks whether a particular temp file should be generated/created.
# The 'filepath' argument does not necessarily correlate to the file-to-be-created.
# Example: Checking if FileA should be generated to decide how to process FileB.
def should_generate_resource(filepath, run_clean):
    if run_clean:
        should_create = True
        print(f"'--clean' flag detected. Checking if '{filepath}' already exists.")
        if os.path.exists(filepath):
            print(f"'{filepath}' already exists. Removing.")
            os.remove(filepath)
        else:
            print(f"'{filepath}' is not present.")
    elif not os.path.exists(filepath):
        should_create = True
        print(f"'{filepath}' does not currently exist.")
    else:
        should_create = False
        print(f"'{filepath}' already exists.")
    return should_create


def abort_operation(reason, exit_code):
    print(reason)
    print("Aborting operation.")
    sys.exit(exit_code)


### ZIP GENERATION FUNCTIONS

# create a json file that contains key value pairs of relative path of a file going into the zip, to that file's hash
def generate_hashes_file(bin_directory):
    directory_to_hash = os.path.join(bin_directory, "")
    out_file_path = os.path.join(directory_to_hash, HASH_FILE_NAME)
    if os.path.exists(out_file_path):
        print(f"Hash list already exists. Reusing existing file:\n{out_file_path}")
    else:
        file_hashes = {}
        for root, _, files in os.walk(directory_to_hash):
            for filename in files:
                file_to_hash = os.path.join(root, filename)
                rel_file_path = os.path.relpath(file_to_hash, os.path.dirname(directory_to_hash))
                # make sure the key (file path) is relative, so that it doesn't matter
                #   where the customer has their repository.
                # Opens file in universal mode to reduce unix vs pc line endings issues.
                file_hashes[rel_file_path] = LyChecksum.getChecksumForSingleFile(file_to_hash).hexdigest()
        with open(out_file_path, 'w') as out_file:
            json.dump(file_hashes, out_file, sort_keys=True, indent=4, separators=(',', ': '))
        print(f"Hash list file output to {out_file_path}")


### GIT COMMIT FUNCTIONS

def get_downloader_version(downloader_filepath):
    p = subprocess.Popen([downloader_filepath, "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = p.communicate()
    if p.returncode != 0:
        raise Exception(f"Downloader version command failed ({p.returncode:d}) {output} {error}")
    version = output.strip()
    return version


def get_ly_version(src_directory):
    sys.path.append(os.path.join(src_directory, 'dev'))
    import waf_branch_spec
    return waf_branch_spec.LUMBERYARD_VERSION


def get_ly_build(src_directory):
    sys.path.append(os.path.join(src_directory, 'dev'))
    import waf_branch_spec
    return waf_branch_spec.LUMBERYARD_BUILD


# creates a file with the given name at the root of the repo containing all of the
#   additive binary info
def create_bootstrap_config(filename, path, url, checksum, size):
    zip_info = { URL_KEY: url, CHECKSUM_KEY: checksum, SIZE_KEY: size }
    out_file_path = os.path.join(path, filename)
    with open(out_file_path, 'w') as out_file:
        json.dump(zip_info, out_file, sort_keys=True, indent=4, separators=(',', ': '))


# returns True if needed to create a new branch during this operation
def checkout_git_branch(repo, ly_version):
    if hasattr(repo.heads, ly_version):
        print(f'Performing checkout of local branch {ly_version}')
        repo.git.checkout(ly_version)
    elif hasattr(repo.remote("origin").refs, ly_version):
        print(f'Performing checkout of remote branch {ly_version}')
        remote_ref = repo.remote("origin").refs[ly_version]
        repo.create_head(ly_version, remote_ref) \
            .set_tracking_branch(remote_ref) \
            .checkout()
    else:
        print(f"Branch '{ly_version}' was not found. Searching for closest relative.")
        ly_version_split = ly_version.split('.')

        remote_heads = []
        for head in repo.remote('origin').refs:
            origin_prefix = 'origin/'
            ref_name = head.name[len(origin_prefix):]
            product_major_version_pattern = f"{ly_version_split[0]}.{ly_version_split[1]}"
            if ref_name.startswith(product_major_version_pattern):
                remote_heads.append(ref_name)

        # Did we get any matches?
        if len(remote_heads) > 0:
            identified_parent_branch_name = remote_heads[len(remote_heads)-1]
            repo.git.checkout(identified_parent_branch_name)

            # Magic Git voodoo code to show commits belonging to checked out branch, containing the word 'Promoted'.
            promoted_commits_string = subprocess.check_output("git log"
                                                              " --decorate=full"
                                                              " --simplify-by-decoration"
                                                              " --pretty=oneline"
                                                              " HEAD"
                                                              " | sed -r -e \"s#^[^\(]*\(([^\)]*)\).*$#\\1#\" -e 's#,#\\n#g'"
                                                              " | grep 'tag:'"
                                                              " | sed -r -e 's#[[:space:]]*tag:[[:space:]]*##'"
                                                              " | grep 'Promoted'", shell=True)
            promoted_commits_array = promoted_commits_string.splitlines()

            # Due to output order of subprocess command, the first element is the last promoted commit in this branch.
            identified_promoted_commit_tag_name = promoted_commits_array[0]
            print(subprocess.check_output(["git", "checkout", "-b", ly_version, identified_promoted_commit_tag_name]))
        # We found no matches, we are adding a new version to master.
        # No need to checkout master because every checkout for this logic branch has failed; we are already on master.
        else:
            return True
    return False


def validate_downloader(args, clone_directory, binary_downloader_filename):
    p4_downloader_filepath = args.binDownloader
    git_downloader_filepath = os.path.join(clone_directory, binary_downloader_filename)

    if os.path.exists(git_downloader_filepath):
        print("A binary downloader already exists in the Git repo.")
        p4_downloader_version = StrictVersion(get_downloader_version(p4_downloader_filepath).decode())
        git_downloader_version = StrictVersion(get_downloader_version(git_downloader_filepath).decode())

        if p4_downloader_version > git_downloader_version:
            print(f"The binary downloader in Git is outdated: v{git_downloader_version}")
            print(f"Updating to a newer version: v{p4_downloader_version}")
            shutil.copy(p4_downloader_filepath, git_downloader_filepath)
            # The downloader binary might be read-only, specially if taken directy from a Perforce-managed
            # directory while not checked-out. This may cause permission issues, prompts, or errors on future
            # scripts/automation without authority to modify read-only files. Setting the file to read/write.
            os.chmod(git_downloader_filepath, stat.S_IWRITE)
        elif p4_downloader_version == git_downloader_version:
            print("No changes detected in the binary downloader.")
        else:
            raise Exception(f"The binary downloader in the Git repo is newer (v{git_downloader_version}) than the internal one(v{p4_downloader_version}).\n"
                            "Has the Git repo become compromised?")
    else:
        print("No binary downloader exists in the Git repository. Adding.")
        shutil.copy(p4_downloader_filepath, git_downloader_filepath)


def checkout_git_repo (args):
    print_status_message("Generating empty git repo...")
    GitOpsCodeCommit.init_git_repo(args.gitURL, args.awsProfile, os.path.curdir)
    repo = Repo(os.path.curdir)

    print_status_message("Fetching git repo from remote...")
    repo.remote("origin").fetch()

    if not hasattr(repo.heads, args.gitBranch) and not hasattr(repo.remote().refs, args.gitBranch):
        # We only reach here if gitBranch (presumably 'master', but can be anything) is missing from local & remote.
        # This happens when staging to an empty repo. In such case, we create a local branch which will push at the end.
        # Orphan checkout is only useful if gitBranch is other than 'master', otherwise, this call has no effect.
        repo.git.checkout('--orphan', args.gitBranch)
    else:
        repo.git.checkout(args.gitBranch)

    return repo


def checkout_version_branch(src_directory, repo):
    # We want to attach the Lumberyard version number to the commit message.
    # Obtain version & build-number from the package contents via module import.
    # To successfully import package contents, we must append to our Python 'sys' path.
    ly_version = get_ly_version(src_directory)
    print(f'Parsed Lumberyard version from source: {ly_version}')

    # Checkout whatever branch we should check this commit in to, or make one if there is not already an appropriate branch
    creating_new_version_branch = checkout_git_branch(repo, ly_version)

    return ly_version, creating_new_version_branch


def stage_files_for_commit(args, clone_directory, src_directory, repo):
    # Performing an accurate commit requires detecting modification, deletion, and addition to the repo files.
    # In order to automatically detect this, we will leverage Git's ability to pick up on changes. To do this, we will
    # delete all local files from the repo to then add the new source files. Git should know which files differ.
    print_status_message("Collecting files for staging...")
    binary_downloader_filename = os.path.basename(args.binDownloader)
    excludes = [
        os.path.join(clone_directory, ".git"),
        os.path.join(clone_directory, binary_downloader_filename)
    ]
    clean_replace_repo_contents(src_directory, clone_directory, excludes)

    # During the Git staging process, we may or may not include the binary downloader as part of the commit.
    print_status_message("Validating binary downloader...")
    validate_downloader(args, clone_directory, binary_downloader_filename)

    # Add all into Git staging to determine what the historical changes are.
    # Force the add to bypass .gitignore rules. This ensures any unintended ignored files
    # are mistakenly added to the repository instead of being lost/deleted during this staging proceedure.
    print_status_message("Staging Git files...")
    repo.git.add("--all", "--force")


def commit_to_local_repo(args, repo, ly_version, src_directory, bin_directory_size_in_bytes, creating_new_version_branch):
    bin_directory_size_in_gigabytes = bytes_to_gigabytes(bin_directory_size_in_bytes)

    print_status_message("Generating Git commit...")
    # Generate a commit message.  This can be expanded with development
    # highlights. The highlights could be fed in via python arguments in form
    # of a URL to scrape, text file, or raw arguments.
    git_commit_message_args = [ly_version,
                               args.zipDescriptor,
                               locale.format_string("%.2f", bin_directory_size_in_gigabytes, grouping=True),
                               bin_directory_size_in_bytes,]
    git_commit_message = textwrap.dedent(
        """Lumberyard Release {0}

        {1} Uncompressed Size: {2}GB ({3} bytes)
        """.format(*git_commit_message_args))

    print(f"Generating commit with the following message:\n{git_commit_message}")
    repo.index.commit(git_commit_message)

    # For CI builds, we want to tag the commit with the Perforce change list (CL) number.
    repo.create_tag('CL' + str(get_ly_build(src_directory)))

    if creating_new_version_branch:
        repo.git.checkout('-b', ly_version)
        # TODO:
        # Need to figure out how to set upstream without pushing so that we may isolate all
        # pushes to a single block of code in this file (for easier debugging and maintenance).
        if args.performPush:
            repo.git.push('-u', 'origin', ly_version)


def push_repo_to_remote(args, repo):
    if args.performPush:
        print_status_message("Pushing Git commit to remote...")
        repo.git.push('--all')
        repo.git.push('--tags')

    else:
        print_status_message("'--performPush' flag not present. Skipping Git push procedure...")


def clean_up_repo_and_tempfiles(args, repo, abs_gen_root):
    if not args.keep:
        print_status_message("Cleaning up temp files...")
        repo.close()

        # Give OS time to release any handles on files/paths (we see you, Windows)
        time.sleep(0.1)

        # Although we created multiple directories, the files in genRoot are all
        # temp files. We can safely delete the parent directory instead of each
        # individually created directory/file.
        shutil.rmtree(abs_gen_root, ignore_errors=False, onerror=handle_remove_readonly)
    else:
        print_status_message("'--keep' flag detected. Skipping cleanup procedure...")


# Tests for any invalid input. Any error results into application termination.
def validate_args(args):
    # Test for cloudfront url secure protocol
    if args.performUpload == True or args.zipOnly == False:
        if args.cloudfrontURL == None:
            abort_operation("Need to specify --cloudfrontURL if generating a commit or uploading the zip.")
    if args.performUpload == True or args.cloudfrontURL is not None:
        if not args.cloudfrontURL.startswith("https://"):
            abort_operation("Incorrect cloudfront protocol. Ensure cloudfront URL starts with 'https://'",
                            ExitCodes.INVALID_ARGUMENT)
        args.cloudfrontURL = appendTrailingSlashToUrl(args.cloudfrontURL)

        # Check to see if a default aws profile is available
        try:
            boto3.Session(profile_name=args.awsProfile)
        except botocore.exceptions.ProfileNotFound:
            abort_operation("AWS credentials files are missing. "
                            "Ensure AWS CLI is installed and configured with your IAM credentials.",
                            ExitCodes.INVALID_ARGUMENT)

    # Ensure Lumberyard package exists
    ly_package_filepath = os.path.abspath(args.packagePath)
    if os.path.exists(ly_package_filepath) is False:
        abort_operation(f"'{ly_package_filepath}' does not exist.",
                        ExitCodes.INVALID_ARGUMENT)
    if os.path.isfile(ly_package_filepath) is False:
        abort_operation(f"'{ly_package_filepath}' is not a valid file. Did you specify a directoy or symlink?",
                        ExitCodes.INVALID_ARGUMENT)

    if args.performPush == True and args.zipOnly == True:
        abort_operation("Cannot specify both 'zipOnly' and 'performPush'. Please specify just one.")

    if args.zipOnly == False:
        # Verify Git is installed
        if spawn.find_executable("git") is None:
            abort_operation("Cannot find Git in your environment path. Ensure Git is installed on your machine.")

        if args.gitURL is None:
            abort_operation('You must specify "--gitURL" with a valid URL to a git repo in order to perform any git operations with this script.')

        # Ensure repo URL does not point to a public GitHub repo
        if "github.com" in args.gitURL.lower():
            abort_operation("Cannot stage to GitHub directly. Please use a git repo not on GitHub.")

        # Ensure bin downloader is a valid file
        if os.path.exists(args.binDownloader) is False:
            abort_operation(f"'--binDownloader' filepath does not exist:\n{args.binDownloader}")

        # Ensure readme is a valid file
        if os.path.exists(args.gitReadme) is False:
            abort_operation(f"'--gitReadme' filepath does not exist:\n{args.gitReadme}")

        # Ensure contributions is a valid file
        if os.path.exists(args.gitGuidelines) is False:
            abort_operation(f"'--gitGuidelines' filepath does not exist:\n{args.gitGuidelines}")

        # Ensure gitignore is a valid file
        if os.path.exists(args.gitIgnore) is False:
            abort_operation(f"'--gitIgnore' filepath does not exist:\n{args.gitIgnore}")

        # Ensure gitBugTemplate is a valid file
        if os.path.exists(args.gitBugTemplate) is False:
            abort_operation(f"'--gitBugTemplate' filepath does not exist:\n{args.gitBugTemplate}")

        # Ensure gitFeatureTemplate is a valid file
        if os.path.exists(args.gitFeatureTemplate) is False:
            abort_operation(f"'--gitFeatureTemplate' filepath does not exist:\n{args.gitFeatureTemplate}")

        # Ensure gitQuestionTemplate is a valid file
        if os.path.exists(args.gitQuestionTemplate) is False:
            abort_operation(f"'--gitQuestionTemplate' filepath does not exist:\n{args.gitQuestionTemplate}")


# Returns True if a Lumberyard package is signed.
def is_lumberyard_package_signed(unpacked_directory_root):
    # List of binaries obtained from 'InstallerAutomation.py'
    binaries_to_scan = [
        os.path.join(unpacked_directory_root, "dev", "Bin64vc141", "Editor.exe"),
        os.path.join(unpacked_directory_root, "dev", "Bin64vc142", "Editor.exe")
    ]

    for bin_filename in binaries_to_scan:
        if SignTool.signtoolVerifySign(bin_filename, True) is False:
            return False
    return True


def main():
    args = parse_script_arguments()
    validate_args(args)

    initial_cwd = os.getcwd()
    locale.setlocale(locale.LC_NUMERIC, 'english')

    # Cache the absolute path of genRoot (aka, the workspace)
    abs_gen_root = os.path.abspath(args.genRoot)

    # Where the package shall be entirely extracted to.
    # Contents are 1:1 with zip.
    bin_directory = os.path.join(abs_gen_root, "PackageExtract")

    # Where the source files will be once split (moved) from the extracted
    # package directory.
    src_directory = os.path.join(abs_gen_root, "Src")

    # Directory for the local git clone of the repository.
    clone_directory = os.path.join(abs_gen_root, "Repo")

    # We want to split the package into source files and binary files.
    # To begin this process, we must extract the contents from the zip.
    print_status_message("Creating genRoot directories...")
    ensure_directory_is_usable(clone_directory, args.clean)
    ensure_directory_is_usable(src_directory, args.clean)
    ensure_directory_is_usable(bin_directory, args.clean)

    package_zip_filepath = os.path.abspath(args.packagePath)

    if args.clean or not os.path.exists(bin_directory):
        print_status_message(f"Extracting files from {package_zip_filepath}")
        subprocess.call(["ant",
                         "ExtractPackage",
                         "-DZipfile=" + package_zip_filepath,
                         "-DExtractDir=" + bin_directory],
                        shell=True)
    else:
        print_status_message("Path already exists. Reusing contents")

    print_status_message("Verifying structure of package contents")
    empty_directories = get_empty_subdirectories(bin_directory)
    if len(empty_directories) > 0:
        stringList = '\n'.join(empty_directories)
        print(f"Package contains empty directories. Deleting:\n{stringList}")
        for x in empty_directories:
            shutil.rmtree(x)
    else:
        print("Structure is valid. No empty directories found.")

    if args.allowUnsignedPackages:
        print_status_message("'--allowUnsignedPackages' flag detected. Skipping signature check.")
    else:
        print_status_message("Scanning for signed Lumberyard package")
        if not is_lumberyard_package_signed(bin_directory):
            abort_operation("The provided package is not signed. Retry with a signed package, or ensure "
                            "'--allowUnsignedPackages' flag is specified.", ExitCodes.UNSIGNED_PACKAGE)
        elif not SignTool.signtoolVerifySign(args.binDownloader, True):
            abort_operation("The provided package is signed, but the binary downloader is not. Sign the downloader, or "
                            "ensure '--allowUnsignedPackages' flag is specified.", ExitCodes.UNSIGNED_PACKAGE)
        else:
            print("Package is signed.")

    # We now meet the requirements for the next step: Splitting.
    # For this, we invoke an ant script.
    if args.clean:
        print_status_message("Splitting source and binary files...")
        subprocess.call(["ant",
                         "SplitZip",
                         "-DExtractBuild=" + bin_directory,
                         "-DGitSrc=" + src_directory],
                        shell=True)

        # The Github readme, guidelines and templates are stored separately so let's make sure they're included in the distribution.
        github_readme_filename = os.path.basename(args.gitReadme)
        shutil.copy(args.gitReadme, os.path.join(src_directory, github_readme_filename))

        github_contributions_filename = os.path.basename(args.gitGuidelines)
        shutil.copy(args.gitGuidelines, os.path.join(src_directory, github_contributions_filename))

        github_gitignore_filename = os.path.basename(args.gitIgnore)
        shutil.copy(args.gitIgnore, os.path.join(src_directory, github_gitignore_filename))

        template_path = os.path.join(src_directory,".github/ISSUE_TEMPLATE")
        if not os.path.exists(template_path):
            os.makedirs(template_path)

        github_bug_filename = os.path.join(template_path, os.path.basename(args.gitBugTemplate))
        shutil.copy(args.gitBugTemplate, os.path.join(github_bug_filename))

        github_feature_filename = os.path.join(template_path, os.path.basename(args.gitFeatureTemplate))
        shutil.copy(args.gitFeatureTemplate, os.path.join(github_feature_filename))

        github_question_filename = os.path.join(template_path, os.path.basename(args.gitQuestionTemplate))
        shutil.copy(args.gitQuestionTemplate, os.path.join(github_question_filename))

    # Generate the file containing the list of hashes of all of the content that will go into the zip
    print_status_message("Generating file containing list of file hashes...")
    generate_hashes_file(bin_directory)


    # We have split our Source from Binaries. We can now identify the size of
    # the binaries zip when uncompressed. We will need this for the commit message.
    print_status_message("Measuring unpacked binaries size...")
    bin_directory_size_in_bytes = get_directory_size_in_bytes(bin_directory)
    bin_directory_size_in_megabytes = bytes_to_megabytes(bin_directory_size_in_bytes)
    bin_directory_size_in_gigabytes = bytes_to_gigabytes(bin_directory_size_in_bytes)

    print(textwrap.dedent(f"""
        Total size in Bytes: {locale.format_string("%d", bin_directory_size_in_bytes, grouping=True)}
                  Megabytes: {locale.format_string("%.4f", bin_directory_size_in_megabytes, grouping=True)}
                  Gigabytes: {locale.format_string("%.2f", bin_directory_size_in_gigabytes, grouping=True)}
        """))

    # Generate the expected filepath for the binaries zip.
    if args.clean:
        timestamp = time.time()
        bin_zip_filename = os.path.basename(package_zip_filepath)
        bin_zip_filepath = "{0}_{1}-{2}.zip".format(
            os.path.join(abs_gen_root, os.path.splitext(bin_zip_filename)[0]),
            datetime.datetime.fromtimestamp(timestamp).strftime('%m%d%y_%H%M%S'),
            args.binZipSuffix)
    else:
        # Find the last generated binary zip.
        glob_pattern = f"{abs_gen_root}\\*-{args.binZipSuffix}.zip"
        glob_list_result = glob.glob(glob_pattern)
        glob_list_length = len(glob_list_result)
        if glob_list_length > 0:
            bin_zip_filepath = glob_list_result[glob_list_length-1]
        else:
            raise Exception("No pre-existing binary zip to reuse. Run with '--clean' flag to generate new binary zip.")


    # Let's zip up the Binaries for submitting to S3.
    # We may be resuming from a previous run. We may not want to start over
    # from scratch as this takes a while, thus, we check before processing...
    if should_generate_resource(bin_zip_filepath, args.clean):
        print_status_message(f"Zipping binary files into '{bin_zip_filepath}'")
        subprocess.call(["ant",
                         "ZipBinaries",
                         "-DZipDest=" + bin_zip_filepath,
                         "-DZipSrc=" + bin_directory],
                        shell=True)
    else:
        print_status_message("Skipping binary files zipping operation.")

    # We generate a checksum for the zip file. The downloader will use this to
    # ensure the contents have not been tampered with.
    print_status_message("Generating bin zip checksum.")
    bin_zip_file_checksum = LyChecksum.getChecksumForSingleFile(bin_zip_filepath)

    target_cloudfront_url = "N/A"
    if args.performUpload:
        print_status_message("Uploading to S3...")
        target_bucket_path = LyCloudfrontOps.uploadFileToCloudfrontURL(bin_zip_filepath,
                                                                       args.cloudfrontURL,
                                                                       args.uploadProfile,
                                                                       False)
        # Bucket path and cloudfront url both contain the bucket folder name in their paths.
        # We'll trim the the clourfront url to only be the cloudfront distribution link.
        # We then append the path to generate a qualified url for download.
        parsed_url = urlparse(args.cloudfrontURL)
        target_bucket_url = '{url.scheme}://{url.netloc}/'.format(url=parsed_url)
        target_cloudfront_url = urljoin(target_bucket_url, target_bucket_path)
        print(f"Uploaded file to: {target_cloudfront_url}")
        if not args.keep:
            os.remove(bin_zip_filepath)
    else:
        print_status_message("'--performUpload' flag not present. Skipping S3 upload procedure...")

    # generate the JSON file that contains information on the git binary. Can
    #   only do that if we have the url to put into the file.
    if args.cloudfrontURL:
        create_bootstrap_config(BOOTSTRAP_CONFIG_FILENAME,
            src_directory,
            urljoin(args.cloudfrontURL,os.path.basename(bin_zip_filepath)),
            bin_zip_file_checksum.hexdigest(),
            bin_directory_size_in_bytes)

    if args.zipOnly == False:
        # We may now begin the git phase.
        # To perform all subsequent git operations, we should do them from the repo directory.
        os.chdir(clone_directory)

        repo = checkout_git_repo(args)
        ly_version, create_new_branch = checkout_version_branch(src_directory, repo)
        stage_files_for_commit(args, clone_directory, src_directory, repo)
        commit_to_local_repo(args, repo, ly_version, src_directory, bin_directory_size_in_bytes, create_new_branch)
        push_repo_to_remote(args, repo)

        # Let's return to the original working directory.
        os.chdir(initial_cwd)
        clean_up_repo_and_tempfiles(args, repo, abs_gen_root)

    else:
        print_status_message("'--zipOnly' flag present. Skipping Git commit generation...")

    print_status_message("Lumberyard to Git operation completed successfully.")


if __name__ == "__main__":
    main()
    sys.exit()
