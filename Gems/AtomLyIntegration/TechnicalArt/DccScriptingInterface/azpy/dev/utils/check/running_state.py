# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
# -- Standard Python modules --
import sys
import os
import logging as _logging

# -- External Python modules --
# none

# -- Extension Modules --
# none (yet)

# --------------------------------------------------------------------------
# -- Global Definitions --
_DCCSI_G_DCC_APP = None

_MODULENAME = 'azpy.dev.utils.check.running_state'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# First Class
class CheckRunningState(object):
    """
    < To Do: document Class >
    """

    # Class Variables
    DCCSI_G_DCC_APP = None

    def __init__(self, *args, **kwargs):
        '''
        CheckRunningState Class Initialization

        < To Do: Need to document >

        Input Attributes:
        -----------------
        self. ->        SCALAR: Description.
                        Default =

        Keyword Arguments:
        ------------------
        self. ->        STRING: Description.
                        Default =
        self. ->        OBJECT: Description.
                        Default =

        Additional Attributes:
        ----------------------
        self. ->        BOOLEAN: Description.
                        Default =

        Documentation last updated: Month. Day, Year - Author
        '''

        # -- Default Values --
        # top level storage for whether or not we are running
        # in a DCC tool interpreter
        self._dcc_py = False

        # basic python info
        # if these can't run, we are in a bad state anyway
        self._python = sys.version
        self._py_version_info = sys.version_info

        # -- Input Checks --
        self.check_known()
    # ---------------------------------------------------------------------

    # --method-------------------------------------------------------------
    def check_known(self):
        # -- init --
        # first let's check if any of these DCC apps are running

        # To Do?: Add O3DE checks, treat as a DCC app?
        
        # 0 - maya first
        CheckRunningState.DCCSI_G_DCC_APP = self.maya_running()

        # 1 - then max
        if not CheckRunningState.DCCSI_G_DCC_APP:
            CheckRunningState.DCCSI_G_DCC_APP = self.max_running()
        else:
            _LOGGER.warning('DCCSI_G_DCC_APP is already set: {}'.format(CheckRunningState.DCCSI_G_DCC_APP))

        # 2 - then blender
        if not CheckRunningState.DCCSI_G_DCC_APP:
            CheckRunningState.DCCSI_G_DCC_APP = self.blender_running()
        else:
            _LOGGER.warning('DCCSI_G_DCC_APP is already set: {}'.format(CheckRunningState.DCCSI_G_DCC_APP))

        # store checks for DCC info
        if CheckRunningState.DCCSI_G_DCC_APP:
            self.dcc_py = True

        # store check for is maya  running headless
        if CheckRunningState.DCCSI_G_DCC_APP == 'maya':
            self.maya_headless = self.is_maya_headless()

        # set a envar other modules can easily check
        if CheckRunningState.DCCSI_G_DCC_APP:
            os.environ['DCCSI_G_DCC_APP'] = CheckRunningState.DCCSI_G_DCC_APP
    # ---------------------------------------------------------------------


    #--properties----------------------------------------------------------
    @property
    def python(self):
        return self._python

    @python.setter
    def python(self, value):
        self._python = value
        return self._python

    @property
    def py_version_info(self):
        return self._py_version_info

    @py_version_info.setter
    def py_version_info(self, value):
        self._py_version_info = value
        return self._py_version_info

    @property
    def dcc_app(self):
        return self._dcc_py

    @dcc_app.setter
    def dcc_app(self, value):
        self._dcc_py = value
        return self._dcc_py

    @property
    def maya_headless(self):
        return self._maya_headless

    @maya_headless.setter
    def maya_headless(self, value):
        self._maya_headless = value
        return self._maya_headless

    # template property
    # @property
    # def foo(self):
        # return self._foo

    # @foo.setter
    # def foo(self, value):
        #self._foo = value
        # return self._foo
    # --properties----------------------------------------------------------

    # --method-------------------------------------------------------------
    def maya_running(self):
        """< To Do: Need to document >"""
        try:
            import azpy.dev.utils.check.maya_app as check_dcc
            DCCSI_G_DCC_APP = check_dcc.validate_state()
        except ImportError as e:
            _LOGGER.info('Not Implemented: azpy.dev.utils.check.maya_app')
        if DCCSI_G_DCC_APP:
            CheckRunningState.DCCSI_G_DCC_APP = check_dcc.validate_state()
            os.environ["DCCSI_G_DCC_APP"] = str(DCCSI_G_DCC_APP)
        return CheckRunningState.DCCSI_G_DCC_APP
    #----------------------------------------------------------------------

    # --method-------------------------------------------------------------
    def is_maya_headless(self):
        """< To Do: Need to document >"""
        if self.maya_app:
            import maya.cmds as mc
            try:
                if mc.about(batch=True):
                    return True
                else:
                    return False
            except Exception as e:
                # cmds module isn't fully loaded/populated
                # (which only happens in batch, maya.standalone, or maya GUI)
                # no maya
                return False
        else:
            return False
    # --method-------------------------------------------------------------


    # --method-------------------------------------------------------------
    def max_running(self):
        """
        < To Do: implement >
        """
        try:
            import azpy.dev.utils.check.max_app as check_dcc
            CheckRunningState.DCCSI_G_DCC_APP = check_dcc.validate_state()
        except ImportError as e:
            _LOGGER.info('Not Implemented: azpy.dev.utils.check.max')
        if CheckRunningState.DCCSI_G_DCC_APP:
            CheckRunningState.DCCSI_G_DCC_APP = check_dcc.validate_state()
            os.environ["DCCSI_G_DCC_APP"] = str(CheckRunningState.DCCSI_G_DCC_APP)
        return CheckRunningState.DCCSI_G_DCC_APP
    #----------------------------------------------------------------------


    # --method-------------------------------------------------------------
    def blender_running(self):
        """
        < To Do: implement >
        """
        try:
            import azpy.dev.utils.check.blender_app as check_dcc
            CheckRunningState.DCCSI_G_DCC_APP = check_dcc.validate_state()
        except ImportError as e:
            _LOGGER.info('Not Implemented: azpy.dev.utils.check.blender')
        if CheckRunningState.DCCSI_G_DCC_APP:
            CheckRunningState.DCCSI_G_DCC_APP = check_dcc.validate_state()
            os.environ["DCCSI_G_DCC_APP"] = str(CheckRunningState.DCCSI_G_DCC_APP)
        return CheckRunningState.DCCSI_G_DCC_APP
    #----------------------------------------------------------------------


#==========================================================================
# Class Test
#==========================================================================
if __name__ == '__main__':
    _DCCSI_GDEBUG = True
    _DCCSI_DEV_MODE = True
    _LOGGER.setLevel(_logging.DEBUG)  # force debugging

    # -- Extend Logger
    #_handler = _logging.StreamHandler(sys.stdout)
    # _handler.setLevel(_logging.DEBUG)
    #FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
    #_formatter = _logging.Formatter(FRMT_LOG_LONG)
    # _handler.setFormatter(_formatter)
    # _LOGGER.addHandler(_handler)
    #_LOGGER.debug('Loading: {0}.'.format({_MODULENAME}))

    # happy print
    from azpy.constants import STR_CROSSBAR
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('{} ... Running script as __main__'.format(_MODULENAME))
    _LOGGER.info(STR_CROSSBAR)

    foo = CheckRunningState()
    _LOGGER.info('DCCSI_G_DCC_APP: {}'.format(foo.DCCSI_G_DCC_APP))
