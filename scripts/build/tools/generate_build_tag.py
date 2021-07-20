#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import os
import pathlib
import shutil
import subprocess
import sys


def run_git_command(args, repo_root):

    process = subprocess.run(['git', *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=repo_root,
        env=os.environ.copy(),
        universal_newlines=True,
    )

    if process.returncode != 0:
        print(
            f'An error occurred while running a command\n'
            f'Command: git {subprocess.list2cmdline(args)}\n'
            f'Return Code: {process.returncode}\n'
            f'Error: {process.stderr}',
            file=sys.stderr
        )
        exit(1)

    output = process.stdout.splitlines()
    if not output:
        print(f'No output received for command: git {subprocess.list2cmdline(args)}.')
        output

    return output[0].strip('"')


if __name__ == "__main__":
    '''
    Generates a build ID based on the state of the git repository.  Will first attempt to use
    existing environment variable (e.g. BRANCH_NAME, CHANGE_ID, CHANGE_DATE) before falling
    back to running git commands directly
    '''
    repo_root = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))

    branch = os.environ.get('BRANCH_NAME')
    if not branch:
        branch = run_git_command(['rev-parse', '--abbrev-ref', 'HEAD'], repo_root)
    branch = branch.replace('/', '-')

    commit_hash = os.environ.get('CHANGE_ID')
    if not commit_hash:
        commit_hash = run_git_command(['rev-parse', 'HEAD'], repo_root)
    commit_hash = commit_hash[0:9]

    # include the commit date to allow some sensible way of sorting
    commit_date = os.environ.get('CHANGE_DATE')
    if not commit_date:
        commit_date = run_git_command(['show', '-s', '--format=%cI', commit_hash], repo_root)
    commit_date = commit_date[0:10]

    print(f'{branch}/{commit_date}-{commit_hash}')
    exit(0)
