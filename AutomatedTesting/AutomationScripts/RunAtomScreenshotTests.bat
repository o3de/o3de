REM --------------------------------------------------------------------------------------------------
REM 
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM 
REM --------------------------------------------------------------------------------------------------

REM record current dir
pushd .
REM cd to o3de root dir, %~dp0 is the path to the folder that contains this script
cd %~dp0/../..
setlocal
call python/python.cmd -s -B -m pytest -v --tb=short --show-capture=stdout -c pytest.ini --build-directory AutomatedTesting/build/bin/profile AutomatedTesting/Gem/PythonTests/Atom/TestSuite_Periodic_GPU.py --output-path AutomatedTesting/build/Testing/LyTestTools/AutomatedTesting_Atom_TestSuite_Periodic_GPU --junitxml=AutomatedTesting/build/Testing/Pytest/AutomatedTesting_Atom_TestSuite_Periodic_GPU.xml
endlocal
popd
