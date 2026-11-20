#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
#
"""! Pyside uic utilities

:file: < DCCsi >/azpy/shared/ui/puic_utils.py
:Status: Prototype
:Version: 0.0.1
"""
from pathlib import Path
import logging as _logging
import xml.etree.ElementTree as xml  # Qt .ui files are xml
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.shared.ui.puic_utils'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# ensure dccsi and o3de core access
# in a future iteration it is suggested that the core config
# be rewritten from ConfigClass, then WingConfig inherits core
import DccScriptingInterface.config as dccsi_core_config

# this is currently disabled while trying to get wing to boot from o3de
# Qt/PySide envars are propogating from o3de and cause a wing boot failure
# _settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=True,
#                                                        enable_o3de_pyside=True,
#                                                        set_env=True)

from DccScriptingInterface.azpy.shared.ui import PATH_DCCSI_PYTHON_LIB
import subprocess
import sys
import shutil
import PySide6
from pathlib import Path

# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *
# -------------------------------------------------------------------------

def compileUi(ui_file, pyfile):
    """
    Replacement for pyside2uic.compileUi for PySide6.

    ui_file: path to .ui file
    pyfile: file-like object opened for writing
    """

    uic = shutil.which("pyside6-uic")
    if not uic:
        raise RuntimeError("pyside6-uic not found")

    cmd = [
        sys.executable,
        uic,
        str(ui_file),
        "-o",
        pyfile
    ]

    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    pyfile.write(result.stdout)

# -------------------------------------------------------------------------
def from_ui_generate_form_and_base_class(filename, return_output=False):
    """! Parse a Qt Designer .ui file and return Pyside Form and Base Class
    Usage:
            import azpy.shared.ui as azpyui
            form_class, base_class = azpyui.from_ui_generate_form_and_class(r'C:\my\filepath\tool.ui')

    To Do: write a proper doxygen formated docstring
    """
    ui_file = Path(filename)
    output = ''
    parsed_xml = None
    try:
        ui_file.exists()
    except FileNotFoundError as error:
        output += 'File does not exist: {0}/r'.format(error)
        if _DCCSI_GDEBUG:
            print(error)
        if return_output:
            return False, output
        else:
            return False

    try:
        ui_file.ext == 'ui'
    except IOError as error:
        output += 'Not a Qt Designer .ui file: {0}/r'.format(error)
        if return_output:
            return False, output
        else:
            return False

    parsed_xml = xml.parse(ui_file)
    form_class = parsed_xml.find('class').text
    widget_class = parsed_xml.find('widget').get('class')

    with open(ui_file, 'r') as ui_file:
        stream = StringIO()  # create a file io stream
        frame = {}

        # compile the .ui file as a .pyc represented in steam
        compileUi(ui_file, stream)
        # compile the .pyc bytecode from stream
        pyc = compile(stream.getvalue(), '', 'exec')
        # execute the .pyc bytecode
        exec (pyc, frame)

        # Retreive the form_class and base_class based on type in designer .ui (xml)
        form_class = frame['Ui_{0}'.format(form_class)]
        base_class = eval('QtWidgets.{0}'.format(widget_class))

        ui_file.close()

    if return_output:
        return form_class, base_class, output
    else:
        return form_class, base_class
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    from azpy.constants import STR_CROSSBAR
    from DccScriptingInterface.constants import FRMT_LOG_LONG

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                         datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    # log global state to cli
    _LOGGER.debug(STR_CROSSBAR)
    _LOGGER.debug(f'_MODULENAME: {_MODULENAME}')

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info(f'~ {_MODULENAME}.py ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)

    # can run local tests
    if DCCSI_TESTS: # from DccScriptingInterface.globals
        # this will validate pyside bootstrapping
        foo = dccsi_core_config.test_pyside(exit = False)
        pass
