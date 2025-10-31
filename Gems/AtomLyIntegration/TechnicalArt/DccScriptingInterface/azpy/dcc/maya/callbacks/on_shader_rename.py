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
# <DCCsi>\\azpy\\maya\\\callbacks\\on_shader_rename.py
# Maya node message callback handler
# -------------------------------------------------------------------------

"""
.. module:: on_shader_rename
    :synopsis: when a node name change fires off a callback, if that callback
    is registered to this function the node will be passed in.  If the node
    is a shader node, we will find that shaders shadingGroup and give it a
    similar name.

.. moduleauthor:: Amazon Lumberyard

.. :note: nothing mundane to declare
.. :attention: callbacks should be uninstalled on exit
.. :warning: maya may crash on exit if callbacks are not uninstalled

.. Usage:
    when a rename node callback is fired, the node information is passed
    to the function on_shader_rename(*args) as a tuple

        args[0] is OpenMaya.MObject, which is the object
        args[1] is the node previous name
        args[2] None (not sure what else could get passed in here)

    We get the dependancy node
        depNode = om.MFnDependencyNode(arg[0])

    We can check the node type
        nodeType = mc.nodeType( depNode.name(), api=True )

    If it's shader type the we are looking for, we can then find it's
    shading engine (shadingGroup), we are looking for u"kPluginHardwareShader"
    which is a dx11Shader (and possibly related hardware shader types)

        if nodeType == "kPluginHardwareShader":
            sG = findShadingGroup(depNode)

    The function findShadingGroup(materialDepNode), will search the shaders
    plugs for a om.MFn.kShadingEngine, if found it will return that node

    Then we rename that node: sG.setName('{0}SG'.format(depNode.name()))

.. Version:
    0.1.0 | prototype

.. History:
    < To Do >

.. Reference:
    < To Do >
"""

# --------------------------------------------------------------------------
# -- Standard Python modules
import os
import logging as _logging

# -- External Python modules

# -- Lumberyard Extension Modules
from DccScriptingInterface.globals import *

# -- Maya Modules
import maya.api.OpenMaya as om
import maya.cmds as mc
# -------------------------------------------------------------------------


#--------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy.dcc.maya.callbacks import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.on_shader_rename'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Invoking:: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# =========================================================================
# Public Functions
# =========================================================================
# --First Function---------------------------------------------------------
def find_shading_group(materialDepNode):
    """ To Do: Document"""
    # before moving on, let's see if we can figure out if this is a material
    # since we KNOW currently we are working with a specific type (dx11)
    # we can already know what we are looking for:  u"kPluginHardwareShader"
    node_type = mc.nodeType(materialDepNode.name(), api=True)  # ==> "kPluginObjectSet"

    # if it's the right type, let;s move on
    if node_type == "kPluginHardwareShader":

        #plugs = om.MPlugArray()
        #otherside = om.MPlugArray()
        #the_shading_grp = om.MFnDependencyNode()

        # gather the nodes connections
        plugs = materialDepNode.getConnections()

        the_shading_grp = None
        # loop through connections to look for shadingGroup
        for j in range(0, len(plugs)):
            if plugs[j].isConnected:
                otherside = plugs[j].connectedTo(False, True)
                for i in range(0, len(otherside)):
                    if otherside[i].node().hasFn(om.MFn.kShadingEngine):
                        the_shading_grp = om.MFnDependencyNode(otherside[i].node())

        # if we want this guys name, it's: theShadingGroup.name()
        return the_shading_grp
    else:
        return None
# -------------------------------------------------------------------------


# --Second Function--------------------------------------------------------
def on_shader_rename_rename_shading_group(*args):
    """
    When NameChangedCallback fires,
    If the node being renamed is a dx11Shader (kPluginHardwareShader),
    Find the shagingGroup and rename it to match
    """
    # get node
    try:
        mNode = args[0]  # matched maya camelCase
    except Exception as e:
        mNode = None

    # convert the MObject to a dep mNode
    if mNode:
        depNode = om.MFnDependencyNode(mNode)
    else:
        depNode = None

    # get node name
    if depNode:
        nodeName = depNode.name()
    else:
        nodeName = None

    # this seems to return nothing in this situation
    # https://tinyurl.com/y2uf66sh
    # I would expect this to return: "shader/surface"
    if nodeName:
        classifications = mc.getClassification(nodeName)
        # To Do: figure this out ^

    # before moving on, let's see if we can figure out if this is a material
    # since we KNOW currently we are working with a specific type (dx11)
    # we can already know what we are looking for:  u"kPluginHardwareShader"
    if nodeName:
        try:
            nodeType = mc.nodeType(nodeName, api=True)  # ==> "kPluginObjectSet"
        except:
            nodeType = None

    # storage container
    sG = None

    # if it's the right type, let;s move on
    if nodeType == "kPluginHardwareShader":

        # get old name
        try:
            oldName = args[1]
        except Exception as e:
            oldName = None
            _LOGGER.warning('no oldName: {0}'.format(e))

        if oldName == (u""):
            oldName = 'null'

        # get the shadingGroup
        sG = find_shading_group(depNode)

        # now rename that node, to match <nodeName>SG
        if sG:
            try:
                sG.setName('{0}SG'.format(nodeName))
            except Exception as e:
                sG = None
                _LOGGER.warning('could not renameNode: {0}'.format(e))
    else:
        return None
# -------------------------------------------------------------------------


#==========================================================================
# Module Tests
#==========================================================================
if __name__ == '__main__':

    name_change_cb = om.MNodeMessage.addNameChangedCallback
    from .node_message_callback_handler import NodeMessageCallbackHandler
    ncbh = NodeMessageCallbackHandler(name_change_cb,
                                      on_shader_rename_rename_shading_group)
