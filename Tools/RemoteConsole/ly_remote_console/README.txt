Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


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

