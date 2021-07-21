# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

../../python/python.sh run_launcher_tests_local_validation.py --dev-root-folder "../.." --project "StarterGame" || exit
../../python/python.sh device_farm_create_bundle.py --project StarterGame --project-launcher-tests-folder "../../StarterGame/LauncherTests" --python-test-tools-folder "../PythonTestTools/test_tools"
# Current known limitation on iOS, only one test at a time is supported, see run_launcher_tests_ios.py run_test
../../python/python.sh device_farm_schedule_run.py --app-path "$1" --project-name "LyAutomatedLauncherIOS" --device-pool-name "LyIOS" --test-spec-path "device_farm_test_spec_ios.yaml" --test-bundle-path "temp/StarterGame/test_bundle.zip" --artifacts-output-folder "temp/StarterGame" --device-arns "\\\"arn:aws:devicefarm:us-west-2::device:D125AEEE8614463BAE106865CAF4470E\\\""  --test-names "progress"
