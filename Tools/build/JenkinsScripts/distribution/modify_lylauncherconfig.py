"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import fileinput
import os
import stat

os.chmod('SetupAssistantConfig.ini', stat.S_IWRITE)
for line in fileinput.input('SetupAssistantConfig.ini', inplace=1):
    # Below is an example of how to modify the value for 'compileandroid'.  Its commented out to demonstrate the ability to 
    # alter entries in this file in the future
    #if line.startswith(';compileandroid'):
    #    print('compileandroid="enabled"  ; compile runtime for android')
    #else:
    print line,