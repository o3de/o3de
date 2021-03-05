All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


INTRODUCTION
------------

Remote Console is used to connect to a Lumberyard game instance. It can be used to send and 
read console commands.


REQUIREMENTS
------------

 * Python 3.7.x (64-bit)

It is recommended that you completely remove any other versions of Python
installed on your system.


INSTALL
-----------
It is recommended to set up these these tools with the lmbr_test tool Lumberyard's root directory:
  lmbr_test pysetup install

To manually install the project in development mode:
  python -m pip install -e .


UNINSTALLATION
--------------

To uninstall the project, use:

  python -m pip uninstall remote-console

