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

# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
# <DCCsi>\\azpy\\maya\\\toolbits\\detach.py
# Maya event callback handler
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
'''
Module Documentation:
    DccScriptingInterface\\azpy\\maya\\\toolbits\\detach.py

    Implements a clean detach in maya
'''
# -------------------------------------------------------------------------
# built in's
# none

# 3rd Party
# none

# Lumberyard extensions
import azpy
import azpy.helpers.decorators.wrapper
#from azpy.helpers.decorators.wrapper import wrapper


# Maya Imports
import maya.mc as mc

# -------------------------------------------------------------------------
#  global space debug flag
from azpy import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.dcc.maya.toolbits.detatch'

import azpy
_LOGGER = azpy.initialize_logger(_PACKAGENAME)
_LOGGER.debug('Invoking __init__.py for {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
#@wrapper
def clean_detach(detachType=0, args=None, name=None,
                deletHistoyIn=False, deleteHistoryOut=True):
    '''
    Helper Function to aid in detaching faces from any obj
    or duplicating those faces without harming the orignal
    '''
    
    sel = azpy.dcc.maya.helpers.utils.Selection()
    
    for obj in sel.selection.keys():
        print("~ cleanDetach:: Working on: {0}".format(obj))
        
        # set up / open the maya undo context
        with azpy.dcc.maya.helpers.UndoContext():
            
            if deletHistoyIn:
                mc.delete( obj, constructionHistory = True)
            
            obShortName = mc.ls( obj, shortNames = True)[0]
            fubName = '{0}_detWrk0'.format(obShortName)
            
            #newObj = mc.duplicate( obj, renameChildren = True, name = fubName)[0]
            newObj = mc.duplicate( obj, name = fubName)[0]
            
            mc.makeIdentity( newObj, apply = True, translate = True,
                               rotate = True, scale = True)
            
            mc.delete( newObj, constructionHistory = True)
            
            newObj = mc.parent(newObj, obj)
            
            newObj = mc.ls( newObj, long = True)[0]

            if sel.selection[obj][2] == None:
                continue
            
            # Continue detachin
            faceList = []
            for faceNum in sel.get_inverse_component_index(obj,2):
                faceList.append( '{0}.f[{1}]'.format( newObj, str(faceNum) ) )
            mc.delete(faceList)

            if detachType == 0 :
                mc.delete( sel.selection[obj][2] )

            if name:
                newObj = mc.rename( newObj, name )
            
            #mc.delete(obj, constructionHistory = True)
            
            if deleteHistoryOut:
                mc.delete(newObj, constructionHistory = True)
            
    mc.select( clear = True )
    mc.select( newObj, toggle = True )
    
    obj = mc.ls( obj, long = True)[0]
    newObj = mc.ls( newObj, long = True)[0]
    
    return obj, newObj
# -------------------------------------------------------------------------
