# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

import sys
# This should be the path your PyCharm installation
pydevd_egg = r"C:\Program Files\JetBrains\PyCharm 2020.3.2\debug-eggs\pydevd-pycharm.egg"
if not pydevd_egg in sys.path:
    sys.path.append(pydevd_egg)
import pydevd
# This clears out any previous connection in case you restarted the debugger from PyCharm
pydevd.stoptrace()
# 9001 matches the port number that I specified in my configuration
pydevd.settrace('localhost', port=9001, stdoutToServer=True, stderrToServer=True, suspend=False)