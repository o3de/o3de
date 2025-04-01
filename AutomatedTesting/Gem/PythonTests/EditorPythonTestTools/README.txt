Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


INTRODUCTION
------------

EditorPythonBindings is a Python project that contains a collection of editor testing tools
developed by the O3DE feature teams. The project contains tools for system level
editor tests.


REQUIREMENTS
------------

 * Python 3.10.13 (64-bit)

It is recommended that you completely remove any other versions of Python
installed on your system.


INSTALL
-----------
It is recommended to set up these these tools with O3DE's CMake build commands.
Assuming CMake is already setup on your operating system, below are some sample build commands:
    cd /path/to/od3e/
    mkdir windows
    cd windows
    cmake .. -G "Visual Studio 16 2019" -DLY_PROJECTS=AutomatedTesting

To manually install the project in development mode using your own installed Python interpreter:
    cd /path/to/od3e/AutomatedTesting/Gem/PythonTests/EditorPythonTestTools
    /path/to/your/python -m pip install -e .


UNINSTALLATION
--------------

The preferred way to uninstall the project is:
 /path/to/your/python -m pip uninstall editor_python_test_tools
