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
"""
This file is the central location for functions and operations common to GitHub and CodeCommit
"""

import subprocess


def get_revision_list(repo_diretory):
    print(f"Parsing hashes from git repository: {repo_diretory}")
    p = subprocess.Popen(["git", "rev-list", "--all"], stdout=subprocess.PIPE, cwd=repo_diretory)
    (git_hash_output, git_hash_error) = p.communicate()
    if git_hash_error is not None:
        raise Exception(git_hash_error)
    return git_hash_output.splitlines()
