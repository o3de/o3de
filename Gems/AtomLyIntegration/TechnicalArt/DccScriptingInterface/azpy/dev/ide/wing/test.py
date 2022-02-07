# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""This is a module to test script extensions for WingIDE
reference: https://wingware.com/doc/scripting/example

note: there are important instructions in that doc for
setting up your project files with wingapi auto-complete, etc.

We added C:\Program Files (x86)\Wing Pro 7.1\src to the
PYTHONPATH via env.bat and dynaconf config instead so it is
part of the inhereted environment."""

import sys
import logging as _logging
import wingapi

# -------------------------------------------------------------------------
_MODULENAME = 'azpy.dev.ide.wing.test'
_LOGGER = _logging.getLogger(_MODULENAME)
_handler = _logging.StreamHandler(sys.stdout)
_handler.setLevel(_logging.DEBUG)
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_formatter = _logging.Formatter(FRMT_LOG_LONG)
_handler.setFormatter(_formatter)
_LOGGER.addHandler(_handler)
_LOGGER.debug('Loading: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def dccsi_test_script(test_str):
    """Simple test command for WingIDE
    
    to run in wing: Edit > Command by Name
    ^ opens a commanline at bottom of IDE
    
    type: test-script (then return)
    ^ commandline now takes entering a Test Str
    
    Test Str: Booyah
    ^ a Pop-up dialog will display in wing
    
    """
    app = wingapi.gApplication
    v = "Product info is: " + str(app.GetProductInfo())
    v += "\nAnd you typed: %s" % test_str
    wingapi.gApplication.ShowMessageDialog("Test Message", v)
    
#dccsi_test_script.contexts = [wingapi.kContextNewMenu("Scripts")]

# this will add to a menu in WingIDE
dccsi_test_script.contexts = [
  wingapi.kContextNewMenu("DCCsi Scripts"),
  wingapi.kContextEditor(),
]
# -------------------------------------------------------------------------

# bind a hotkey inside Wing that will execute our newly installed Module
# Inside Wing, choose Edit -> Preferences, and on the left, under User Interface, choose Keyboard
# In the center right of the Keyboard section is where you can add "Ccustom Key Bindings" combinations to execute code
# For this test example, I have bound Ctrl+Alt+Shift+T as my key combination for executing dccsi_test_script

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # there are not really tests to run here due to this being a list of
    # constants for shared use.
    _DCCSI_GDEBUG = True
    _DCCSI_DEV_MODE = True
    _LOGGER.setLevel(_logging.DEBUG)  # force debugging
    
    foo = dccsi_test_script("This is a test")
