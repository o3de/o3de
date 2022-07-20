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
"""An O3DE out-of-box WingIDE integration

    < DCCsi >/Tools/IDE/WingIDE/__init__.py

:Status: Prototype
:Version: 0.0.1
:Wing Version: Wing Pro 8.x

This init allows us to treat WingIDE setup as a DCCsi tools python package.

Setting up a consistent development environment can be daunting for an
initiate; hours or days searching through docs and setting up a project,
just to get to the point you are authoring code while able to debug it.
This DCCsi Tool integration performs much of this setup for developers
and contributors working with Python and Wing IDE.

Choice of IDEs like most tools, is a very personal choice and investment.
This integration doesn't mean we have locked O3DE or Technical ARt into a
single solution, it is just one possible tool integration of many (we'd
like to add VScode and PyCharm in a similar manner.) And the community
could likewise set up a multitude of others.

The reason I like Wing IDE as a TA is that getting it working with DCC
tools like Maya is relatively straightforward and well documented.

Also the DCCsi works with many DCC apps, each with their own Python
interpretter, and sometimes these python interpretters are managed, such
as mayapy.exe

This integration allows each DCC py interpretter, to be specified in a
data-driven manner as a dev/launch environment; and since the DCCsi
has portions of API that are shared py3 code, you may want to test the
code against several different interpretters in a similar environment as
the code would operate normally outside of the IDE and within the DCC app.

So for instance,
The Wing IDE launch environment, may specify the following:

:: shared location for 64bit DCCSI_PY_MAYA python location
set "DCCSI_PY_MAYA=%MAYA_BIN_PATH%\mayapy.exe"

Then within Wing IDE, there will be a set of Python Shell environments,
each can start a defined interpretter.

DCCSI_PY_O3DE = ${DCCSI_PY_O3DE}
DCCSI_PY_MAYA = ${DCCSI_PY_MAYA}
...

And there are times special considerations to be made, for instance
intellisense auto-complete with mayapy.exe doesn't work unless mayapy.exe
is that actual interpretter specified.

In Wing you can switch the interpretter via:
Console (panel) > Python Shell (tab) > Options > Use Environment ...

This integration also hooks into bootstrapping code, which can auto-start
and attach to Wing as the external debugger from other entrypoints and code,
such as a plugin or tool starting within O3DE.

This is a python script driven replacement for files found here:
    < DCCsi >\Tools\Dev\Windows\*.bat

Notes:
- The old .bat launchers and some DCCsi code supported py2.7+, but we are
depricating support because of end-of-life

- The old .bat launchers supported Wing 7.x, however that version of Wing
also shipped with py2.7 as it's own managed interpretter, so we do not
intend to implement support for older versions of wing < 8. However, wing
can support py3 interpretters and debugging so it's possible it will
continue to work.

- I am a paid developer, so I am using Wing Pro. I need a fully featured IDE
to develop with.  This increment of work does not intend to support Wing
Community edition, I consider that future work.

- The old .bat files are a great fallback to avoid a catch22; I am using
Wing to develop the framework for a managed environment, launching and
boostrapping of tools like Wing. So the .bat files will be updated to
Wing 8.x support and stick around to start Wing on windows if and when the
framework is broken and inoperable.

"""
# -------------------------------------------------------------------------
# standard imports
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'Tools.IDE.Wing'

__all__ = ['config',
           'constants'
           'discovery',
           'start']

_LOGGER = _logging.getLogger(_PACKAGENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Boilerplate code in this module is here as a developer convenience,
# simply so I can run this module as an entrypoint, perform some local tests,
# and/or propogate values easily into sub-modules with less boilerplate.

# get parent package variables
from Tools.IDE import _PATH_DCCSIG
from Tools.IDE import _PATH_DCCSI_TOOLS
from Tools.IDE import _PATH_DCCSI_TOOLS_IDE

# set up access to this Wing IDE folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_DCCSI_TOOLS_IDE_WING = Path(_MODULE_PATH.parent)
site.addsitedir(_DCCSI_TOOLS_IDE_WING.as_posix())

from Tools.IDE import _DCCSI_GDEBUG
from Tools.IDE import _DCCSI_DEV_MODE
from Tools.IDE import _DCCSI_GDEBUGGER

# message collection
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'_DCCSI_TOOLS_IDE_WING: {_DCCSI_TOOLS_IDE_WING}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if _DCCSI_DEV_MODE:
    from azpy.config_utils import attach_debugger

    attach_debugger(debugger_type=_DCCSI_GDEBUGGER)

    from azpy.shared.utils.init import test_imports
    # If in dev mode this will test imports of __all__
    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')
    test_imports(_all=__all__,_pkg=_PACKAGENAME,_logger=_LOGGER)
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run as main, perform debug and tests"""
    pass
