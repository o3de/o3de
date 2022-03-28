Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


INTRODUCTION
------------

o3de is a package of scripts containing functionality to register engine, projects, gems,
templates and download repositories with the o3de manifests
It also contains functionality for creating new projects, gems and templates as well
as querying existing gems and templates


REQUIREMENTS
------------

 * Python 3.7.10 (64-bit)

INSTALL
-----------
It is recommended to set up these these tools with O3DE's CMake build commands.
Assuming CMake is already setup on your operating system, below are some sample build commands:
    cd /path/to/od3e/
    cmake -B windows -S . -G"Visual Studio 16"

To manually install the project in development mode using your own installed Python interpreter:
    cd /path/to/od3e/o3de
    /path/to/your/python -m pip install -e .


UNINSTALLATION
--------------

The preferred way to uninstall the project is:
 /path/to/your/python -m pip uninstall o3de
