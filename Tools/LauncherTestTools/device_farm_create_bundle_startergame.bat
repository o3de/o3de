@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM
REM

python device_farm_create_bundle.py --project StarterGame --project-launcher-tests-folder "../../StarterGame/LauncherTests" --python-test-tools-folder "../PythonTestTools/test_tools"
