All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


INTRODUCTION
------------

LyTestTools is a Python project that contains a collection of testing tools
developed by the Lumberyard Test Tech team. The project contains
the following tools:

 * Workspace Manager:
     A library to manipulate Lumberyard installations
 * Launchers:
     A library to test the game in a variety of platforms


REQUIREMENTS
------------

 * Python 3.7.5 (64-bit)

It is recommended that you completely remove any other versions of Python
installed on your system.


INSTALL
-----------
It is recommended to set up these these tools with Lumberyard's CMake build commands.
Assuming CMake is already setup on your operating system, below are some sample build commands:
    cd /path/to/lumberyard/dev/
    mkdir windows_vs2019
    cd windows_vs2019
    cmake -E time cmake --build . --target ALL_BUILD --config profile
NOTE:
Using the above command also adds LyTestTools to the PYTHONPATH OS environment variable.
Additionally, some CTest scripts will add the Python interpreter path to the PYTHON OS environment variable.
There is some LyTestTools functionality that will search for these, so feel free to populate them manually.

To manually install the project in development mode using your own installed Python interpreter:
    cd /path/to/lumberyard/dev/Tools/LyTestTools/
    /path/to/your/python -m pip install -e .

For console/mobile testing, update the following .ini file in your root user directory:
    i.e. C:/Users/myusername/ly_test_tools/devices.ini (a.k.a. %USERPROFILE%/ly_test_tools/devices.ini)

You will need to add a section for the device, and a key holding the device identifier value (usually an IP or ID).
It should look similar to this for each device:
    [android]
    id = 988939353955305449

    [gameconsole]
    ip = 192.168.1.1

    [gameconsole2]
    ip = 192.168.1.2


PACKAGE STRUCTURE
-----------------

The project is organized into packages. Each package corresponds to a tool:

- LyTestTools.ly_test_tools._internal: contains logging setup, pytest fixture, and lumberyard workspace manager modules
- LyTestTools.ly_test_tools.builtin: builtin helpers and fixtures for quickly writing tests
- LyTestTools.ly_test_tools.console: modules used for consoles
- LyTestTools.ly_test_tools.environment: functions related to file/process management and cleanup
- LyTestTools.ly_test_tools.image: modules related to image capturing and processing
- LyTestTools.ly_test_tools.launchers: game launchers library
- LyTestTools.ly_test_tools.log: modules for interacting with generated or existing log files
- LyTestTools.ly_test_tools.lumberyard: modules used to interact with lumberyard
- LyTestTools.ly_test_tools.mobile: modules used for android/ios
- LyTestTools.ly_test_tools.report: modules used for reporting
- LyTestTools.tests: LyTestTools integration, unit, and example usage tests


DIRECTORY STRUCTURE
-------------------

The directory structure corresponds to the package structure. For example, the
ly_test_tools.builtin package is located in the ly_test_tools/builtin/ directory.


ENTRY POINTS
------------

Deploying the project in development mode installs only entry points for pytest fixtures.


UNINSTALLATION
--------------

The preferred way to uninstall the project is:
 /path/to/your/python -m pip uninstall ly_test_tools
