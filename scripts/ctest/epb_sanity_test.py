"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Sanity test for EditorPythonBindings CTest wrapper

import azlmbr.framework as framework

print("EditorPythonBindings CTest Sanity Test")

# A test should have logic to determine success (zero) or failure (non-zero) and 
# return it to the caller. In this sanity test, always return success.
return_code = 0
framework.Terminate(return_code)
