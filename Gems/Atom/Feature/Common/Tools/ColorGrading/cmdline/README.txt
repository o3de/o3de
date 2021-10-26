#
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

These are commandline tools for color grading workflow and authoring LUTs externally.
Note: may require OpenColorIO (ocio) and/or OpenImageIO (oiio) to be set up properly.

CMD_ColorGradingTools.bat		:: opens a configured window env context in CMD
Env_Core.bat					:: core O3DE env hooks (any envars DCCSI_ shares some overlay with DccScriptingInterface Gem)
Env_Python.bat					:: sets up access and configures env for O3DE python (to run externally)
Env_Tools.bat					:: hooks for ocio and oiio
								:: note: currently you must build coio tools/apps yourself (we used vcpkg and cmake gui)
								:: note: oiio, it's .pyd and oiiotool should build in O3DE (if you have troubel issue a ticket to investigate)
Env_WingIDE.bat					:: hooks for WingIDE (a python IDE and debugger)
								:: note: feel free to configure a different IDE, user choice.
Launch_WingIDE-7-1.bat			:: this will launch WingIDE with envar hooks and access to O3DE python (for authroing, debugging)
								:: note: you will need to make sure wingstub.py is configured correctly
								:: note: you will need to create the wing project file/path (see Env_WingIDE.bat)
User_Env.bat.template			:: this is an example of overriding or extending the envar hooks (set local engine root, or branch path, etc.)
								:: note: to use, copy and rename to User_Env.bat
								:: bote: we use this to primarily enable the debugging related envar hooks