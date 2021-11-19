# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
# <DCCsi>\\azpy\\maya\\\callbacks\\event_callback_handler.py
# Maya event callback handler
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------

"""
Module Documentation:
    <DCCsi>:: azpy//maya//callbacks//event_callback_handler.py

.. module:: event_callback_handler
    :synopsis: Simple event based callback_event handler
    using maya.api.OpenMaya (api2)

.. :note: nothing mundane to declare
.. :attention: callbacks should be uninstalled on exit
.. :warning: maya may crash on exit if callbacks are not uninstalled

.. Usage:
    def test_func(*arg):
        _logging.debug("~ test_func ccallbackEvent fired! arg={0}"
                       "".format(arg))

    #register an event based based callback event
    cb = EventCallbackHandler('NameChanged', test_func)

.. Reference:
    The following call will return all the available events that can be
    passed into the EventCallbackHandler.

        import maya.api.OpenMaya as openmaya
        openmaya.MEventMessage.getEventNames()

    Important ones for quick reference are:
        quitApplication
        SelectionChanged
        NameChanged
        SceneSaved
        NewSceneOpened
        SceneOpened
        PostSceneRead
        workspaceChanged

.. moduleauthor:: Amazon Lumberyard
"""

#--------------------------------------------------------------------------
# -- Standard Python modules
import os

# -- External Python modules

# -- Lumberyard Extension Modules
import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# -- Maya Modules
import maya.api.OpenMaya as openmaya
#--------------------------------------------------------------------------


#--------------------------------------------------------------------------
# -- Misc Global Space Definitions
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.dcc.maya.callbacks.event_callback_handler'

_LOGGER = azpy.initialize_logger(_PACKAGENAME, default_log_level=int(20))
_LOGGER.debug('Invoking:: {0}.'.format({_PACKAGENAME}))
# --------------------------------------------------------------------------


# =========================================================================
# First Class
# =========================================================================
class EventCallbackHandler(object):
    """
    A simple Maya event based callback_event handler class

    :ivar callback_event: stores event type trigger for a maya callback_event
    :vartype event: for example, 'NameChanged'

    :ivar this_function: stores this_function to call when callback_event is triggered
    :vartype this_function: for example,
            cb = EventCallbackHandler(callback_event='NameChanged',
                                      this_function=test_func)
    """

    # --BASE-METHODS-------------------------------------------------------
    # --constructor-
    def __init__(self, callback_event, this_function, install=True):
        """
        initializes a callback_event object        
        """
        # callback_event id storage
        self._callback_id = None
        # state tracker
        self._message_id_set = None

        # the callback_event event trigger
        self._callback_event = callback_event
        # the thing to do on callback_event
        self._function  = this_function

        if install:
            self.install()

    # --properties---------------------------------------------------------
    @property
    def callback_id(self):
        return self._callback_id

    @property
    def callback_event(self):
        return self._callback_event    

    @property
    def this_function(self):
        return self._this_function

    # --method------------------------------------------------------------- 
    def install(self):
        """
        installs this callback_event for event, which makes it active
        """

        add_event_method = openmaya.MEventMessage.addEventCallback

        # when called, check if it's already installed
        if self._callback_id:
            _LOGGER.warning("EventCallback::{0}:{1}, is already installed"
                            "".format(self._callback_event,
                                      self._function.__name__))
            return False

        # else try to install it
        try:
            self._callback_id = add_event_method(self._callback_event,
                                                 self._function)
        except Exception as e:
            _LOGGER.error("Failed to install EventCallback::'{0}:{1}'"
                          "".format(self._callback_event,
                                    self._function.__name__))
            self._message_id_set = False
        else:
            _LOGGER.debug("Installing EventCallback::{0}:{1}"
                          "".format(self._callback_event,
                                    self._function.__name__))
            self._message_id_set = True

        return self._callback_id

    # --method------------------------------------------------------------- 
    def uninstall(self):
        """
        uninstalls this callback_event for the event, deactivates
        """

        remove_event_callback = openmaya.MEventMessage.removeCallback

        if self._callback_id:
            try:
                remove_event_callback(self._callback_id)
            except Exception as e:
                _LOGGER.error("Couldn't remove EventCallback::{0}:{1}"
                              "".format(self._callback_event,
                                        self._function.__name__))

            self._callback_id = None
            self._message_id_set = None
            _LOGGER.debug("Uninstalled the EventCallback::{0}:{1}"
                          "".format(self._callback_event,
                                    self._function.__name__))
            return True
        else:
            _LOGGER.warning("EventCallback::{0}:{1}, not currently installed"
                            "".format(self._callback_event,
                                      self._function.__name__))
            return False   

    # --method------------------------------------------------------------- 
    def __del__(self):
        """
        if object is deleted, the callback_event is uninstalled
        """
        self.uninstall()
# -------------------------------------------------------------------------


#==========================================================================
# Class Test
#==========================================================================
if __name__ == "__main__":

    def test_func(*arg):
        print("~ test_func callback_event fired! arg={0}"
              "".format(arg))

    cb = EventCallbackHandler('NameChanged', test_func)

    cb.install()
    # callback_event is active

    #cb.uninstall()
    ## callback_event not active

