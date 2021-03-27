"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from contextlib import contextmanager
import psutil
import subprocess


@contextmanager
def managed_popen(args_list, cwd=None, stdout=subprocess.PIPE, stderr=subprocess.STDOUT):
    """
    Context manager for subprocess.Popen objects which allows Popen to be used in a with/as statement.
    This should not be used with processes that are to continue running outside of the with block.
    :param args_list: Sequence of arguments as they would be passed to a terminal.
    :param cwd: The current working directory to use when issuing the command.
    :param stdout: File handle or pipe to use for stdout.
    :param stderr: File handle or pipe to use for stderr.
    """
    process = subprocess.Popen(args_list, cwd=cwd, stdout=stdout, stderr=stderr)
    try:
        yield process
    finally:
        if not process.poll():
            process.terminate()


def get_psutil_process(process_name):
    """
    Gets a reference to a psutil.Process object with the given process name.
    :return: A reference to the first process encountered with the given name or None if no process is found.
    """
    for process in psutil.process_iter():
        if process_name == process.name():
            return process
    return None
