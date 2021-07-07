@echo off
REM
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM
REM

call ../../python/python.cmd run_launcher_tests_local_validation.py --dev-root-folder "../.." --project "StarterGame"
if %ERRORLEVEL% == 0 (
call ../../python/python.cmd device_farm_create_bundle.py --project StarterGame --project-launcher-tests-folder "../../StarterGame/LauncherTests" --python-test-tools-folder "../PythonTestTools/test_tools"
call ../../python/python.cmd device_farm_schedule_run.py --app-path "../../BinAndroidArmv8Clang/StarterGameLauncher_w_assets.apk" --project-name "LyAutomatedLauncher" --device-pool-name "LyAndroid" --test-spec-path "device_farm_test_spec_android.yaml" --test-bundle-path "temp/StarterGame/test_bundle.zip" --artifacts-output-folder "temp/StarterGame"
)