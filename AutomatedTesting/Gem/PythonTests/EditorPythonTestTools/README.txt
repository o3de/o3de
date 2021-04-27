All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


INTRODUCTION
------------

EditorPythonBindings is a Python project that contains a collection of editor testing tools
developed by the Lumberyard feature teams. The project contains tools for system level
editor tests.


REQUIREMENTS
------------

 * Python 3.7.5 (64-bit)

It is recommended that you completely remove any other versions of Python
installed on your system.


INSTALL
-----------
It is recommended to set up these these tools with Lumberyard's CMake build commands.
Assuming CMake is already setup on your operating system, below are some sample build commands:
    cd /path/to/od3e/
    mkdir windows_vs2019
    cd windows_vs2019
    cmake .. -G "Visual Studio 16 2019" -A x64 -T host=x64 -DLY_3RDPARTY_PATH="%3RDPARTYPATH%" -DLY_PROJECTS=AutomatedTesting

To manually install the project in development mode using your own installed Python interpreter:
    cd /path/to/od3e/AutomatedTesting/Gem/PythonTests/EditorPythonTestTools
    /path/to/your/python -m pip install -e .


UNINSTALLATION
--------------

The preferred way to uninstall the project is:
 /path/to/your/python -m pip uninstall editor_python_test_tools
