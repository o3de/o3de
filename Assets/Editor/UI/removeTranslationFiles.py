#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os

filterAction = ".ts"
walkPath = "qml"

execCmd = "start /B "

for root, dirs, files in os.walk(walkPath):
    if filterAction == ".ts":
        for name in files:
            # remove all the .ts files
            if name.endswith(".ts"):
                os.remove(os.path.join(root, name))
    if filterAction == ".qml":
        for name in files:
            if name.endswith(".qml"):
                filePath = os.path.join(root, name)
                print(filePath)
                # open the .qml file in the default editor (QtCreator)
                os.system(execCmd + filePath)
