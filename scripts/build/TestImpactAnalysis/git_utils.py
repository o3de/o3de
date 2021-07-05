#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import subprocess
import git

# Returns True if the dst commit descends from the src commit, otherwise False
def is_descendent(src_commit_hash, dst_commit_hash):
    if src_commit_hash is None or dst_commit_hash is None:
        return False
    result = subprocess.run(["git", "merge-base", "--is-ancestor", src_commit_hash, dst_commit_hash])
    return result.returncode == 0

# Attempts to create a diff from the src and dst commits and write to the specified output file
def create_diff_file(src_commit_hash, dst_commit_hash, output_path):
    if os.path.isfile(output_path):
        os.remove(output_path)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    # git diff will only write to the output file if both commit hashes are valid
    subprocess.run(["git", "diff", "--name-status", f"--output={output_path}", src_commit_hash, dst_commit_hash])
    if not os.path.isfile(output_path):
        raise FileNotFoundError(f"Source commit '{src_commit_hash}' and/or destination commit '{dst_commit_hash}' are invalid")

# Basic representation of a repository
class Repo:
    def __init__(self, repo_path):
        self.__repo = git.Repo(repo_path)

    # Returns the current branch
    @property
    def current_branch(self):
        branch = self.__repo.active_branch
        return branch.name
