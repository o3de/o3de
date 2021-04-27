"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Sanity test for EditorPythonBindings CTest wrapper

import azlmbr.framework as framework

print("EditorPythonBindings CTest Sanity Test")

# A test should have logic to determine success (zero) or failure (non-zero) and 
# return it to the caller. In this sanity test, always return success.
return_code = 0
framework.Terminate(return_code)
