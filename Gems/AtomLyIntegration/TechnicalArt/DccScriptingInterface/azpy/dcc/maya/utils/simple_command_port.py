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
import socket
import site
import time
import logging as _logging

# -- External Python modules --
# none

# -- Extension Modules --
# none (yet)

# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'azpy.dcc.maya.utils.simple_command_port'
_LOGGER = _logging.getLogger(_MODULENAME)

_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class SimpleCommandPort:
    """
    Simple maya command port Object Class.
    """
    #----------------------------------------------------------------------
    #def __new__(self):
        #try:
            #self.port_name
            #return self
        #except NameError as e:
            #self.port_name = None
            #self.logger.error("Specify a port like: '127.0.0.1:6000'")
            #return
        #return

    # -- Constructor ------------------------------------------------------
    def __init__(self,
                 local_host=_LOCAL_HOST,
                 comman_port=6000,
                 logger=_LOGGER,
                 source_type='python',
                 echo_output=True,
                 noreturn=False,
                 *args, **kwargs):
        '''
        SimpleCommandPort Class Initialization

        < To Do: Need to document >

        Input Attributes:
        -----------------
        self.           SCALAR: Description.
                        Default =

        Keyword Arguments:
        ------------------
        self.           STRING: Description.
                        Default =
        self.           OBJECT: Description.
                        Default =

        Additional Attributes:
        ----------------------
        self.           BOOLEAN: Description.
                        Default =

        Documentation last updated: Month. Day, Year - Author
        '''

        # -- Default Values --
        self._port_name = '0.0.0.0:0000'
        self._port = None
        self._logger = None
        self._echo_output = echo_output
        self._noreturn = noreturn
        self._source_type = source_type

        if logger != None:
            self._logger = logger

        # -- Input Checks --
        if local_host != None and local_host != '0.0.0.0':
            self._port_name = str('{0}:{1}'.format(local_host, comman_port))
        else:
            self.logger.error("Specify a port: SimpleCommandPort('127.0.0.1','6000')")

        ## -- init --
        #try:
            #self.port = self.cmdPortOpen()
        #except Exception as e:
            #self.port = None
            #if self.logger:
                #self.logger.autolog(e, level='error')
            #else:
                #self.logIt(e, level='error')
    #----------------------------------------------------------------------


    #--properties----------------------------------------------------------
    @property
    def port_name(self):
        return self._port_name

    @port_name.setter
    def port_name(self, value):
        self._port_name = value
        return self._port_name

    @port_name.getter
    def port_name(self):
        return self._port_name

    @property
    def port(self):
        return self._port

    @port.setter
    def port(self, value):
        self._port = value
        return self._port   

    @property
    def logger(self):
        return self._logger

    @logger.setter
    def logger(self, value):
        self._logger = value
        return self._logger 
    #----------------------------------------------------------------------


    # --method-------------------------------------------------------------
    def open(self):
        from azpy.dev.utils.check.maya_app import validate_state
        if validate_state():
            if self.port_name != None and self.port_name != '0.0.0.0:0000':
                import maya.cmds as cmds
                try:
                    self.logger.info('Opening the cmd port: {0}'
                                     ''.format(self.port_name))
                except Exception as e:
                    self.logger.error('{0}'.format(e))

                try:
                    # to do: get mel working
                    self.port = cmds.commandPort(name=self.port_name,
                                                 echoOutput=self._echo_output,
                                                 sourceType =self._source_type,
                                                 noreturn=self._noreturn)
                    self.logger.info('{0}:: Open!'.format(self.port_name))
                    time.sleep(0.25)
                    return True, self.port_name
                except Exception as e:
                    self.logger.error('{0}'.format(e))
                    _LOGGER.info(cmds.commandPort(self.port_name, q=True))
                    self.port_name='ERROR'
                    return False, self.port_name
            else:
                self.logger.warning('Can not use a port: {}'.format(self.port_name))
        else:
            self.logger.warning('Did not perform port open, Not running Maya!')
            return False
    #----------------------------------------------------------------------


    # --method-------------------------------------------------------------
    def close(self):

        self.logger.info('Closing the port: {0}'.format(self.port_name))

        try:
            import maya.cmds as cmds
            cmds.commandPort(name=self.port_name, close=True, echoOutput=self._echo_output)
            self.port = None
            self.logger.info('{0}:: Port Closed!'.format(self.port_name))
        except Exception as e:
            self.logger.error('{0}'.format(e))
            self.port_name = 'ERROR'
            return False

        time.sleep(1)

        return True
    #----------------------------------------------------------------------
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
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

    # should throw an error message
    foo_port = SimpleCommandPort()

    # should attemp to open the port, should warn as defualt port is '0.0.0.0:0000'
    foo_port.open() 

    # should return a port object
    foo_port = SimpleCommandPort('127.0.0.1:6000')

    # should attemp to open the port, which should warn because only works in Maya
    foo_port.open()

    _LOGGER.info('Port Name: {}'.format(foo_port.port_name))
