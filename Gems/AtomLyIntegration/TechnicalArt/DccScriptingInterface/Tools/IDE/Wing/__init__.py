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
interpreter, and sometimes these python interpreters are managed, such
as mayapy.exe

This integration allows each DCC py interpreter, to be specified in a
data-driven manner as a dev/launch environment; and since the DCCsi
has portions of API that are shared py3 code, you may want to test the
code against several different interpreters in a similar environment as
the code would operate normally outside of the IDE and within the DCC app.

So for instance,
The Wing IDE launch environment, may specify the following:

:: shared location for 64bit DCCSI_PY_MAYA python location
set "DCCSI_PY_MAYA=%MAYA_BIN_PATH%\mayapy.exe"

Then within Wing IDE, there will be a set of Python Shell environments,
each can start a defined interpreter.

DCCSI_PY_O3DE = ${DCCSI_PY_O3DE}
DCCSI_PY_MAYA = ${DCCSI_PY_MAYA}
...

And there are times special considerations to be made, for instance
intellisense auto-complete with mayapy.exe doesn't work unless mayapy.exe
is that actual interpreter specified.

In Wing you can switch the interpreter via:
Console (panel) > Python Shell (tab) > Options > Use Environment ...

This integration also hooks into bootstrapping code, which can auto-start
and attach to Wing as the external debugger from other entrypoints and code,
such as a plugin or tool starting within O3DE.

This is a python script driven replacement for files found here:
    < DCCsi >\Tools\Dev\Windows\*.bat

Notes:
- The old .bat launchers and some DCCsi code supported py2.7+, but we are
deprecating support because of end-of-life

- The old .bat launchers supported Wing 7.x, however that version of Wing
also shipped with py2.7 as it's own managed interpreter, so we do not
intend to implement support for older versions of wing < 8. However, wing
can support py3 interpreters and debugging so it's possible it will
continue to work.

- I am a paid developer, so I am using Wing Pro. I need a fully featured IDE
to develop with.  This increment of work does not intend to support Wing
Community edition, I consider that future work.

- The old .bat files are a great fallback to avoid a catch22; I am using
Wing to develop the framework for a managed environment, launching and
bootstrapping of tools like Wing. So the .bat files will be updated to
Wing 8.x support and stick around to start Wing on windows if and when the
framework is broken and inoperable.

"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.IDE import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.Wing'

__all__ = ['globals',
           'config',
           'constants'
           'discovery',
           'start']
          #'Foo',

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up access to this IDE/Wing folder as a pkg
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

from DccScriptingInterface import add_site_dir
from DccScriptingInterface import SETTINGS_FILE_SLUG
from DccScriptingInterface import LOCAL_SETTINGS_FILE_SLUG

# these are here to support debugging with wing now during development
from DccScriptingInterface import SLUG_DCCSI_WING_TYPE
from DccScriptingInterface import SLUG_DCCSI_WING_VERSION_MAJOR

from DccScriptingInterface import PATH_WINGHOME
from DccScriptingInterface import PATH_WING_APPDATA

from DccScriptingInterface.constants import PATH_PROGRAMFILES_X86
from DccScriptingInterface.constants import USER_HOME
from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS_IDE_WING

# last two parents
from DccScriptingInterface.Tools.IDE import PATH_DCCSI_TOOLS
from DccScriptingInterface.Tools.IDE import PATH_DCCSI_TOOLS_IDE
from DccScriptingInterface.globals import *

# set up access to this Wing IDE folder as a pkg
_DCCSI_TOOLS_IDE_WING = Path(_MODULE_PATH.parent)
add_site_dir(_DCCSI_TOOLS_IDE_WING.as_posix())

# the path to this < dccsi >/Tools/IDE pkg
PATH_DCCSI_TOOLS_IDE_WING = Path(_MODULE_PATH.parent)
PATH_DCCSI_TOOLS_IDE_WING = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS_IDE_WING,
                                           PATH_DCCSI_TOOLS_IDE_WING.as_posix()))
add_site_dir(PATH_DCCSI_TOOLS_IDE_WING.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS_IDE_WING}: {PATH_DCCSI_TOOLS_IDE_WING}')
_LOGGER.debug(STR_CROSSBAR)

# from dynaconf import LazySettings

PATH_DCCSI_TOOLS_IDE_WING_SETTINGS = PATH_DCCSI_TOOLS_IDE_WING.joinpath(SETTINGS_FILE_SLUG).resolve()
PATH_DCCSI_TOOLS_IDE_WING_LOCAL_SETTINGS = PATH_DCCSI_TOOLS_IDE_WING.joinpath(LOCAL_SETTINGS_FILE_SLUG).resolve()

# settings = LazySettings(
#     SETTINGS_FILE_FOR_DYNACONF=PATH_DCCSI_TOOLS_IDE_WING.as_posix(),
#     INCLUDES_FOR_DYNACONF=[PATH_DCCSI_TOOLS_IDE_WING_LOCAL_SETTINGS.as_posix()]
# )
#
# settings.setenv()
# -------------------------------------------------------------------------
