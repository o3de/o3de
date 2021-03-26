"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# --runpythontest @devroot@\tests\hydra\ctests\stop_with_error_one.py
# an example terminating with a non-zero return code from Editor.exe

import azlmbr.framework
print ('hello, stop_with_error_one')
azlmbr.framework.Terminate(1)
