#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import marshal
from subprocess import Popen, PIPE, check_output
from typing import List


def run_p4_command(command: str, client: str = None, raw_output: bool = False) -> List[dict]:
    """Executes a Perforce command by calling p4 in a subprocess

    :param command: The command to run (not including p4, e.g., not ['p4', 'opened'] but ['opened'])
    :param client: The Perforce client to use when running the command
    :param raw_output: if True, return the raw output from p4 instead of using the python dict.
    :return: The list of results from the command where each result is a dict (refer to p4's global -G option)
             if 'raw_output' is true, return will be verbatim string from p4.
    """
    results = []

    base_command = ['p4']

    if not raw_output:
        base_command.append('-G')

    if client is not None:
        base_command += ['-c', client]

    if raw_output:
        return check_output(base_command + command.split()).decode('utf8')
    
    pipe = Popen(base_command + command.split(), stdout=PIPE).stdout
    try:
        while True:
            result = marshal.load(pipe)
            results.append(result)
    except EOFError:
        pass
    finally:
        pipe.close()

    decoded_results = [{k.decode(): v.decode() if isinstance(v, bytes) else str(v) for k, v in r.items()} for r in results]
    if decoded_results and decoded_results[0]['code'] == 'error':
        command_was = ' '.join(base_command + command.split())
        error_was = decoded_results[0]['data']
        raise RuntimeError(f'P4 Command failed: "{command_was}"\nERROR FROM P4: {error_was}\n')
    return decoded_results
