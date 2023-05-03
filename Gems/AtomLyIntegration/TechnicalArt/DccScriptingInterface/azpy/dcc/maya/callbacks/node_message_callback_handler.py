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
# <DCCsi>\\azpy\\maya\\\callbacks\\node_message_callback_handler.py
# Maya node message callback handler
# -------------------------------------------------------------------------
"""
.. module:: node_message_callback_handler
    :synopsis: this module contains code related to mNodeName message based callbacks in Maya

.. moduleauthor:: Amazon Lumberyard

.. :note: nothing mundane to declare
.. :attention: callbacks should be uninstalled on exit
.. :warning: maya may crash on exit if callbacks are not uninstalled

.. Usage:
    < To Do >

.. Version:
    0.1.0 | prototype

.. History:
    < To Do >

.. Reference:
    MNodeMessage
    This class is used to register callbacks for dependency mNodeName messages of specific dependency nodes.
    http://download.autodesk.com/us/maya/2011help/API/class_m_node_message.html

    There are 4 add thisCallback methods which will add callbacks for the following types of messages:
    - Attribute Changed
    - Attribute Added or Removed
    - Node Dirty
    - Name Changed

    If we import OpenMaya,
    import maya.api.OpenMaya as om
    from maya.api.OpenMaya import MNodeMessage as mNM

    The valid callbacks for usage are:
    mNM.addAttributeChangedCallback
    mNM.addAttributeAddedOrRemovedCallback
    mNM.addNodeDirtyCallback
    mNM.addNodeDirtyPlugCallback
    mNM.addNameChangedCallback
    mNM.addNodeAboutToDeleteCallback
    mNM.addNodePreRemovalCallback
    mNM.addNodeDestroyedCallback
    mNM.addKeyableChangeOverride
"""

# --------------------------------------------------------------------------
# -- Standard Python modules
import os
import logging as _logging

# -- External Python modules

# -- Lumberyard Extension Modules
from DccScriptingInterface.globals import *

# -- Maya Modules --
import maya.api.OpenMaya as om
import maya.cmds as mc
#--------------------------------------------------------------------------


#--------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy.dcc.maya.callbacks import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.node_message_callback_handler'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Invoking:: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# =========================================================================
# First Class
# =========================================================================
class NodeMessageCallbackHandler(object):
    """
    < To Do: document Class >
    """
    # --BASE-METHODS-------------------------------------------------------
    # --constructor-
    def __init__(self,
                 this_function,
                 this_callback,
                 mNodeName=None,  # keep maya camel case formatting?
                 install=True,
                 *args, **kwargs):
        """
        initializes a this_callback object
        """
        # this_callback id storage
        self._callback_type_id = None
        # state tracker
        self._message_id_set = None

        # the this_callback event trigger
        self._this_callback = this_callback
        # the thing to do on this_callback
        self._function  = this_function

        # this handlers object mNodeName
        # passing in a null MObject (ie, without a name as an argument)
        # registers the this_callback to get all name changes in the scene
        # if you wanted to monitor a specific object's name changes
        # you could pass a name to the MObject
        if mNodeName is None:
            self._m_object = om.MObject()
        else:
            self._m_object = om.MObject(mNodeName)

        # install / activate this callback
        if install:
            self.install()
    #----------------------------------------------------------------------


    #--properties----------------------------------------------------------
    @property
    def callback_id(self):
        return self._callback_type_id

    @property
    def this_callback(self):
        return self._this_callback

    @property
    def this_function(self):
        return self._this_function
    #--properties----------------------------------------------------------


    # --method-------------------------------------------------------------
    def install(self):
        """
        installs this this_callback for event, which makes it active
        """
        # when called, check if it's already installed
        if self._callback_type_id:
            _LOGGER.warning("NodeMessageCallback::{0}:{1}, is already installed"
                            "".format(self._this_callback,
                                      self._function.__name__))
            return False

        # else try to install it
        try:
            self._callback_type_id = self._this_callback(self._m_object,
                                                         self._function)
        except Exception as e:
            _LOGGER.error("Failed to install NodeMessageCallback::'{0}:{1}'"
                          "".format(self._this_callback,
                                    self._function.__name__))
            self._message_id_set = False
        else:
            _LOGGER.debug("Installing NodeMessageCallback::{0}:{1}"
                          "".format(self._this_callback,
                                    self._function.__name__))
            self._message_id_set = True

        return self._callback_type_id
    #----------------------------------------------------------------------


    # --method-------------------------------------------------------------
    def uninstall(self):
        """
        uninstalls this thisCallback for the event, deactivates
        """
        if self._callback_type_id:
            try:
                om.MMessage.removeCallback(self._callback_type_id)
            except Exception as e:
                _LOGGER.error("Couldn't remove NodeMessageCallback::{0}:{1}"
                              "".format(self._this_callback,
                                        self._function.__name__))

            self._callback_type_id = None
            self._message_id_set = None
            _LOGGER.debug("Uninstalled the NodeMessageCallback::{0}:{1}"
                          "".format(self._this_callback,
                                    self._function.__name__))
            return True
        else:
            _LOGGER.warning("NodeMessageCallback::{0}:{1}, not currently installed"
                            "".format(self._this_callback,
                                      self._function.__name__))
            return False
    #----------------------------------------------------------------------


    # --method-------------------------------------------------------------
    def __del__(self):
        """
        if object is deleted, the thisCallback is uninstalled
        """
        self.uninstall()
    #----------------------------------------------------------------------
# -------------------------------------------------------------------------


# =========================================================================
# Public Functions
# =========================================================================
# --First Function---------------------------------------------------------
def testNameChanged(*args):
    # get node
    try:
        mNode = args[0]
    except Exception as e:
        mNode = None
        _LOGGER.debug('\t~ no node')
        _LOGGER.debug('\t~ warning: {0}'.format(e))

    # get old name
    try:
        oldName = args[1]
    except Exception as e:
        oldName = None
        _LOGGER.debug('\t~ no oldName')
        _LOGGER.debug('\t~ warning: {0}'.format(e))

    # convert the MObject to a dep mNode
    try:
        depNode = om.MFnDependencyNode(mNode)
    except Exception as e:
        depNode = None
        _LOGGER.debug('\t~ no depNode')
        _LOGGER.debug('\t~ warning: {0}'.format(e))

    if oldName == (u""): oldName = 'null'

    # get node type
    try:
        nodeType = depNode.typeName()
    except Exception as e:
        nodeType = None
        _LOGGER.debug('\t~ no nodeType')
        _LOGGER.debug('\t~ warning: {0}'.format(e))

    # get node name
    try:
        nodeName = depNode.name()
    except Exception as e:
        nodeName = None
        _LOGGER.debug('\t~ no nodeType')
        _LOGGER.debug('\t~ warning: {0}'.format(e))

    _LOGGER.debug('----\ntestNameChangedCallback')
    _LOGGER.debug('newName: {0}'.format(nodeName))
    _LOGGER.debug('oldName: {0}'.format(oldName))
    _LOGGER.debug('nodeType: {0}'.format(nodeType))

    return depNode
# -------------------------------------------------------------------------


#==========================================================================
# Run as LICENSE
#==========================================================================
if __name__ == '__main__':

    name_changed_callback = om.MNodeMessage.addNameChangedCallback
    ncbh = NodeMessageCallbackHandler(name_changed_callback,
                                      testNameChanged)

