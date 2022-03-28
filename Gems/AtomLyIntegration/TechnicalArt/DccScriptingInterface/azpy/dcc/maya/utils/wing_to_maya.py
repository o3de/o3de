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
import site
import socket
import time
import logging as _logging

# -- External Python modules --
# none

# -- Extension Modules --
from simple_command_port import SimpleCommandPort

# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'azpy.dcc.maya.utils.wing_to_maya'
_LOGGER = _logging.getLogger(_MODULENAME)

_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
def start_wing_to_maya(local_host=_LOCAL_HOST,
                       command_port=6000,
                       logger=_LOGGER,
                       *args, **kwargs):
    """
    imports the module
    opens a port to run python code from wingIDE directly
    """

    if logger != None:
        _LOGGER = _LOGGER

    try:
        import maya.cmds as cmds
    except ImportError as e:
        _LOGGER.error('Could not perform: {}'.format(e))
        raise e

    try:
        port
    except NameError:
        port = None

    port_name = str('{0}:{1}'.format(local_host, command_port))

    # should only be getting the port passed in
    _LOGGER.info('Attempting to open port:: {0}'.format(port_name))
    port_object = SimpleCommandPort(port_name)

    try:
        port = port_object.open()
    except Exception as e:
        _LOGGER.error('Could not open port: {e}'.format(e))
        raise e

    if not port:
        return port
    else:
        try:
            import execute_wing_code
        except Exception as e:
            _LOGGER.error('Could not execute code: {}'.format(e))
            return 'Error'

        time.sleep(0.25)
        _LOGGER.info('WingIDE: Python >> to >> Maya, is started.')

        try:
            test_port = cmds.commandPort(port_name, q=True)
        except Exception as e:
            _LOGGER.autolog(e, level='error')
            return 'Error'

        if test_port:
            message = 'That port is already open. '
            message += 'Attempting to close old port, so I can re-establish it...'
            _LOGGER.info(message)
            try:
                cmds.commandPort(name=port_name, close=True, echoOutput=True)
            except Exception as e:
                _LOGGER.error(''.format(e))

            time.sleep(0.25)
            try:
                test_port = cmds.commandPort(port_name, q=True)
            except Exception as e:
                _LOGGER.error('Could not query port: {}'.format(e))

            if test_port == False:
                _LOGGER.info('Port closed! Re-opening ...')
                try:
                    port = SimpleCommandPort(port_name)
                except Exception as e:
                    _LOGGER.error('Could not create port object: {}'.format(e))

                time.sleep(0.25)
                try:
                    test_port = cmds.commandPort(port_name, q=True)
                    _LOGGER.info('The posrt is: {}'.format(test_port))
                except Exception as e:
                    _LOGGER.error('Could not query port: {}'.format(e))

        elif test_port == False:
            _LOGGER.info('The port does not exist ... attempting to open it again now')
            try:
                port = SimpleCommandPort(port_name)
            except Exception as e:
                _LOGGER.error('Could not create port: {}'.format(e))

            time.sleep(0.25)

            try:
                test_port = cmds.commandPort(port_name, q=True)
                _LOGGER.info('Port is: {}'.format(test_port))
            except Exception as e:
                _LOGGER.error('Could not create port: {}'.format(e))

        return port
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def start_wing_to_maya_menu():
    """
    Simple hook to call the function from a menu item,
    using the default port name defined
    """
    port = object()  # init a dummy object

    # default name ... name is first arg, or a kwarg
    portName, kwargs = set_synth_arg_kwarg(port, arg_pos_index=0, arg_tag='portName',
                                        in_args=args, in_kwargs=kwargs,
                                        default_value="127.0.0.1:6000")

    port = start_wing_to_maya(local_host=_LOCAL_HOST, command_port=6000)
    return
# -------------------------------------------------------------------------
