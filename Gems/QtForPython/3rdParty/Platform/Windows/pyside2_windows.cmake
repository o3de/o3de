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
set(pyside2_SHARED_LIB_PATH ${BASE_PATH}/windows/$<IF:$<CONFIG:Debug>,debug,release>)

# Adding Shared libs
set(pyside2_RUNTIME_DEPENDENCIES
    ${pyside2_SHARED_LIB_PATH}/PySide2/$<IF:$<CONFIG:Debug>,pyside2_d.cp37-win_amd64.dll,pyside2.abi3.dll>
    ${pyside2_SHARED_LIB_PATH}/shiboken2/$<IF:$<CONFIG:Debug>,shiboken2_d.cp37-win_amd64.dll,shiboken2.abi3.dll>
    ${pyside2_SHARED_LIB_PATH}/shiboken2/$<IF:$<CONFIG:Debug>,shiboken2_d.cp37-win_amd64.pyd,shiboken2.pyd>)