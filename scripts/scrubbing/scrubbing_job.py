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
import sys
cur_dir = cur_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.abspath(f'{cur_dir}/../build/package'))
import util

# Run validator
success = True
validator_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'validator.py')
engine_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
if sys.platform == 'win32':
    python = os.path.join(engine_root, 'python', 'python.cmd')
else:
    python = os.path.join(engine_root, 'python', 'python.sh')
args = [python, validator_path, '--package_platform', 'Windows', '--package_type', 'all', engine_root]
return_code = util.safe_execute_system_call(args)
if return_code != 0:
    success = False
if not success:
    util.error('Restricted file validator failed.')
print('Restricted file validator completed successfully.')
