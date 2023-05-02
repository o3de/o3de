#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import subprocess
import git
import pathlib

# Basic representation of a git repository
class Repo:
    def __init__(self, repo_path: str):
        self._repo = git.Repo(repo_path)
        self._remote_url = self._repo.remotes[0].config_reader.get("url")

    # Returns the current branch
    @property
    def current_branch(self):
        branch = self._repo.active_branch
        return branch.name

    # Returns the remote URL
    @property
    def remote_url(self):
        return self._remote_url

    def create_diff_file(self, src_commit_hash: str, dst_commit_hash: str, output_path: pathlib.Path, multi_branch: bool):
        """
        Attempts to create a diff from the src and dst commits and write to the specified output file.

        @param src_commit_hash: The hash for the source commit.
        @param dst_commit_hash: The hash for the destination commit.
        @param multi_branch:    The two commits are on different branches so view the changes on the 
                                branch containing and up to dst_commit, starting at a common ancestor of both.
        @param output_path:     The path to the file to write the diff to.
        """

        try:
            # Remove the existing file (if any) and create the parent directory
            # output_path.unlink(missing_ok=True) # missing_ok is only available in Python 3.8+
            if output_path.is_file():
                output_path.unlink()
            output_path.parent.mkdir(parents=True, exist_ok=True)
        except EnvironmentError as e:
            raise RuntimeError(f"Could not create path for output file '{output_path}'")

        args = ["git", "diff", "--name-status", f"--output={output_path}"]
        if multi_branch:
            args.append(f"{src_commit_hash}...{dst_commit_hash}")
        else:
            args.append(src_commit_hash)
            args.append(dst_commit_hash)

        # git diff will only write to the output file if both commit hashes are valid
        subprocess.run(args)
        if not output_path.is_file():
            raise RuntimeError(f"Source commit '{src_commit_hash}' and/or destination commit '{dst_commit_hash}' are invalid")

    def is_descendent(self, src_commit_hash: str, dst_commit_hash: str):
        """
        Determines whether or not dst_commit is a descendent of src_commit.

        @param src_commit_hash: The hash for the source commit.
        @param dst_commit_hash: The hash for the destination commit.
        @return:                True if the dst commit descends from the src commit, otherwise False.
        """

        if not src_commit_hash and not dst_commit_hash:
            return False
        result = subprocess.run(["git", "merge-base", "--is-ancestor", src_commit_hash, dst_commit_hash])
        return result.returncode == 0

    # Returns the distance between two commits
    def commit_distance(self, src_commit_hash: str, dst_commit_hash: str):
        """
        Determines the number of commits between src_commit and dst_commit.

        @param src_commit_hash: The hash for the source commit.
        @param dst_commit_hash: The hash for the destination commit.
        @return:                The distance between src_commit and dst_commit (if both are valid commits), otherwise None.
        """

        if not src_commit_hash and not dst_commit_hash:
            return None
        commits = self._repo.iter_commits(src_commit_hash + '..' + dst_commit_hash)
        return len(list(commits))
