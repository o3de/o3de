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
            f'Error: {process.stderr}'
        )
        exit(1)

    output = process.stdout.splitlines()
    # something went wrong and we somehow got more information then requested
    if len(output) != 1:
        print(f'Unexpected output received.\n'
            f'Command: git {subprocess.list2cmdline(args)}\n'
            f'Output:{process.stdout}\n'
            f'Error: {process.stderr}\n'
        )
        exit(1)

    return output[0].strip('"')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generates a build ID based on the state of the git repository')
    parser.add_argument('output_file', help='Path to the output file where the build ID will be written to')
    parsed_args = parser.parse_args()

    repo_root = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))

    branch = run_git_command(['branch', '--show-current'], repo_root)
    branch = branch.replace('/', '-')

    commit_hash = run_git_command(['show', '--format="%h"', '--no-patch'], repo_root)

    # include the commit date to allow some sensible way of sorting
    commit_date = run_git_command(['show', '-s', '--format="%cs"', commit_hash], repo_root)

    with open(parsed_args.output_file, 'w') as out_file:
        out_file.write(f'{branch}/{commit_date}-{commit_hash}')

    sys.exit(0)
