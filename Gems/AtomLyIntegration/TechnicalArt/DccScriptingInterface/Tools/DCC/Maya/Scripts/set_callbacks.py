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
"""
Module Documentation:
    DccScriptingInterface:: SDK//maya//scripts//set_callbacks.py

This module manages a set of predefined callbacks for maya
"""
# -------------------------------------------------------------------------
# -- Standard Python modules
import os
import sys
import logging as _logging
# -- External Python modules
from box import Box
# maya imports
import maya.cmds as mc
import maya.api.OpenMaya as om
# -- DCCsi Extension Modules
from azpy.constants import *
import azpy.dcc.maya
azpy.dcc.maya.init()  # <-- should have already run?
import azpy.dcc.maya.callbacks.event_callback_handler as azEvCbH
import azpy.dcc.maya.callbacks.node_message_callback_handler as azNdMsH
# Node Message Callback Setup
import azpy.dcc.maya.callbacks.on_shader_rename as oSR
from set_defaults import set_defaults
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, True)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, True)

_MODULENAME = r'DCCsi.SDK.Maya.Scripts.set_callbacks'

_LOGGER = azpy.initialize_logger(_MODULENAME, default_log_level=int(20))
_LOGGER.debug('Invoking:: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope callbacks, set up set and initialize all to None
# To Do: should callback initialization use data-driven settings?
# To Do: should we move callback initialization to a sub-module?
# To Do: move the callback key like 'NewSceneOpened' here (instead of None)
# ^ this would provide ability to loop through and replace key with CB object

_G_CALLBACKS = Box(box_dots=True)  # global scope container
_G_PRIMEKEY = 'DCCsi_callbacks'
_G_CALLBACKS[_G_PRIMEKEY] = True  # required prime key


# -------------------------------------------------------------------------
def init_callbacks(_callbacks=_G_CALLBACKS):
    # store as a dict (Box is a fancy dict)
    _callbacks[_G_PRIMEKEY] = True  # required prime key
    
    # signature dict['callback key'] = ('CallBack'(type), func, callbackObj)
    _callbacks['on_new_file'] = ['NewSceneOpened', set_defaults, None]
    _callbacks['new_scene_fix_paths'] = ['NewSceneOpened', install_fix_paths, None]
    _callbacks['post_scene_fix_paths'] = ['PostSceneRead', install_fix_paths, None]
    _callbacks['workspace_changed'] = ['workspaceChanged', update_workspace, None]
    _callbacks['quit_app'] = ['quitApplication', uninstall_callbacks, None]
    
    # nodeMessage style callbacks
    # fire a function
    _func_00 = oSR.on_shader_rename_rename_shading_group
    # using a nodeMessage callback trigger
    _cb_00 = om.MNodeMessage.addNameChangedCallback
    # all nodeMessage type callbacks can use 'nodeMessageType' key
    _callbacks['shader_rename'] = ['nodeMessageType', (_func_00, _cb_00), None]

    return _callbacks
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def uninstall_callbacks():
    """Bulk uninstalls hte globally defined set of callbacks:
    _G_callbacks"""

    global _G_CALLBACKS

    _LOGGER.debug('uninstall_callbacks() fired')

    for key, value in _G_CALLBACKS:
        if value[2] is not None:  # have a cb
            value[2].uninstall()  # so uninstall it
        else:
            _LOGGER.warning('No callback in: key {0}, value:{1}'
                            ''.format(key, value))
    _G_CALLBACKS = None
    _LOGGER.info('DCCSI CALLBACKS UNINSTALLED ... EXITING')
    return _G_CALLBACKS
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_callbacks(_callbacks=_G_CALLBACKS):
    """Bulk installs the globally defined set of callbacks:
    _G_callbacks"""
    
    _LOGGER.debug('install_callback_set() fired')
    
    _callbacks = init_callbacks(_callbacks)

    # we initialized the box with this so pop it
    if 'box_dots' in _callbacks:
        _callbacks.pop('box_dots')

    # don't pass anything but carefully considered dict
    if _G_PRIMEKEY in _callbacks:
        _primekey = _callbacks.pop(_G_PRIMEKEY)
    else:
        _LOGGER.error('No prime key, use a correct dictionary')
        #To Do: implement error handling and return codes
        return _callbacks[None]

    for key, value in _G_CALLBACKS.items():
        # we popped the prime key should the rest should be safe
        if value[0] != 'nodeMessageType':
            # set callback up
            _cb = azEvCbH.EventCallbackHandler(value[0],
                                               value[1])
            # ^ installs by default
            # stash it back into managed dict
            value[2] = _cb
            # value[2].install()
        else:
            # set up callback, value[1] should be tupple(func, trigger)
            _cb = azNdMsH.NodeMessageCallbackHandler(value[1][0],
                                                     value[1][1])
            # ^ installs by default
            # stash it back into managed dict
            value[2] = _cb
            # value[2].install()

    return _callbacks
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_fix_paths(foo=None):
    """Installs and triggers a fix paths module.
    This can repair broken reference paths in shaders"""
    global _fix_paths
    _fix_paths = None

    _LOGGER.debug('install_fix_paths() fired')

    # if we don't have it already, this function is potentially triggered
    # by a callback, so we don't need to keep importing it.
    try:
        _fix_paths
        reload(_fix_paths)
    except Exception as e:
        try:
            import fixPaths as _fix_paths
        except Exception as e:
            # To Do: not implemented yet
            _LOGGER.warning('NOT IMPLEMENTED: {0}'.format(e))

    # if we have it, use it
    if _fix_paths:
        return _fix_paths.main()
    else:
        # To Do: implement error handling and return codes
        return 1
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def update_workspace(foo=None):
    """Forces and update of the workspace (workspace.mel)"""
    _LOGGER.debug('update_workspace() fired')
    result = mc.workspace(update=True)
    return result
# -------------------------------------------------------------------------

# install and init callbacks on an import obj
_G_CALLBACKS = install_callbacks(_G_CALLBACKS)

# ==========================================================================
# Module Tests
#==========================================================================
if __name__ == '__main__':
    _G_CALLBACKS = install_callbacks(_G_CALLBACKS)
