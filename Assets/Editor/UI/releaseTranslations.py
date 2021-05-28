# -*- coding: utf-8 -*-
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

os.chdir('qml')
startDir = os.getcwd()

# since it's a .exe file it will only work on windows, but we may as well
# construct the path in a platform-independent way.
lreleaseCmd = os.path.join(startDir, '..', '..', '..',
                          'Code', 'SDKs', 'Qt', 'x64', 'bin', 'lrelease.exe ')

print(startDir)

# Korean, Japanese and Simplified Chinese
targetLanguages = ['ko', 'ja', 'zh_CN']

for lang in targetLanguages:
    os.chdir(startDir)
    tgtLang = '-target-language ' + lang
    os.system(lreleaseCmd + 'this_' + lang + '.ts')

    for fileName in os.listdir():
        if not fileName.endswith(".ts"):
            continue

        os.system(lreleaseCmd + ' ' + fileName)

        print(('Finished processing: ' + fileName))
