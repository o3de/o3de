"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# --runpythontest @devroot@\tests\hydra\ctests\throws_exception.py
# An example of how a test script to fatal from Editor.exe when a Python exception happens

print ('hello, throws_exception')
foo = 1.0
bar = 0.0
baz = foo / bar

