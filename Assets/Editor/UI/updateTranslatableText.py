# -*- coding: utf-8 -*-
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os

os.chdir('qml')
startDir = os.getcwd()

# since it's a .exe file it will only work on windows, but we may as well
# construct the path in a platform-independent way.
lupdateCmd = os.path.join(startDir, '..', '..', '..',
                          'Code', 'SDKs', 'Qt', 'x64', 'bin', 'lupdate.exe')

lupdateCmd += ' -source-language en_GB '

print(startDir)

# Korean, Japanese and Simplified Chinese
targetLanguages = ['ko', 'ja', 'zh_CN']

for lang in targetLanguages:
    os.chdir(startDir)
    tgtLang = ' -target-language ' + lang
    tsName = ' -ts this_' + lang + '.ts'
    os.system(lupdateCmd + '-no-recursive .' + tgtLang + tsName)

    for folder in os.listdir():
        print(folder)
        folderPath = os.path.join(startDir, folder)

        if not os.path.isdir(folderPath):
            continue

        filenameBase = os.path.join(folderPath, "..", folder + '_')
        tsSwitches = ' -ts ' + filenameBase + lang + '.ts'
        os.system(lupdateCmd + '-recursive . ' + tsSwitches)
        print(('Finished processing: ' + lang + ': ' + folderPath))
