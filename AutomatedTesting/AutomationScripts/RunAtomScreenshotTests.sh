#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $SCRIPT_DIR > /dev/null

../../python/python.sh -s -B -m pytest -v --tb=short --show-capture=stdout -c ../../pytest.ini --build-directory ../../AutomatedTesting/build/bin/profile ../../AutomatedTesting/Gem/PythonTests/Atom/TestSuite_Periodic_GPU.py --output-path ../../AutomatedTesting/build/Testing/LyTestTools/AutomatedTesting_Atom_TestSuite_Periodic_GPU --junitxml=../../AutomatedTesting/build/Testing/Pytest/AutomatedTesting_Atom_TestSuite_Periodic_GPU.xml

popd > /dev/null
