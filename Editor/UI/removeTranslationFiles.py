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
