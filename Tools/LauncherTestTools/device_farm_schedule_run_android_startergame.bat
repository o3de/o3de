@echo off
REM
REM  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM  its licensors.
REM
REM  REM  For complete copyright and license terms please see the LICENSE at the root of this
REM  distribution (the "License"). All use of this software is governed by the License,
REM  or, if provided, by the license below or the license accompanying this file. Do not
REM  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM
REM

call ../../python/python.cmd run_launcher_tests_local_validation.py --dev-root-folder "../.." --project "StarterGame"
if %ERRORLEVEL% == 0 (
call ../../python/python.cmd device_farm_create_bundle.py --project StarterGame --project-launcher-tests-folder "../../StarterGame/LauncherTests" --python-test-tools-folder "../PythonTestTools/test_tools"
call ../../python/python.cmd device_farm_schedule_run.py --app-path "../../BinAndroidArmv8Clang/StarterGameLauncher_w_assets.apk" --project-name "LyAutomatedLauncher" --device-pool-name "LyAndroid" --test-spec-path "device_farm_test_spec_android.yaml" --test-bundle-path "temp/StarterGame/test_bundle.zip" --artifacts-output-folder "temp/StarterGame"
)