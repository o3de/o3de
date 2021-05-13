#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import os
import subprocess

# Returns True if the dst commit descends from the src commit, otherwise False
def is_descendent(src_commit_hash, dst_commit_hash):
    if src_commit_hash is None or dst_commit_hash is None:
        return False
    result = subprocess.run(["git", "merge-base", "--is-ancestor", src_commit_hash, dst_commit_hash])
    return result.returncode == 0

# Attempts to create a diff from the src and dst commits and write to the specified output file, returning True on success, otherwise False
def create_diff_file(src_commit_hash, dst_commit_hash, output_path):
    if os.path.isfile(output_path):
        os.remove(output_path)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    # git diff will only write to the output file if both commit hashes are valid
    subprocess.run(["git", "diff", f"--output={output_path}", src_commit_hash, dst_commit_hash])
    return os.path.isfile(output_path)