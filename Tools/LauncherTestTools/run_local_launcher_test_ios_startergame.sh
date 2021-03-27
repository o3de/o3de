#  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
#  its licensors.
#
#  For complete copyright and license terms please see the LICENSE at the root of this
#  distribution (the "License"). All use of this software is governed by the License,
#  or, if provided, by the license below or the license accompanying this file. Do not
#  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# Provided as an example of how to run the Automated Launcher Test on a developer's local machine.
../../python/python.sh run_launcher_tests_local_validation.py --dev-root-folder "../.." --project "StarterGame" || exit
# Current known limitation on iOS, only one test at a time is supported, see run_launcher_tests_ios.py run_test
../../python/python.sh run_launcher_tests_ios.py --project-json-path "../../StarterGame/project.json" --project-launcher-tests-folder "../../StarterGame/LauncherTests" --test-names "progress"
