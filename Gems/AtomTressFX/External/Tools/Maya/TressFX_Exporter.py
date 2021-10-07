#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#-------------------------------------------------------------------------------------
#
# Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
#-------------------------------------------------------------------------------------
#
# 1. Copy tressFX_Exporter.py into Maya plug-in path. The easy place would be C:\Users\YOUR_USER_NAME\Documents\maya\plug-ins\
# 2. If you want to install TressFX menu, go to Windows->Plug-in Managers menu and check Loaded and Auto load for TressFX_Exporter.py. 
# 3. Then TressFX menu would appear in the top menu bar. 
# 4. Within it, there should be export menu item. Click it and it will bring up TressFX Exporter window. 
# 5. Alternatively, without loading TressFX plugin in Plug-in Managers, you can simply run the following python script commands in Script Editor
#    and bring up TressFX Exporter window. 
#
#  import TressFX_Exporter
#  reload(TressFX_Exporter)
#  TressFX_Exporter.UI()
#
# Or, it's easy to run this method for loading from a script, from python (python tab of script editor)
#
# Following is a script to unload and load this plugin. It may be useful to reload the plugin quickly during the development. 
# import maya.cmds as cmds
# cmds.unloadPlugin('TressFX_Exporter.py')
# cmds.loadPlugin('TressFX_Exporter.py')

from functools import partial
import maya.cmds as cmds
import maya.OpenMaya as OpenMaya
import maya.OpenMayaAnim as OpenMayaAnim
import maya.OpenMayaMPx as OpenMayaMPx
import maya.mel as mel
import pymel.core as pymel
import ctypes
import random
import sys
import re
from maya.OpenMaya import MIntArray, MDagPathArray


selected_mesh_shape_name = ''
joint_sel_list_names = [] 
singleItemGroup_Select = ''	
customRootGroup_Select = ''
jointSubsetGroup_Select = ''
hairTab_Select_Joints = ''
hairTab_Custom_Root = ''
defaultJointRootIndex = 0
defaultJointRootWeight = 0.0
customUVRange_Select = ''
customUVRange_Values = ''

tressfx_exporter_version = '4.1.23' 

# Don't change the following maximum joints per vertex value. It must match the one in TressFX loader and simulation
TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  = 4


import TressFX_Exporter

bReload = True
if(bReload): 
    reload(TressFX_Exporter)

def InstallShelf():
    #----------------------
    # Add a shelf 
    #----------------------
    shelfName = "TressFX"

    layout = mel.eval('$tmp=$gShelfTopLevel')

    if cmds.shelfLayout(shelfName, query=True, exists=True): # If the shelf exists, delete buttons and delete shelf
        for buttons in cmds.shelfLayout(shelfName, query=True, childArray=True) or []:
            cmds.deleteUI(buttons) 

        cmds.setParent(layout + '|' + shelfName)
    else: # If the shelf doesn't exist, create a new one
        cmds.setParent(layout)
        cmds.shelfLayout(shelfName) 

    # create a button
    cmds.shelfButton( label='Export',
                      command='import TressFX_Exporter\nTressFX_Exporter.UI()',
                    #command='TressFX_Exporter.UI()',
                    sourceType='python',
                    annotation='Launch TressFX exporter',
                    #image='', # This empty icon image will cause a warning message like 'Pixmap file  not found, using default.'.  
                    style='textOnly')


def initializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject, "TressFX", tressfx_exporter_version, "Any")

    # install menu
    gMainWindow = mel.eval('$temp1=$gMainWindow')

    if cmds.menu('TressFX', exists=True):
        cmds.deleteUI('TressFX')

    tressfx_top_menu = cmds.menu('TressFX', parent=gMainWindow, tearOff=False, label='TressFX')
    cmds.menuItem(parent = tressfx_top_menu, 
                  label='Export Hair/Fur', 
                  command = 'import TressFX_Exporter\nTressFX_Exporter.UI()')
    cmds.menuItem(parent = tressfx_top_menu, 
                  label='Export Collision Mesh', 
                  command = 'import TressFX_Exporter\nTressFX_Exporter.CollisionUI()')
    # install shelf
    #InstallShelf() 

def uninitializePlugin(mobject): 
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    try:
        # Close TressFX plugin windows if it is already open. 
        if cmds.window("TressFXExporterUI", exists = True):
            cmds.deleteUI("TressFXExporterUI") 

        if cmds.window("TressFXCollisionMeshUI", exists = True):
            cmds.deleteUI("TressFXCollisionMeshUI") 

        # uninstall menu
        if cmds.menu('TressFX', exists=True):
            cmds.deleteUI('TressFX') 

    except:
        sys.stderr.write("Failed to uninitialize TressFX plugin")
        raise

    return

# Helper class to show progress bar for lengthy process
class ProgressBar:
    def __init__ (self,title,steps):
        self.window = pymel.window(title, sizeable=False)
        pymel.columnLayout()

        self.progressControls = []
        self.progressbar = pymel.progressBar(maxValue=steps, width=300)
        pymel.showWindow( self.window )
        self.progressbar.step(0)

    def Kill(self):
        pymel.deleteUI(self.window)

    def Increment(self):
        self.progressbar.step(1)

def GetAllDAGObjects(mFnType, maxDepth = 1):
    #dagIterator = OpenMaya.MItDag( OpenMaya.MItDag.kBreadthFirst, OpenMaya.MFn.kJoint ) #could use kBreadthFirst to find top of each 'type' (curve, bones, xgen)
    dagIterator = OpenMaya.MItDag( OpenMaya.MItDag.kBreadthFirst, mFnType ) #could use kBreadthFirst to find top of each 'type' (curve, bones, xgen)

    #todo: Notes for making it easier to use Xgen directly
    # kJoint
    #kNurbsCurve (parent is kTransform)
    #kTransform for mesh (like sphere, and shpae is kPluginShape but not helpful since xgGuides are kTransform and kPluginShape too!)
    #and xgen collection is kPluginTransformNode (depth1), description is kTransform again
    #and the other sub items are both kTransform
    # Will need to find better way to identify xgen, mesh/skinclusters, for joints
    #perhaps joints have connection??

    # This reference to the MFnDagNode function set will be needed
    # to obtain information about the DAG objects.
    dagNodeFn = OpenMaya.MFnDagNode()
        
    listOfDagNodes = []
    # Traverse the scene.
    while( not dagIterator.isDone() ):

        # Obtain the current item.
        dagObject = dagIterator.currentItem()

        # Extract the depth of the DAG object.
        depth = dagIterator.depth()

        if depth > maxDepth:
            return listOfDagNodes
        else:
            listOfDagNodes.append(dagObject)

        # Make our MFnDagNode function set operate on the current DAG object.
        dagNodeFn.setObject( dagObject )
        
        # Extract the DAG object's name.
        name = dagNodeFn.name()
                
        #print name + ' (' + dagObject.apiTypeStr() + ') depth: ' + str( depth )
        
        # Iterate to the next item.
        dagIterator.next()
    
    return listOfDagNodes

def GetNurbCurvesByGroup():
    return GetAllDAGObjects(OpenMaya.MFn.kNurbsCurve, 4)

def GetAvailableRigs():
    return GetAllDAGObjects(OpenMaya.MFn.kJoint, 1)

def GetSkinClustersByRig():
    return 

#currently undefined. Maya doesn't seem to have a python api for getting to the 
#Utilities (tab) of Xgen, or rendering. both are needed to render out guides, 
# then convert guides to nurbs curves.
#def GetXGenGroups(): 
#	return

def GetSkinClusterInfluenceObjectsNames():
    #-------------------------
    # Get skin cluster object 
    #------------------------- 
    if selected_mesh_shape_name == '':
        cmds.warning("TressFX: Cannot retrieve skin cluster, base mesh must be set.\n")
        return    
    
    skinClusterName = ''
    skinClusters = cmds.listHistory(selected_mesh_shape_name)
    skinClusters = cmds.ls(skinClusters, type="skinCluster")
    if skinClusters:
        skinClusterName = skinClusters[0]
    else:
        cmds.warning('TressFX: No skin cluster found on '+ selected_mesh_shape_name)
        return

    # get the MFnSkinCluster using skinClusterName
    selList = OpenMaya.MSelectionList() 
    selList.add(skinClusterName) 
    skinClusterNode = OpenMaya.MObject() 
    selList.getDependNode(0, skinClusterNode) 
    skinFn = OpenMayaAnim.MFnSkinCluster(skinClusterNode)

    dagPaths = MDagPathArray()
    skinFn.influenceObjects(dagPaths)
    skInfluenceNames = []

        # get joint names
    for i in range(dagPaths.length()):
        influenceName = dagPaths[i].partialPathName()  
        skInfluenceNames.append(influenceName) #we want the partial path, even if it includes NS here (for comparision with user selected list)

    return skInfluenceNames


def UI():
    # prevents multiple windows
    if cmds.window("TressFXExporterUI", exists = True):
        cmds.deleteUI("TressFXExporterUI")
    global joint_sel_list_names
    joint_sel_list_names = [] #clear joint list, can't guarantee clear() method will be present (need python 3.3 minimum)

    window_width = 550
    window_height = 525

    windowTitle = 'TressFX Hair/Fur'
    window = cmds.window("TressFXExporterUI", title = windowTitle, w=window_width, h = window_height, mnb=False, sizeable=False)

    mainLayout = cmds.columnLayout(columnAlign = 'center') # In case you want to see the area of the main layout, use backgroundColor = (1, 1, 1) as an argument
    #cmds.separator(h=10)
    mainTabs = cmds.tabLayout(imw = 5, imh = 5)

    #cmds.rowColumnLayout( numberOfColumns=2, columnWidth=[ (1,10),(2,window_width-10) ])
    #cmds.separator(style='none', width=10)
    
    hairTab_SelectTarget = cmds.rowColumnLayout( numberOfColumns=3, columnWidth=[ (1,300),(2,10),(3,170) ], columnAlign = [ (1,'center'),(2,'center'),(3,'center') ], parent = mainTabs )
    hairTab_SetOptions = cmds.rowColumnLayout( numberOfColumns=3, columnWidth=[ (1,300),(2,10),(3,170) ], columnAlign = [ (1,'center'),(2,'center'),(3,'center') ], parent = mainTabs )
    hairTab_Export = cmds.rowColumnLayout( numberOfColumns=3, columnWidth=[ (1,300),(2,10),(3,170) ], columnAlign = [ (1,'center'),(2,'center'),(3,'center') ], parent = mainTabs )
    cmds.tabLayout( mainTabs, edit=True, tabLabel=((hairTab_SelectTarget, 'Select Hair/Mesh/Rig'), (hairTab_SetOptions, 'Choose Options'), (hairTab_Export, 'Export Files')) )
    
#Selection tab items - selecting the rig, xgen, curves, skincluster/meshes
    cmds.setParent(hairTab_SelectTarget)
    global singleItemGroup_Select
    singleItemGroup_Select = cmds.columnLayout(columnAlign = 'center', parent = hairTab_SelectTarget)
    
    cmds.separator(style='none',h=50)
    cmds.button(label="Set the base mesh", w=200, h=25, command=SetBaseMesh)
    cmds.separator(style='none',h=20)
    cmds.textField("MeshNameLabel", w=200, editable=False)

    cmds.separator(style='none',h=10)
    global customRootGroup_Select
    customRootGroup_Select = cmds.columnLayout(columnAlign = 'center', parent = singleItemGroup_Select)
    cmds.checkBox("UseCustomJointRootLabel", label='Use Custom Joint Root', align='left', enable=False, changeCommand=UseCustomJointRoot, parent = customRootGroup_Select )
    
    cmds.separator(style='none',h=10) #renormalize so weights add up to one
    cmds.checkBox("RenormalizeFinalPairs", label='Re-Normalize Final Weights (sumMaxInfluences=1)', align='left', enable=False)
    
    cmds.separator(style='none',h=10)
    global jointSubsetGroup_Select
    jointSubsetGroup_Select = cmds.columnLayout(columnAlign = 'center', parent = singleItemGroup_Select)
    cmds.checkBox("UseJointSubsetLabel", label='Use Joints Subset Only', align='left', enable=False, changeCommand=UseJointSubset, parent = jointSubsetGroup_Select )
    
    cmds.separator(style='none',h=10)


#Options tab items - vertices per strand, sampling, random, unreal/directx/special formats
    #cmds.intField("numVerticesPerStrand", w=30, minValue=4, maxValue=64, value=16 )
    cmds.setParent(hairTab_SetOptions)
    
    cmds.separator(style='none',h=50)
    cmds.separator(style='none',h=50)
    cmds.separator(style='none',h=50)
    
    cmds.text(label='Number of vertices per strand:', align='left', parent = hairTab_SetOptions)    
    cmds.separator(style='none', width=10)

    cmds.optionMenu("numberOfStrandsOptionMenu", label='')
    cmds.menuItem(label='4')
    cmds.menuItem(label='8')
    cmds.menuItem(label='16')
    cmds.menuItem(label='32')
    cmds.menuItem(label='64')

    cmds.text(label='Minimum curve length:', align='left')
    cmds.separator(style='none',h=30)
    cmds.floatField("minCurveLength", minValue=0, value=0, w=70)

    cmds.text(label='Sample every N curves:', align='left')    
    cmds.separator(style='none', width=10)
    cmds.optionMenu("samplingOptionMenu", label='')
    cmds.menuItem(label='1')
    cmds.menuItem(label='2')
    cmds.menuItem(label='4')
    cmds.menuItem(label='8')
    cmds.menuItem(label='16')
    cmds.menuItem(label='32')
    cmds.menuItem(label='64')
    cmds.menuItem(label='128')

    cmds.text(label='Sample start offset[0-32]:', align='left')
    cmds.separator(style='none',h=30)
    cmds.intField("curveOffset", w=70, minValue=0, maxValue=32, value=0 )

    cmds.text(label='Scale Scene:', align='left')
    cmds.separator(style='none', width=10)
    cmds.optionMenu('scalingOptionMenu', label='') #TressFX does not know about the auto-scaling an engine might do (such as FBX cm->m) but you can manually set scaling here to adjust for what the engine will want
    cmds.menuItem(label='0.001')
    cmds.menuItem(label='0.01')
    cmds.menuItem(label='0.1')
    cmds.menuItem(label='1.0')
    cmds.menuItem(label='10.0')
    cmds.menuItem(label='100.0')
    cmds.menuItem(label='1000.0')
    cmds.optionMenu('scalingOptionMenu', edit=True, value='1.0') 
    
    singleItemGroup_Options = cmds.columnLayout(columnAlign = 'center', parent = hairTab_SetOptions)

    cmds.separator(style='none',h=50)
    cmds.checkBox("bothEndsImmovable", label='Both ends immovable')
    cmds.separator (style='none', width=10)
    cmds.checkBox("InvertZ", label='Invert Z-axis of Hairs', value = False)
    cmds.checkBox("randomStrandCheckBox", label='Randomize strands for LOD', value = False)
    

    cmds.separator(style='none',h=30)
    cmds.text(label='Unreal 4.x Options', align='center')
    cmds.checkBox("useZUp", label='Make Z-Up Direction', value = True) #if user requests z-up, and maya not using Z (but Y as up), will swap y and z coordinates of point

    cmds.separator(style='none',h=30)
    cmds.text(label='DirectX 11/12 Options', align='center')
    cmds.checkBox("InvertYForUVs", label='Invert Y-axis of UV coordinates', value = True, changeCommand=AllowCustomUV )
    global customUVRange_Select
    customUVRange_Select = cmds.columnLayout(columnAlign = 'center', parent = singleItemGroup_Options)
    cmds.checkBox("useNonUniformUVRange", label='Using Non-Uniform UV Range', value = False, align='left', enable=True, changeCommand=UseCustomUV, parent = customUVRange_Select )
    cmds.separator(style='none',h=10)


#Export Specific Items
    cmds.setParent(hairTab_Export)
    singleItemGroup_Export = cmds.columnLayout(columnAlign = 'center', parent = hairTab_Export)

    cmds.separator(style='none',h=50)
    cmds.text(label='Export (TressFX 4.x)', align='center')
    cmds.checkBox("exportHairCheckBox", label='Export hair data (*.tfx)', value = True)
    cmds.checkBox("exportBoneCheckBox", label='Export bone data (*.tfxbone)', value = True)

    cmds.separator(style='none',h=30)

    cmds.text(label='ErrorMode (TressFX 4.x)', align='center')
    cmds.checkBox("ignoreUVErrorsCheckBox", label='ignore TFX UVcoord Errors', value = True)
    cmds.checkBox("removeNamespace", label='remove Namespace from bones', value = True) #key when importing one .ma/.mb into another

    cmds.separator(style='none',h=30)

    cmds.text(label='Export (TressFX 3.0, deprecated)', align='center')
    cmds.checkBox("exportSkinCheckBox", label='Export skin data (*.tfxskin)', value = False)

    cmds.separator(style='none',h=30)
    cmds.button(label="Export!", w=300, h=50, command=DoExport)

    #cmds.separator(style='none',h=15)
    #cmds.button(label="Export collision mesh", w=170, h=30, command=DoExportCollisionMesh)

    #cmds.button(label="Goto Bind Pose", w=220, h=30, command=GotoBindPose)

    cmds.separator(h=20)

    cmds.setParent(mainLayout)
    version_text = 'v' + tressfx_exporter_version
    cmds.text(label=version_text, width=window_width-25, align='right')
    
    cmds.separator(style='none',h=20)


    global selected_mesh_shape_name
    selected_mesh_shape_name = ''

    cmds.showWindow(window)

    # After showWindow, resize the window to enforce its size. This is a workaround of Maya's bug. 
    cmds.window(window, edit=True, widthHeight=(window_width, window_height))

def CollisionUI():
    # prevents multiple windows
    if cmds.window("TressFXCollisionMeshUI", exists = True):
        cmds.deleteUI("TressFXExporterUI") 

    window_width = 320
    window_height = 300

    windowTitle = 'TressFX Collision'
    window = cmds.window("TressFXCollisionMeshUI", title = windowTitle, w=window_width, h = window_height, mnb=False, sizeable=False)

    mainLayout = cmds.columnLayout(columnAlign = 'center') # In case you want to see the area of the main layout, use backgroundColor = (1, 1, 1) as an argument
    
    cmds.rowColumnLayout( numberOfColumns=3, columnWidth=[ (1,10),(2,window_width-20), (3, 10) ], rowSpacing = (1,10), parent=mainLayout)

    cmds.separator(style='none', width=10)
    cmds.separator(style='none', width=10)
    cmds.separator(style='none', width=10)

    cmds.separator(style='none', width=10)
    cmds.button(label="Set the collision mesh", w=120, h=25, command=partial(SetBaseMesh, True))
    cmds.separator(style='none', width=10)

    cmds.separator(style='none', width=10)
    cmds.textField("MeshNameLabel", w=220, editable=False)
    cmds.separator(style='none', width=10)    
    
    cmds.separator(style='none', width=10, height=20)
    cmds.separator(style='none', width=10)
    cmds.separator(style='none', width=10)

    subwidth = (300 - 20)/4
    cmds.rowColumnLayout( numberOfColumns=4, columnWidth=[ (1,10),(2, subwidth), (3, subwidth), (4, 10) ], rowSpacing = (1,10), parent=mainLayout)
    cmds.separator(style='none', width=10)	
    cmds.text(label='Scale Scene:', align='left')
    cmds.optionMenu('scalingCollisionOptionMenu', label='') #TressFX does not know about the auto-scaling an engine might do (such as FBX cm->m) but you can manually set scaling here to adjust for what the engine will want
    cmds.menuItem(label='0.001')
    cmds.menuItem(label='0.01')
    cmds.menuItem(label='0.1')
    cmds.menuItem(label='1.0')
    cmds.menuItem(label='10.0')
    cmds.menuItem(label='100.0')
    cmds.menuItem(label='1000.0')
    cmds.optionMenu('scalingCollisionOptionMenu', edit=True, value='1.0') 
    cmds.separator(style='none', width=10)

    #cmds.setParent('..')
    #cmds.rowColumnLayout( numberOfColumns=1, columnWidth=[ (1,window_width-20) ], rowSpacing = (1,10), parent=mainLayout)
    cmds.rowColumnLayout( numberOfColumns=3, columnWidth=[ (1,10),(2,window_width-20), (3, 10) ], rowSpacing = (1,10), parent=mainLayout)

    cmds.separator(style='none', width=10, height=20)
    cmds.separator(style='none', width=10)
    cmds.separator(style='none', width=10)

    cmds.separator(style='none', width=10)
    cmds.text(label='ErrorMode (TressFX 4.x)', align='center')
    cmds.separator(style='none', width=10)
    cmds.separator(style='none', width=10)
    cmds.checkBox("removeNamespaceCM", label='remove Namespace from bones', value = True) 
    cmds.separator(style='none', width=10)
    cmds.separator(style='none', width=10)
    cmds.checkBox("RenormalizeFinalPairs", label='Re-Normalize Final Weights (sumMaxInfluences=1)', align='left', enable=True)
    cmds.separator(style='none', width=10)
    #cmds.checkBox("noDupBoneWts", label='test:no dup nonzero bone wts per vtx', value = False) #error checking. todo: may want this, or may not need it
    
    cmds.separator(style='none', width=10)
    cmds.button(label="Export", w=170, h=30, command=DoExportCollisionMesh)
    cmds.separator(style='none', width=10)

    cmds.separator(style='none', width=10)
    cmds.separator(style='none', width=10)
       
    version_text = 'v' + tressfx_exporter_version
    cmds.text(label=version_text, width=window_width-25, align='right', parent=mainLayout)

    global selected_mesh_shape_name
    selected_mesh_shape_name = ''

    cmds.showWindow(window)

    # After showWindow, resize the window to enforce its size. This is a workaround of a Maya bug. 
    cmds.window(window, edit=True, widthHeight=(window_width, window_height))

def AllowCustomUV(*args):
    bEnableCustomUV = cmds.checkBox("InvertYForUVs", q = True, v = True)
    if (bEnableCustomUV):
        cmds.checkBox("useNonUniformUVRange", edit = True, enable = True)
    else:
        cmds.checkBox("useNonUniformUVRange", edit = True, enable = False)
    return

def UseCustomUV(*args):
    #if using DirectX invert-y option, means we are inverting the v of uv coordinates, and allow the user the option of specifying
    #a custom uv range other than 0-1. This means that they have an actual non-uniform range. We do not check to see if that is true or not.
    #mainly because I couldn't find a way for Maya to tell me that info
    bUseCustomUV = cmds.checkBox("useNonUniformUVRange", q = True, v = True)

    global customUVRange_Values
    if (bUseCustomUV):
        if cmds.rowColumnLayout(customUVRange_Values, exists = True):
            cmds.deleteUI(customUVRange_Values, layout=True)

        customUVRange_Values = cmds.rowColumnLayout( numberOfColumns=4, columnWidth=[ (1,50),(2,80),(3,50),(4,80) ], columnAlign = [ (1,'center'),(2,'center'),(3,'center'),(4,'center') ], parent = customUVRange_Select )
        cmds.setParent(customUVRange_Values)
        cmds.text( "U min:", w=20)
        cmds.floatField( "uMin", w=50, edit=False, pre=2, value = 0.0)
        cmds.text( "U max:", w=20)
        cmds.floatField( "uMax", w=50, edit=False, pre=2, value = 1.0)
        
        cmds.text( "V min:", w=20)
        cmds.floatField( "vMin", w=50, edit=False, pre=2, value = 0.0)
        cmds.text( "V max:", w=20)
        cmds.floatField( "vMax", w=50, edit=False, pre=2, value = 1.0)
    else:
        if cmds.rowColumnLayout(customUVRange_Values, exists = True):
            cmds.deleteUI(customUVRange_Values, layout=True)

    return

def CreateCustomRootUI(jointNameRoot):
    global defaultJointRootIndex
    global defaultJointRootWeight
    global hairTab_Custom_Root

    hairTab_Custom_Root = cmds.columnLayout(columnAlign = 'center', parent = customRootGroup_Select )
    hairTab_Custom_Root1 = cmds.rowColumnLayout( numberOfColumns=2, columnWidth=[ (1,100),(2,100) ], columnAlign = [ (1,'center'),(2,'center') ], parent = hairTab_Custom_Root )
    cmds.setParent(hairTab_Custom_Root1)
    cmds.button("Set Joint", w=50, h=25, enable=True, visible=True, command=SetRootJoint)
    cmds.button("Clear Joint", w=50, h=25, enable=True, visible=True, command=ClearRootJoint)
        
    hairTab_Custom_Root2 = cmds.rowColumnLayout( numberOfColumns=2, columnWidth=[ (1,100),(2,100) ], columnAlign = [ (1,'center'),(2,'center') ], parent = hairTab_Custom_Root )
    cmds.setParent(hairTab_Custom_Root2)
    cmds.text( "New Root:", w=50)
    cmds.text( "JointRootName", w=50, label=jointNameRoot)
        
    hairTab_Custom_Root3 = cmds.rowColumnLayout( numberOfColumns=2, columnWidth=[ (1,100),(2,100) ], columnAlign = [ (1,'center'),(2,'center') ], parent = hairTab_Custom_Root )
    cmds.setParent(hairTab_Custom_Root2)
    cmds.text( "Weight (0-1)", w=50)
    cmds.floatField( "JointRootWeightFloat", w=50, edit=False, minValue = 0.0, maxValue = 1.0, pre=2, value = defaultJointRootWeight, changeCommand=SetRootWeight)
    return

def DeleteCustomRootUI():
    global hairTab_Custom_Root
    #destroy the panel/float input with the root joint selection and weight (includes all children)
    if cmds.columnLayout(hairTab_Custom_Root, exists = True):
        cmds.deleteUI(hairTab_Custom_Root, layout=True)
    return

def UseCustomJointRoot(*args):
    #todo: automatically add the custom root to the joint subset (only if a joint subset is being used)
    #create UI for setting a custom root (joint) and its weight (only used when hit a 0-weight case)
    bUseCustomRoot = cmds.checkBox("UseCustomJointRootLabel", q = True, v = True)
    global defaultJointRootIndex
    global defaultJointRootWeight

    if (bUseCustomRoot):
        rootName = GetJointNameFromListByIndex(0)
        CreateCustomRootUI(rootName)
    else:
        DeleteCustomRootUI()
        defaultJointRootIndex = 0
        defaultJointRootWeight = 0.0
    return

def GetJointNameFromListByIndex(index):
    skinClusterJointList = GetSkinClusterInfluenceObjectsNames()
    #check to make sure we have joints (influencers) bound to mesh (skin)
    if len(skinClusterJointList) == 0:
        cmds.warning("TressFX: Could not retrieve joint name by index. Base mesh has no skin cluster influencers (check to see if skin is bound)")
        return "none"
    if (index < 0) or (index >= len(skinClusterJointList)):
        cmds.warning("TressFX: Could not retrieve joint name by index. Index out of bounds.")
        return "none"
    return skinClusterJointList[index]


def GetJointIndexFromList(jointName):
    skinClusterJointList = GetSkinClusterInfluenceObjectsNames()
    jointIndex = 0
    #check to make sure we have joints (influencers) bound to mesh (skin)
    if len(skinClusterJointList) == 0:
        cmds.warning("TressFX: Could not retrieve joint index (ret joint index=0). Base mesh has no skin cluster influencers (check to see if skin is bound)")
    
    for i in range(len(skinClusterJointList)):
        if (jointName == skinClusterJointList[i]):
            jointIndex = i
    return jointIndex

def SetRootWeight(value):
    global defaultJointRootWeight
    defaultJointRootWeight = value #the value is already constrained to be between 0 and 1, when the user hits <return> or eventually hits another button, etc to force a return, we get notified.
    return

def SetRootJoint(*args):
    #------------------------------
    # Set joint to be used as the default 'root joint' used when there are 0 weights during influence (for hair strands) calculations
    #-------------------------------
    if selected_mesh_shape_name == '':
        cmds.warning("TressFX: To select custom root joint, base mesh must be set.\n")
        return

    sel_list = OpenMaya.MSelectionList() 
    OpenMaya.MGlobal.getActiveSelectionList(sel_list) 

    if sel_list.length() == 0:
        return
    if sel_list.length() > 1:
        cmds.warning("TressFX: Can only select one joint to be *root*")
        return

    skinClusterJointList = GetSkinClusterInfluenceObjectsNames()
    #check to make sure we have joints (influencers) bound to mesh (skin)
    if len(skinClusterJointList) == 0:
        cmds.warning("TressFX: Base mesh has no skin cluster influencers (check to see if skin is bound)")
        return

    dagPath = OpenMaya.MDagPath()
    sel_list.getDagPath(0, dagPath) #index 0, first item in the selection list
    jointFn = OpenMaya.MFnDagNode(dagPath)
    joint_name = jointFn.name()

    if dagPath.apiType() != OpenMaya.MFn.kJoint:  #must be a joint, nothing else
        cmds.warning('TressFX: Not a joint, root must be a joint: objectname = ' + joint_name) 
        return
    elif not (joint_name in skinClusterJointList):
        cmds.warning('TressFX: Joint not in influencer list for skin cluster/base mesh: joint = ' + joint_name)
        return
    else:
        global defaultJointRootIndex
        global defaultJointRootWeight
        DeleteCustomRootUI()
        defaultJointRootIndex = 0
        defaultJointRootWeight = 0.0

        defaultJointRootIndex = GetJointIndexFromList(joint_name)
        CreateCustomRootUI(joint_name)
        cmds.showWindow("TressFXExporterUI")
    return

def ClearRootJoint(*args):
    global defaultJointRootIndex
    global defaultJointRootWeight

    DeleteCustomRootUI()
    defaultJointRootIndex = 0
    defaultJointRootWeight = 0.0
    
    defaultName = GetJointNameFromListByIndex(defaultJointRootIndex)
    CreateCustomRootUI(defaultName)
    cmds.showWindow("TressFXExporterUI")
    return

def CreateJointSubsetUI(): 
    #adds ability to only look at (allow) a subset of the joints for the hair influencers 
    # (good when you have a lot of joints that are 0, like facial weights, to prevent situation with a zero weight/hair that will not track)
    global hairTab_Select_Joints
    
    hairTab_Select_Joints = cmds.columnLayout(columnAlign = 'center', parent = jointSubsetGroup_Select )
    hairTab_Select_Joints1 = cmds.rowColumnLayout( numberOfColumns=3, columnWidth=[ (1,100),(2,100),(3,100) ], columnAlign = [ (1,'center'),(2,'center'),(3,'center') ], parent = hairTab_Select_Joints )
    cmds.setParent(hairTab_Select_Joints1)
    cmds.button("Add joints", w=50, h=25, enable=True, visible=True, command=AddJointSet)
    cmds.button("Delete joints", w=50, h=25, enable=True, visible=True, command=DeleteJointSet)
    cmds.button("Clear All joints", w=50, h=25, enable=True, visible=True, command=ClearJointSet)

    pane_list = cmds.paneLayout("JointNamePane", parent=hairTab_Select_Joints)
    cmds.textScrollList( "JointNameScrollList", w=300, numberOfRows=8, e=False, allowMultiSelection=True, parent = pane_list, append=joint_sel_list_names)
    
    return

def DeleteJointSubsetUI():
    #delete the UI, but not responsible for clearing the joints selected to joint_sel_list_names
    global hairTab_Select_Joints
    #destroy the panel/float input with the root subset selection controls (includes all children)
    if cmds.columnLayout(hairTab_Select_Joints, exists = True):
        cmds.deleteUI(hairTab_Select_Joints, layout=True)
    return

def UseJointSubset(*args):
    bUseJointSubset = cmds.checkBox("UseJointSubsetLabel", q = True, v = True) 
    if selected_mesh_shape_name == '':
        cmds.warning("TressFX: To select custom joint subset, base mesh must be set.\n")
        bUseJointSubset = False
        cmds.checkBox("UseJointSubsetLabel", editable=True, v=False)
        return

    if (bUseJointSubset):
        CreateJointSubsetUI()
        cmds.warning("TressFX: Using a subset can result in loss of hair tracking accuracy.\n Use with caution and only on bones that should never have affect on that skin subsection. \n Useful for masking out dead weights and non-skin joints like weaponry.")	
    else:
        #destroy the panels with the joints/controls and clear the joint set list
        DeleteJointSubsetUI()
    return

def ClearJointSet(*args):
    global joint_sel_list_names
    joint_sel_list_names = []
    DeleteJointSubsetUI()
    CreateJointSubsetUI()
    return

def DeleteJointSet(*args):
    global joint_sel_list_names
    
    if cmds.columnLayout(hairTab_Select_Joints, exists = True):
        numSelectedItems = cmds.textScrollList( "JointNameScrollList", query=True, nsi=True)
        if numSelectedItems > 0:
            selectedItems = cmds.textScrollList( "JointNameScrollList", query=True, si=True)
        else:
            print('TressFX: Nothing deleted: no joints selected from joint subset list')
            return
        for item in selectedItems:
            if item in joint_sel_list_names:
                joint_sel_list_names.remove(item)
    DeleteJointSubsetUI()
    CreateJointSubsetUI()

    return


def AddJointSet(*args):
    #------------------------------
    # Create a list of joints to be used in the mapping (and exclude those not in this list)
    #-------------------------------
    if selected_mesh_shape_name == '':
        cmds.warning("TressFX: To select joint subset, base mesh must be set.\n")
        return

    sel_list = OpenMaya.MSelectionList() 
    OpenMaya.MGlobal.getActiveSelectionList(sel_list) 

    if sel_list.length() == 0:
        return

    skinClusterJointList = GetSkinClusterInfluenceObjectsNames()
    #check to make sure we have joints (influencers) bound to mesh (skin)
    if len(skinClusterJointList) == 0:
        cmds.warning("TressFX: Base mesh has no skin cluster influencers (check to see if skin is bound)")
        return

    # Create iterator through list of selected object
    selection_iter = OpenMaya.MItSelectionList(sel_list)
    obj = OpenMaya.MObject()

    # Loop though iterator objects
    global joint_sel_list_names
    while not selection_iter.isDone():
        selection_iter.getDependNode(obj)
        dagPath = OpenMaya.MDagPath.getAPathTo(obj)
        #print dagPath.fullPathName()
        jointFn = OpenMaya.MFnDagNode(dagPath)
        joint_name = jointFn.name()
        if not (joint_name in skinClusterJointList):
            cmds.warning('TressFX: Joint not in influencer list for skin cluster/base mesh: joint = ' + joint_name)
        elif joint_name in joint_sel_list_names:
            cmds.warning('TressFX: Joint already in list: joint = ' + joint_name) #skip it but continue iterating
        elif dagPath.apiType() != OpenMaya.MFn.kJoint:  #must be a joint, nothing else
            cmds.warning('TressFX: Not a joint, can only add joints to this list: objectname = ' + joint_name) #skip it but continue iterating
        else:
            joint_sel_list_names.append(joint_name)
        selection_iter.next()
        
    DeleteJointSubsetUI()
    CreateJointSubsetUI()
    cmds.showWindow("TressFXExporterUI")
    return


def GetIndicesSubsetInfluenceObjects(skinFn, dagPaths ):
    #get the joints, but if we are limiting to a subset of joints use that, otherwise use full joint set attached to skincluster
    # we also will check the subset and reject any bones that are not contained in dagPaths (which lists the bones for that skincluster)
    boneSubsetIndices = []
    numberItems = 0
    if len(joint_sel_list_names) != 0:
        jointList = joint_sel_list_names
        numberItems = len(joint_sel_list_names)
    else:
        cmds.warning("TressFX: Joint Subset Activated, but no items in subset. Using full joints.")
        return boneSubsetIndices
    
    for k in range(numberItems):
        influenceName = jointList[k] 
        for i in range(dagPaths.length()):
            if (influenceName == dagPaths[i].partialPathName() ): 
                boneSubsetIndices.append(skinFn.indexForInfluenceObject(dagPaths[i])) 
            
    return boneSubsetIndices

def GetInfluenceObjects(dagPaths ):
    #get all the joints (influence objects)..currently TressFX 4.x needs all the bone names and indices exported, not just a subset
    boneNames = []
    #do we need to remove the namespace if present?
    bRemoveNS = cmds.checkBox("removeNamespace", q = True, v = True)

    numberItems = 0
    jointList = dagPaths
    numberItems = jointList.length()

    # get joint names
    for i in range(numberItems):
        influenceName = jointList[i].partialPathName() 
        if bRemoveNS:
            influenceNameList = re.split("[:]", influenceName)
            if len(influenceNameList) != 1: #has a namespace prefix
                boneNames.append(influenceNameList[-1])
            else:
                boneNames.append(influenceNameList[0])
        else:      
            boneNames.append(influenceName) # Need to remove namespace?

    return boneNames

def SetBaseMesh(bCollision = False, *args): 
    #------------------------------
    # Find the selected mesh shape
    #-------------------------------
    #collision also uses this, need to differentiate (currently col should not have same options available)
    # i.e. doesn't hide/disable controls like the Hair side

    sel_list = OpenMaya.MSelectionList()
    OpenMaya.MGlobal.getActiveSelectionList(sel_list)

    if sel_list.length() == 0:
        return

    dagPath = OpenMaya.MDagPath()
    sel_list.getDagPath(0, dagPath)

    if not dagPath.hasFn(OpenMaya.MFn.kMesh): #if the node along this path doesn't support kMesh, we reject it and let the user know
        cmds.warning('TressFX: Selection not a kMesh')
        #note: in this case, we are not resetting the base mesh or any controls from current values (empty or not)
        return

    global selected_mesh_shape_name

    #is it already open and we are just changing base meshes? (Currently, export of hair only)
    if (selected_mesh_shape_name != '') and not (bCollision == True):
        #subcontrols may already be enabled, so reset them to defaults for this base mesh, or if the selected object isn't a mesh
        cmds.checkBox("RenormalizeFinalPairs", edit=True, value=False)
        cmds.checkBox("UseCustomJointRootLabel", edit=True, value =False)
        cmds.checkBox("UseJointSubsetLabel", edit=True, value=False)
        UseCustomJointRoot()
        UseJointSubset()
    
    #proceed as normal to set the new mesh and get its name
    cmds.textField("MeshNameLabel", edit=True, text='')
    selected_mesh_shape_name = ''
    dagPath.extendToShape()
    meshFn = OpenMaya.MFnMesh(dagPath)
    selected_mesh_shape_name = meshFn.name()
    cmds.textField("MeshNameLabel", edit=True, text=meshFn.name()) 

    #enable options
    if not (bCollision == True):
        cmds.checkBox("RenormalizeFinalPairs", edit=True, enable=True)
        cmds.checkBox("UseCustomJointRootLabel", edit=True, enable=True)
        cmds.checkBox("UseJointSubsetLabel", edit=True, enable=True)
    
    return

def DoExport(*args):
    # TODO: Set the time frame to the first one or go to the bind pose? 
    # first_frame = cmds.playbackOptions(minTime=True, query=True)
    # cmds.currentTime(first_frame)

    bExportHairCheckBox = cmds.checkBox("exportHairCheckBox", q = True, v = True)
    bExportSkinCheckBox = cmds.checkBox("exportSkinCheckBox", q = True, v = True)
    bExportBoneCheckBox = cmds.checkBox("exportBoneCheckBox", q = True, v = True)

    #----------------------------------------
    # collect selected nurbs spline curves.
    #----------------------------------------
    minCurveLength = cmds.floatField("minCurveLength",q = True, v = True)
    curves = GetSelectedNurbsCurves(minCurveLength)

    if len(curves) == 0:
        cmds.warning("TressFX: No nurbs curves were selected!")
        return
    else:
        print("TressFX: %d curves were selected.\n" % len(curves))

    #-------------------------------
    # Find the selected mesh shape
    #-------------------------------
    meshShapedagPath = OpenMaya.MDagPath()

    if selected_mesh_shape_name == '':
        meshShapedagPath = None
    else:
        allObject = cmds.ls(selected_mesh_shape_name)
        cmds.select(allObject) # TODO: This selection makes hair curves unselected. This is not a problem but just inconvenient for users if they want to keep the curves selected.  

        sel_list = OpenMaya.MSelectionList()
        OpenMaya.MGlobal.getActiveSelectionList(sel_list)

        if sel_list.length() == 0:
            meshShapedagPath = None
        else:
            sel_list.getDagPath(0, meshShapedagPath)
            meshShapedagPath.extendToShape() # get mesh shape

    # if none of export checkboxes were selected, then exit.     
    if bExportHairCheckBox == 0 and bExportSkinCheckBox == 0 and bExportBoneCheckBox == 0:
        cmds.warning("TressFX: Please select checkbox for exporting data")
        return 

    rootPositions = []

    #-------------------
    # Export a tfx file
    #-------------------
    if bExportHairCheckBox:    
        basicFilter = "*.tfx"
        filepath = cmds.fileDialog2(fileFilter=basicFilter, dialogStyle=2, caption="Save a tfx binary file(*.tfx)", fileMode=0)

        if filepath == None or len(filepath) == 0:
            return

        bRandomize = cmds.checkBox("randomStrandCheckBox", q = True, v = True)

        if bRandomize:
            random.shuffle(curves)

        rootPositions = SaveTFXBinaryFile(filepath[0], curves, meshShapedagPath)

    #------------------------------------------------------------------------------------
    # collect root positions for tfxskin or tfxbone in case SaveTFXBinaryFile was not run
    #------------------------------------------------------------------------------------
    if bExportHairCheckBox == 0 and (bExportSkinCheckBox == 1 or bExportBoneCheckBox == 1):
        for i in xrange(len(curves)):
            curveFn = curves[i]
            rootPos = OpenMaya.MPoint()
            curveFn.getPointAtParam(0, rootPos, OpenMaya.MSpace.kObject) # kWorld?
            rootPositions.append(rootPos)

    if bExportSkinCheckBox == 1 or bExportBoneCheckBox == 1:
        if selected_mesh_shape_name == '':
            cmds.warning("TressFX: To export skin or bone data, base mesh must be set.\n")
            return

    #--------------------    
    # Save tfxskin file
    #--------------------
    if bExportSkinCheckBox == 1:
        basicFilter_tfxskin = "*.tfxskin"
        filepath = cmds.fileDialog2(fileFilter=basicFilter_tfxskin, dialogStyle=2, caption="Save a tfxskin file(*.tfxskin)", fileMode=0)

        if filepath == None or len(filepath) == 0:
            return

        SaveTFXSkinBinaryFile(filepath[0], meshShapedagPath, rootPositions)

    #------------------------
    # Save the tfxbone file.
    #------------------------
    if bExportBoneCheckBox == 1:
        basicFilter = "*.tfxbone"
        filepath = cmds.fileDialog2(fileFilter=basicFilter, dialogStyle=2, caption="Save a tfx bone file(*.tfxbone)", fileMode=0)

        if filepath == None or len(filepath) == 0:
            return
    
    #we will check for a joint subset in the save tfxbone function	
    SaveTFXBoneBinaryFile(filepath[0], selected_mesh_shape_name, meshShapedagPath, rootPositions)

    return

# TODO: Do we need to enforce to go to the bind pose or let user do it? 
def GotoBindPose(*args):
    joints = cmds.ls(type='joint')
    rootJoints = []

    for j in joints:
        while cmds.listRelatives(j, p=True):
            parent = cmds.listRelatives(j, p=True)
            j = parent[0]
        rootJoints.append(j)

    rootJoints = set(rootJoints)

    for rootJoint in rootJoints:
        cmds.select(rootJoint)
        bindpose = cmds.dagPose(q=True, bindPose=True)
        cmds.dagPose(bindpose[0] , restore=True)    
        cmds.select(cl=True)

    return    

class WeightJointIndexPair:
    weight = 0
    joint_index = -1

    # For sorting 
    def __lt__(self, other):
        return self.weight > other.weight

# vertexIndices is three vertex indices belong to one triangle
def GetSortedWeightsFromTriangleVertices(_maxJointsPerVertex, vertexIndices, jointIndexArray, weightArray, baryCoord):
    final_pairs = []

    for j in range(_maxJointsPerVertex):
        for i in range(3):
            vertex_index = vertexIndices[i]
            bary = baryCoord[i]

            weight = weightArray[vertex_index*_maxJointsPerVertex + j] * bary
            joint_index = jointIndexArray[vertex_index*_maxJointsPerVertex + j]

            bFound = False
            for k in range(len(final_pairs)):
                if final_pairs[k].joint_index == joint_index:
                    final_pairs[k].weight += weight
                    bFound = True
                    break

            if bFound == False:
                pair = WeightJointIndexPair()
                pair.weight = weight
                pair.joint_index = joint_index
                final_pairs.append(pair)

    # Set joint index zero if the weight is zero. (old way)
    # User can now selected (new UI) to define their own 'root' joint and have it be non-zero (0-1 float range)
    # (often if there is an issue with a strand anchoring and not moving (all weights 0) or if they
    # are using a subset of joints and masking out the rest..and want a root from that group) 
    # Needed with complex real world skeletal rigs where you often have weights purposely set to 0 
    # (like in a facial rig) on the main skincluster
    final_pairs_sum = 0
    for i in xrange(len(final_pairs)):
        final_pairs_sum += final_pairs[i].weight
        if final_pairs[i].weight == 0:
            final_pairs[i].joint_index = defaultJointRootIndex
            final_pairs[i].weight = defaultJointRootWeight

    final_pairs.sort()

    # TODO: Is it needed to make the sum of weight equal to 1?
    # New UI: now has checkbox option to let you re-normalize the weights to 1 (optional only) 
    bReNormalizeFinalPairs =  cmds.checkBox("RenormalizeFinalPairs", q = True, v = True)
    if (bReNormalizeFinalPairs == True) and (final_pairs_sum > 0):
        for i in xrange(len(final_pairs)):
            final_pairs[i].weight /= final_pairs_sum

    # number of elements of final_pairs could be more than _maxJointsPerVertex but it should be at least _maxJointsPerVertex. 
    # If you really want it to be exactly _maxJointsPerVertex, you can try to pop out elements. 
    return final_pairs            

# p0, p1, p2 are three vertices of a triangle and p is inside the triangle
def ComputeBarycentricCoordinates(p0, p1, p2, p):
    v0 = p1 - p0
    v1 = p2 - p0
    v2 = p - p0

    v00 = v0 * v0
    v01 = v0 * v1
    v11 = v1 * v1
    v20 = v2 * v0
    v21 = v2 * v1
    d = v00 * v11 - v01 * v01
    v = (v11 * v20 - v01 * v21) / d # TODO: Do I need to care about divide-by-zero case?
    w = (v00 * v21 - v01 * v20) / d
    u = 1.0 - v - w

    # make sure u, v, w are non-negative. It could happen sometimes.
    u = max(u, 0)
    v = max(v, 0)
    w = max(w, 0)

    # normalize barycentric coordinates so that their sum is equal to 1
    sum = u + v + w
    u /= sum
    v /= sum
    w /= sum

    return [u, v, w]

def SaveTFXBoneBinaryFile(filepath, selected_mesh_shape_name, meshShapedagPath, rootPositions): 
    #---------------------------------------------------------------------------
    # Build a face/triangle index list to convert face index into triangle index
    #---------------------------------------------------------------------------
    faceIter = OpenMaya.MItMeshPolygon(meshShapedagPath)
    triangleCount = 0
    faceTriaIndexList = []
    index = 0

    util = OpenMaya.MScriptUtil()
    util.createFromInt(0)

    while not faceIter.isDone():
        faceTriaIndexList.append(triangleCount)

        if faceIter.hasValidTriangulation():
            numTrianglesPtr = util.asIntPtr()
            faceIter.numTriangles(numTrianglesPtr)
            numTriangles = util.getInt(numTrianglesPtr)        
            triangleCount += numTriangles

        faceIter.next()

    #print "TressFX: Triangle count:%d\n" % triangleCount

    #----------------------
    # Find the closest face
    #----------------------
    meshFn = OpenMaya.MFnMesh(meshShapedagPath)
    meshIntersector = OpenMaya.MMeshIntersector()
    meshIntersector.create(meshShapedagPath.node())

    triangleCounts = OpenMaya.MIntArray()
    triangleVertexIndices = OpenMaya.MIntArray() # the size of this array is three times of the number of total triangles
    meshFn.getTriangles(triangleCounts, triangleVertexIndices)

    vertexTriangleList = []

    triangleIdForStrandsList = []
    baryCoordList = []
    uvCoordList = []
    pointOnMeshList = []

    progressBar = ProgressBar('Collecting bone data', len(rootPositions))

    for i in range(len(rootPositions)):
        rootPoint = rootPositions[i]

        # Find the closest point info
        meshPt = OpenMaya.MPointOnMesh()
        meshIntersector.getClosestPoint(rootPoint, meshPt)
        pt = meshPt.getPoint()

        pointOnMesh = OpenMaya.MPoint()
        pointOnMesh.x = pt.x
        pointOnMesh.y = pt.y
        pointOnMesh.z = pt.z
        pointOnMeshList.append(pointOnMesh)

        # Find face index
        faceId = meshPt.faceIndex()

        # Find triangle index
        triangleId = faceTriaIndexList[faceId] + meshPt.triangleIndex()

        dummy = OpenMaya.MScriptUtil()
        dummyIntPtr = dummy.asIntPtr()
        triangleId_local = meshPt.triangleIndex() # This values is either 0 or 1. It is not a global triangle index. triangleId is the global triangle index. 
        pointArray = OpenMaya.MPointArray()
        vertIdList = OpenMaya.MIntArray()
        faceIter.setIndex(faceId, dummyIntPtr)
        faceIter.getTriangle(triangleId_local, pointArray, vertIdList, OpenMaya.MSpace.kWorld )

        vertexTriangleList.append((vertIdList[0], vertIdList[1], vertIdList[2]))
        triangleIdForStrandsList.append(triangleId)

        # Find three vertex indices for the triangle. Following two lines should give us three correct vertex indices for the triangle. 
        # I haven't really verified though. 
        #vertexIndex = [triangleVertexIndices[triangleId*3], triangleVertexIndices[triangleId*3+1], triangleVertexIndices[triangleId*3+2]]
        vertexIndex = [vertIdList[0], vertIdList[1], vertIdList[2]]

        # Find barycentric coordinates
        uvw = OpenMaya.MPoint()

        # Somehow, below code gives me negative barycentric coordinates sometimes. 
        # uPtr = OpenMaya.MScriptUtil().asFloatPtr()
        # vPtr = OpenMaya.MScriptUtil().asFloatPtr()
        # meshPt.getBarycentricCoords(uPtr,vPtr)
        # uvw.x = OpenMaya.MScriptUtil(uPtr).asFloat()
        # uvw.y = OpenMaya.MScriptUtil(vPtr).asFloat()
        # uvw.z = 1.0 - uvw.x - uvw.y  

        # Instead getting barycentric coords from getBarycentricCoords, we compute it by the following function. 
        uvw_a = ComputeBarycentricCoordinates(pointArray[0], pointArray[1], pointArray[2], pointOnMesh)

        uvw.x = uvw_a[0]
        uvw.y = uvw_a[1]
        uvw.z = uvw_a[2]

        # barycentric coordinates should be non-zero
        #uvw.x = max(uvw.x, 0)
        #uvw.y = max(uvw.y, 0)
        #uvw.z = max(uvw.z, 0)

        # normalize barycentric coordinates so that their sum is equal to 1
        #sum = uvw.x + uvw.y + uvw.z
        #uvw.x /= sum
        #uvw.y /= sum
        #uvw.z /= sum

        baryCoordList.append(uvw)

        # Find UV coordinates - We don't really use UV coords for tfxbone file. 
        # util = OpenMaya.MScriptUtil()
        # util.createFromList([0.0, 0.0], 2)
        # uv_ptr = util.asFloat2Ptr()
        # meshFn.getUVAtPoint(rootPoint, uv_ptr)
        # u = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_ptr, 0, 0)
        # v = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_ptr, 0, 1)
        # uv_coord = OpenMaya.MPoint()
        # uv_coord.x = u
        # uv_coord.y = v
        # uv_coord.z = 0
        # uvCoordList.append(uv_coord)

        # update progress gui
        progressBar.Increment()

    progressBar.Kill()

    #-------------------------
    # Get skin cluster object 
    #-------------------------     
    skinClusterName = ''
    skinClusters = cmds.listHistory(selected_mesh_shape_name)
    skinClusters = cmds.ls(skinClusters, type="skinCluster")
    if skinClusters:
        skinClusterName = skinClusters[0]
    else:
        cmds.warning('TressFX: No skin cluster found on '+ selected_mesh_shape_name)
        return

    #print skinClusterName

    #---------------------------------------------------------------------------------------------------
    # TODO: Try the following method. 
    # skins = filter(lambda skin: mesh.getShape() in skin.getOutputGeometry(), ls(type='skinCluster'))

    # if len(skins) > 0 :
        # skin = skins[0]    
        # skinJoints = skin.influenceObjects();
        # root = skinJoints[0]

        # while root.getParent() :
            # root = root.getParent()

        # skin.getWeights(mesh.verts[index])    

        # select(root, hierarchy=True, replace=True)
        # joints = ls(selection=True, transforms=True, type='joint')
    #---------------------------------------------------------------------------------------------------

    # get the MFnSkinCluster using skinClusterName
    selList = OpenMaya.MSelectionList() 
    selList.add(skinClusterName) 
    skinClusterNode = OpenMaya.MObject() 
    selList.getDependNode(0, skinClusterNode) 
    skinFn = OpenMayaAnim.MFnSkinCluster(skinClusterNode)

    dagPaths = MDagPathArray()
    skinFn.influenceObjects(dagPaths)

    # influence object is a joint
    influenceObjectsNames = []
    influenceObjectsNames = GetInfluenceObjects(dagPaths)

    influenceSubsetIndices = []
    #check to see if using subset and got the indices to match
    bUseJointSubset = cmds.checkBox("UseJointSubsetLabel", q = True, v = True)
    if (bUseJointSubset == True):
        influenceSubsetIndices = GetIndicesSubsetInfluenceObjects(skinFn, dagPaths )
        if (len(influenceSubsetIndices) == 0):
            cmds.warning("TressFX: Joint Subset activated but failed to get matching indices, defaulting to full joint set")
            bUseJointSubset = False		
    
    
    #do we have bones?
    if (len(influenceObjectsNames) == 0):
        cmds.warning('TressFX: NO INFLUENCES FOR SKINCLUSTER FOUND! ')
    else: #how many bones?
        numBonesNeeded = len(influenceObjectsNames)
        cmds.warning('TressFX: NOTICE: Make sure TressFX Max Allowed Bones (in a skeleton) is set to greater than: ' + str(numBonesNeeded))

    skinMeshes = cmds.skinCluster(skinClusterName, query=1, geometry=1)
    geoIter = OpenMaya.MItGeometry(meshShapedagPath)
    infCount = OpenMaya.MScriptUtil()
    infCountPtr = infCount.asUintPtr()   
    numVertices = geoIter.count()

    weightArray = [0] * TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  * numVertices # joint weight array for all vertices. Each vertex will have TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  weights. 
                                                            # It is initialized with zero for empty weight in case there are less weights than TRESSFX_MAX_INFLUENTIAL_BONE_COUNT .
    jointIndexArray = [-1] * TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  * numVertices # joint index array for all vertices. It is initialized with -1 for an empty element in case 
                                                                # there are less weights than TRESSFX_MAX_INFLUENTIAL_BONE_COUNT . 

    # collect bone weights for all vertices in the mesh
    index = 0
    while geoIter.isDone() == False:
        weights = OpenMaya.MDoubleArray()
        skinFn.getWeights(meshShapedagPath, geoIter.currentItem(), weights, infCountPtr)
        weightJointIndexPairs = []

        sumWtsThisVertex = 0.0
    
        for i in range(len(weights)):
            sumWtsThisVertex += weights[i]

            pair = WeightJointIndexPair()
            pair.weight = weights[i]
            if (bUseJointSubset == True) and (i not in influenceSubsetIndices) and (pair.weight > 0.0):
                    #print("JointSubset: Setting pair weight:%5.2f to zero for bone index: %d (%s)\n" % (pair.weight, i, influenceObjectsNames[i]))
                    pair.weight = 0.0 #don't use this weight, not in subset of joints we want
                    
            pair.joint_index = i 
            weightJointIndexPairs.append(pair)
            
        weightJointIndexPairs.sort()

        a = 0
        sumWtAdjusted = 0.0
        for j in range(min(len(weightJointIndexPairs), TRESSFX_MAX_INFLUENTIAL_BONE_COUNT )):
            weightArray[index*TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  + a] = weightJointIndexPairs[j].weight
            jointIndexArray[index*TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  + a] = weightJointIndexPairs[j].joint_index
            a += 1
            sumWtAdjusted += weightJointIndexPairs[j].weight

        index += 1
        geoIter.next()

    #------------------------
    # Save the tfxbone file.
    #------------------------
    progressBar = ProgressBar('Saving a tfxbone file', len(influenceObjectsNames) + len(triangleIdForStrandsList))
    f = open(filepath, "wb")
    # Number of Bones
    f.write(ctypes.c_int(len(influenceObjectsNames)))

    # Write all bone (joint) names
    for i in range(len(influenceObjectsNames)):
        # Bone Joint Index
        f.write(ctypes.c_int(i))
        # Size of the string, add 1 to leave room for the nullterminate.
        f.write(ctypes.c_int(len(influenceObjectsNames[i]) + 1))
        # Print the characters of the string 1 by 1.
        for j in range(len(influenceObjectsNames[i])):
            f.write(ctypes.c_byte(ord(influenceObjectsNames[i][j])))
        # Add a zero to null terminate the string.
        f.write(ctypes.c_byte(0))
        progressBar.Increment()

    # Number of Strands
    f.write(ctypes.c_int(len(triangleIdForStrandsList)))

    for i in range(len(triangleIdForStrandsList)):
        triangleId = triangleIdForStrandsList[i]

        # three vertex indices from one triangle - Following two lines should work equally but I haven't verified it yet. 
        #vertexIndices = [triangleVertexIndices[triangleId*3], triangleVertexIndices[triangleId*3+1], triangleVertexIndices[triangleId*3+2]]
        vertexIndices = vertexTriangleList[i]

        baryCoord = baryCoordList[i]   
        #todo: Investigate if we are getting outlier case where a baryCoord is causing a strand to go to zero? 
        # We should be using sorted (i.e. of all weights, only top 4 and 
        # which will multiply barycoord against the weight and if we get a weight with 0 in the final four, it is set to the default root,
        # which is joint0 at wt0 (which, if there are no other non-zero influences, would freeze the hair in place)
        # This is why a custom root setting was added, with a settable weight (i.e. non zero), just in case we get tiny barycoords that force
        # a resulting weight to zero (and have few bones influencing that hair, such that all the weight goes to zero)

        #get final pairs (post adjustment by baryCoord)
        weightJointIndexPairs = GetSortedWeightsFromTriangleVertices(TRESSFX_MAX_INFLUENTIAL_BONE_COUNT , vertexIndices, jointIndexArray, weightArray, baryCoord)

        # Index, the rest should be self explanatory.
        f.write(ctypes.c_int(i))
        for j in range(4):
            joint_index = 0
            weight = 0.0

            try:
                joint_index = weightJointIndexPairs[j].joint_index
                weight = weightJointIndexPairs[j].weight
            except:
                print("TressFX: Saving Bone file, exception getting joint_index/bone weight for triangleId %d in StrandsList" % i) #big error: throw exception to stop
                pass

            f.write(ctypes.c_int(joint_index))
            f.write(ctypes.c_float(weight))
        progressBar.Increment()

    f.close()
    progressBar.Kill()
    return

def RecursiveSearchCurve(curves, objNode, minCurveLength):
    if objNode.hasFn(OpenMaya.MFn.kNurbsCurve):
        curveFn = OpenMaya.MFnNurbsCurve(objNode)
        curveLength = curveFn.length()

        # We only export open type curves.         
        if curveFn.form() == OpenMaya.MFnNurbsCurve.kOpen:
            if curveLength >= minCurveLength:
                curves.append(curveFn)

    elif objNode.hasFn(OpenMaya.MFn.kTransform):
        objFn = OpenMaya.MFnTransform(objNode)

        for j in range(objFn.childCount()):
            childObjNode = objFn.child(j)
            RecursiveSearchCurve(curves, childObjNode, minCurveLength)

def GetSelectedNurbsCurves(minCurveLength):
    slist = OpenMaya.MSelectionList()
    OpenMaya.MGlobal.getActiveSelectionList( slist )
    iter = OpenMaya.MItSelectionList(slist)
    curves = []

    # Find all nurbs curves under the selected node recursively. 
    #TODO. Confirm that this should be doing a 'select hierarachy' so user doesn't have to...
    for i in range(slist.length()):
        selObj = OpenMaya.MObject()
        slist.getDependNode(i, selObj)
        RecursiveSearchCurve(curves, selObj, minCurveLength)

    return curves    

class TressFXTFXFileHeader(ctypes.Structure):
    _fields_ = [('version', ctypes.c_float),
                ('numHairStrands', ctypes.c_uint),
                ('numVerticesPerStrand', ctypes.c_uint),
                ('offsetVertexPosition', ctypes.c_uint),
                ('offsetStrandUV', ctypes.c_uint),
                ('offsetVertexUV', ctypes.c_uint),
                ('offsetStrandThickness', ctypes.c_uint),
                ('offsetVertexColor', ctypes.c_uint),
                ('reserved', ctypes.c_uint * 32)]

class tressfx_float4(ctypes.Structure):
    _fields_ = [('x', ctypes.c_float),
                ('y', ctypes.c_float),
                ('z', ctypes.c_float),
                ('w', ctypes.c_float)]

class tressfx_float2(ctypes.Structure):
    _fields_ = [('x', ctypes.c_float),
                ('y', ctypes.c_float)]

def SaveTFXBinaryFile(filepath, curves, meshShapedagPath):
    numCurves = len(curves)
    numVerticesPerStrand = cmds.optionMenu("numberOfStrandsOptionMenu", query=True, value=True)
    numVerticesPerStrand = int(numVerticesPerStrand)

    #check to see if we need to scale the points
    sceneScale = cmds.optionMenu('scalingOptionMenu', query=True, value=True)
    sceneScale = float(sceneScale)
    print("TressFX: Saving TFX Binary:scene scale multiplier = %f" % sceneScale) #todo: make sure scaling doesn't affect uv issues (if there is an issue, that is)

    #sampling options
    curveSample = cmds.optionMenu("samplingOptionMenu", query=True, value=True)
    curveSample = int(curveSample)
    curveIndex_Offset = cmds.intField("curveOffset",q = True, v = True)
    #sanity check
    if curveIndex_Offset >= numCurves:
        cmds.warning('TressFX: Curve Offset requested Greater or Equal to actual Number of Curves - Please Lower Offset Value')
        return
        
    bothEndsImmovable = cmds.checkBox("bothEndsImmovable",q = True, v = True)
    invertZ = cmds.checkBox("InvertZ",q = True, v = True)
    useZUp = cmds.checkBox("useZUp",q = True, v = True)
    bChangeUpToZ = False
    if (useZUp == True):
        #query current up axis in use
        currentAxis = cmds.upAxis(q = True, axis = True)
        if (currentAxis == 'y'):
            bChangeUpToZ = True
            cmds.warning("TressFX: Maya currently using Y axis as UP, Z up requested: doing y/z swap")
        elif (currentAxis != 'z'):
            cmds.warning("TressFX: Problem detected, attempt to detect Maya UP axis setting failed. No change to default UP axis (no y/z swap)")


    ignoreUVErrors = cmds.checkBox("ignoreUVErrorsCheckBox", q = True, v = True)

    #useCurl = cmds.checkBox("useCurl",q = True, v = True)
    rootPositions = []

    tfxHeader = TressFXTFXFileHeader()
    tfxHeader.version = 4.0
    tfxHeader.numHairStrands = int(numCurves//curveSample) #number of curves may be a subset if we are sampling a subset only
    tfxHeader.numVerticesPerStrand = numVerticesPerStrand
    tfxHeader.offsetVertexPosition = ctypes.sizeof(TressFXTFXFileHeader)
    tfxHeader.offsetStrandUV = 0
    tfxHeader.offsetVertexUV = 0
    tfxHeader.offsetStrandThickness = 0
    tfxHeader.offsetVertexColor = 0

    meshFn = None
    meshIntersector = None

    #if sampling at a lower amount than the full curve set, div by sample over entire set, then mult by sample to jump
    #if offseting, subtract offset amount from full range of loop, so when it's added to the index
    #it won't overshoot the actual number of curves range	
    adjustedCurveRange = int(numCurves//curveSample) - curveIndex_Offset #floor division is //
    #sanity check 2
    if curveIndex_Offset >= adjustedCurveRange:
        cmds.warning('TressFX: Curve Offset requested Greater or Equal to subset:Sampled Curves - Please Lower Offset Value')
        return
    
    # if meshShapedagPath is passed then let's get strand texture coords. To do this, we need MFnMesh and MMeshIntersector objects. 
    if meshShapedagPath != None:
        meshFn = OpenMaya.MFnMesh(meshShapedagPath)
        meshIntersector = OpenMaya.MMeshIntersector()
        meshIntersector.create(meshShapedagPath.node())
#		tfxHeader.offsetStrandUV = tfxHeader.offsetVertexPosition + numCurves * numVerticesPerStrand * ctypes.sizeof(tressfx_float4)
        tfxHeader.offsetStrandUV = tfxHeader.offsetVertexPosition + adjustedCurveRange * numVerticesPerStrand * ctypes.sizeof(tressfx_float4)

    bInvertYForUVs = cmds.checkBox("InvertYForUVs",q = True, v = True)


    #totalProgress = numCurves*numVerticesPerStrand #old, non adjusted for sampling of curves
    totalProgress = adjustedCurveRange*numVerticesPerStrand

    if meshFn != None:
        #totalProgress = numCurves*numVerticesPerStrand + numCurves #old, non adjusted for sampling of curves
        totalProgress = adjustedCurveRange*numVerticesPerStrand + adjustedCurveRange

    progressBar = ProgressBar('Saving a tfx file', totalProgress)

    f = open(filepath, "wb")
    f.write(tfxHeader)

    #if sampling at a lower amount than the full curve set, div by sample over entire set, then mult by sample to jump
    #if offseting, subtract offset amount from full range of loop, so when it's added to the index
    #it won't overshoot the actual number of curves range	
    #adjustedCurveRange = int(numCurves//curveSample) - curveIndex_Offset #floor division is //
    for i in xrange(adjustedCurveRange):
        curveFn = curves[(i*curveSample) + curveIndex_Offset] #adjusted curve range to accomodate sampling and offsetting into the curve set

        # getting Min/Max value of the nurbs curve
        min = OpenMaya.MScriptUtil()
        min.createFromDouble(0) 
        minPtr = min.asDoublePtr() 
        max = OpenMaya.MScriptUtil() 
        max.createFromDouble(0) 
        maxPtr = max.asDoublePtr() 
        curveFn.getKnotDomain(minPtr, maxPtr)        
        min_val = OpenMaya.MScriptUtil(minPtr).asDouble()
        max_val = OpenMaya.MScriptUtil(maxPtr).asDouble()
        
        for j in range(0, numVerticesPerStrand):
            param = j/ float(numVerticesPerStrand-1)       
            pos = OpenMaya.MPoint()
            
            param = param * (max_val - min_val)
            curveFn.getPointAtParam(param, pos, OpenMaya.MSpace.kObject) # kObject
            
            p = tressfx_float4()
            p.x = pos.x
            p.y = pos.y

            if invertZ:
                p.z = -pos.z # flip in z-axis
            else:
                p.z = pos.z
                
            #if invertY: #no use case currently
            #	p.y = -pos.y
            #else:
            #	p.y = pos.y

            if (bChangeUpToZ == True): #Unreal uses Z as up (not Y), and Maya is currently using Y (so we need to swap values)
                temp = p.y
                p.y = p.z
                p.z = temp #not sure if can use p.yz = p.zy (or if supported on all possible python builds used with this exporter) so will use old fashioned way
            

            # w component is an inverse mass
            if j == 0 or j == 1: # the first two vertices are immovable always. 
                p.w = 0
            else:
                p.w = 1.0 
            
            if j == (numVerticesPerStrand-1) and bothEndsImmovable: #fix the last vertice of strand
                p.w = 0
            
            if (sceneScale != 1.0):
                #print('tfx scaling doing it...not 1.0')
                p.x = p.x * sceneScale
                p.y = p.y * sceneScale
                p.z = p.z * sceneScale
                
            f.write(p)
            progressBar.Increment()
            

        # # find face index
        rootPos = OpenMaya.MPoint()
        curveFn.getPointAtParam(0, rootPos, OpenMaya.MSpace.kObject) # must be kObject
        rootPositions.append(rootPos)
        

    # if meshShapedagPath is passed then let's get strand texture coords by using raycasting to the mesh from each root position of hair strand.   
    if meshFn != None: 
        #last known good u,v
        #in case we hit bad uv points, we can at least try to set them closer to the
        #last u,v set (versus 0,0)
        u_lkg = 0.0
        v_lkg = 0.0

        uMin = 0.0
        uMax = 1.0	
        vMin = 0.0
        vMax = 1.0	
        #query to see if we have a user defined custom uv bounding box
        bCustomUVRange = cmds.checkBox("useNonUniformUVRange",q = True, v = True)
        if (bCustomUVRange == True) and (bInvertYForUVs == True):
            #only need the min and max of v (currently)
            uMin = cmds.floatField("uMin", q = True, v = True)
            uMax = cmds.floatField("uMax", q = True, v = True)
            vMin = cmds.floatField("vMin", q = True, v = True)
            vMax = cmds.floatField("vMax", q = True, v = True)
            cmds.warning("TressFX: Using custom UV range. This v range (%5.2f, %5.2f) will be used during DirectX y-flipping ('v' reflect) for uv coordinates." % (vMin, vMax))
        print ("TressFX: UV bounding box is: u[%5.2f, %5.2f], v[%5.2f, %5.2f]" % (uMin, uMax, vMin, vMax))

        for i in range(len(rootPositions)):
            rootPoint = rootPositions[i]

                # Find UV coordinates 
            util = OpenMaya.MScriptUtil()
            util.createFromList([0.0, 0.0], 2)
            uv_ptr = util.asFloat2Ptr()
            
            try: 
                meshFn.getUVAtPoint(rootPoint, uv_ptr)
                u = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_ptr, 0, 0)
                v = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_ptr, 0, 1)
                u_lkg = u
                v_lkg = v
            except:
                #if NOT strict mode' then ok to give point a default 0,0 uv point value, else kill the file
                #cmds.warning('Exception Hit! meshFn.getUVAtPoint failed for rootPoint')
                if ignoreUVErrors:
                    cmds.warning('TressFX: UV point error Exception (strict mode off): UV point failed for rootPoint->Ignoring Exception, using last known good (lkg) as uv instead')
                    u = u_lkg
                    v = v_lkg
                else:
                    f.close()
                    progressBar.Kill()
                    cmds.warning('TressFX: UV point error Exception (strict mode on): UV point failed for rootPoint->Failing to create TFX. Deleting the open TFX file: ' + filepath)
                    cmds.sysFile(filepath, delete=True) #remove the damaged file
                    return
                    
            uv_coord = tressfx_float2()
            uv_coord.x = u
            uv_coord.y = v

            if (bInvertYForUVs == True):
                uv_coord.y = vMax - uv_coord.y + vMin #uv_coord.y = 1.0 - uv_coord.y # DirectX has it inverted, uniform means a typical bounding box of 0-1 u and v, so we would actually use 1- v + 0 to invert
                
            #print "uv:%g, %g\n" % (uv_coord.x, uv_coord.y)
            f.write(uv_coord)	
            progressBar.Increment()

    f.close()
    progressBar.Kill()

    return rootPositions

class TressFXSkinFileObject(ctypes.Structure):
    _fields_ = [('version', ctypes.c_uint),
                ('numHairs', ctypes.c_uint),
                ('numTriangles', ctypes.c_uint),
                ('reserved1', ctypes.c_uint * 31), 
                ('hairToMeshMap_Offset', ctypes.c_uint),
                ('perStrandUVCoordniate_Offset', ctypes.c_uint),
                ('reserved1', ctypes.c_uint * 31)]      

class HairToTriangleMapping(ctypes.Structure):
    _fields_ = [('mesh', ctypes.c_uint),
                ('triangle', ctypes.c_uint),
                ('barycentricCoord_x', ctypes.c_float),
                ('barycentricCoord_y', ctypes.c_float),
                ('barycentricCoord_z', ctypes.c_float),
                ('reserved', ctypes.c_uint)]    

def SaveTFXSkinBinaryFile(filepath, meshShapedagPath, rootPositions): 
    #---------------------------------------------------------------------------
    # Build a face/triangle index list to convert face index into triangle index
    #---------------------------------------------------------------------------
    faceIter = OpenMaya.MItMeshPolygon(meshShapedagPath)
    triangleCount = 0
    faceTriaIndexList = []
    index = 0
    bInvertYForUVs = cmds.checkBox("InvertYForUVs",q = True, v = True)

    util = OpenMaya.MScriptUtil()
    util.createFromInt(0)

    while not faceIter.isDone():
        faceTriaIndexList.append(triangleCount)

        if faceIter.hasValidTriangulation():
            numTrianglesPtr = util.asIntPtr()
            faceIter.numTriangles(numTrianglesPtr)
            numTriangles = util.getInt(numTrianglesPtr)        
            triangleCount += numTriangles

        faceIter.next()

    #----------------------
    # Find the closest face
    #----------------------
    meshFn = OpenMaya.MFnMesh(meshShapedagPath)
    meshIntersector = OpenMaya.MMeshIntersector()
    meshIntersector.create(meshShapedagPath.node())

    faceIdList = []
    baryCoordList = []
    uvCoordList = []
    progressBar = ProgressBar('Collecting skin data', len(rootPositions))

    for i in range(len(rootPositions)):
        rootPoint = rootPositions[i]

        # Find the closest point info
        meshPt = OpenMaya.MPointOnMesh()
        meshIntersector.getClosestPoint(rootPoint, meshPt)
        pt = meshPt.getPoint()

        pointOnMesh = OpenMaya.MPoint()
        pointOnMesh = pt

        # Find face index
        faceId = meshPt.faceIndex()

        # Find triangle index
        triangleId = faceTriaIndexList[faceId] + meshPt.triangleIndex()
        faceIdList.append(triangleId)

        # Find barycentric coordinates
        uPtr = OpenMaya.MScriptUtil().asFloatPtr()
        vPtr = OpenMaya.MScriptUtil().asFloatPtr()
        meshPt.getBarycentricCoords(uPtr,vPtr)
        uvw = OpenMaya.MPoint()
        uvw.x = OpenMaya.MScriptUtil(uPtr).asFloat()
        uvw.y = OpenMaya.MScriptUtil(vPtr).asFloat()
        uvw.z = 1.0 - uvw.x - uvw.y  
        baryCoordList.append(uvw)

        # TODO: Why are there negative barycentric coords?
        #if uvw.x < 0 or uvw.y < 0 or uvw.z < 0:
        #    print 'uvw:', uvw.x, uvw.y, uvw.z

        # Find UV coordinates 
        util = OpenMaya.MScriptUtil()
        util.createFromList([0.0, 0.0], 2)
        uv_ptr = util.asFloat2Ptr()
        meshFn.getUVAtPoint(rootPoint, uv_ptr)
        u = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_ptr, 0, 0)
        v = OpenMaya.MScriptUtil.getFloat2ArrayItem(uv_ptr, 0, 1)
        uv_coord = OpenMaya.MPoint()
        uv_coord.x = u
        uv_coord.y = v
        uv_coord.z = 0
        uvCoordList.append(uv_coord)

        # update progress gui
        progressBar.Increment()

    progressBar.Kill()

    #--------------------
    # Save a tfxskin file
    #--------------------
    tfxSkinObj = TressFXSkinFileObject()
    tfxSkinObj.version = 1
    tfxSkinObj.numHairs = len(faceIdList)
    tfxSkinObj.numTriangles = 0
    tfxSkinObj.hairToMeshMap_Offset = ctypes.sizeof(TressFXSkinFileObject)
    tfxSkinObj.perStrandUVCoordniate_Offset = tfxSkinObj.hairToMeshMap_Offset + len(faceIdList) * ctypes.sizeof(HairToTriangleMapping)

    f = open(filepath, "wb")
    f.write(tfxSkinObj)

    progressBar = ProgressBar('Saving a tfxskin file', len(faceIdList) + len(uvCoordList))

    for i in xrange(len(faceIdList)):
        mapping = HairToTriangleMapping()
        mapping.mesh = 0
        mapping.triangle = faceIdList[i]

        uvw = baryCoordList[i]
        mapping.barycentricCoord_x = uvw.x
        mapping.barycentricCoord_y = uvw.y
        mapping.barycentricCoord_z = uvw.z

        f.write(mapping)
        progressBar.Increment()

    # per strand uv coordinate
    for i in xrange(len(uvCoordList)):
        uv_coord = uvCoordList[i]
        p = Point()
        p.x = uv_coord.x

        if bInvertYForUVs:
            p.y = 1.0 - uv_coord.y # DirectX has it inverted

        p.z = uv_coord.z         
        f.write(p)    
        progressBar.Increment()

    f.close()
    progressBar.Kill()

    return

def GetSortedWeightsFromOneVertex(_maxJointsPerVertex, vertexIndex, jointIndexArray, weightArray):
    final_pairs = []
    sumFinal = 0.0

    #noDupNonzeroWts = cmds.checkBox("noDupBoneWts", q = True, v = True)

    for j in range(_maxJointsPerVertex):
        weight = weightArray[vertexIndex*_maxJointsPerVertex + j]
        joint_index = jointIndexArray[vertexIndex*_maxJointsPerVertex + j]

        bFound = False
        for k in range(len(final_pairs)):
            if final_pairs[k].joint_index == joint_index:
                #if noDupNonzeroWts:
                #	if final_pairs[k].weight == 0: #no dup bones case
                #		final_pairs[k].weight += weight
                #else:
                #	final_pairs[k].weight += weight
                final_pairs[k].weight += weight
                bFound = True
                break

        if bFound == False:
            pair = WeightJointIndexPair()
            pair.weight = weight
            pair.joint_index = joint_index
            final_pairs.append(pair)

    #Set joint index to the user defined root/root weight (default is joint 0 with 0 weight) if the weight is zero. 
    for i in xrange(len(final_pairs)):
        if final_pairs[i].weight == 0:
            final_pairs[i].joint_index = defaultJointRootIndex
            final_pairs[i].weight = defaultJointRootWeight
        sumFinal += final_pairs[i].weight 

    #Normalize so that the sum of the final pairs weight is 1.0 (must be done after we have a final summation of the total weight)
    bNormalizeVtWts = cmds.checkBox("RenormalizeFinalPairs", q = True, v = True)
    if (bNormalizeVtWts == True):
        if (sumFinal > 0.0):
            for i in xrange(len(final_pairs)):
                final_pairs[i].weight /= sumFinal 
        else:
            cmds.warning("TressFX: problem with final weights for vertex: %d, total weight <= 0.0: Try using joint subset + custom root joint/weight to compensate." % vertexIndex)

    final_pairs.sort()

    # number of elements of final_pairs could be more than _maxJointsPerVertex but it should be at least _maxJointsPerVertex. 
    # If you really want it to be exactly _maxJointsPerVertex, you can try to pop out elements. 
    return final_pairs  


def is_match(regex, text):
    pattern = re.compile(regex, text)
    return pattern.search(text) is not None

def ExportMesh(filepath, meshShapedagPath): 
    meshFn = OpenMaya.MFnMesh(meshShapedagPath)
    meshIntersector = OpenMaya.MMeshIntersector()
    meshIntersector.create(meshShapedagPath.node())

    faceIdList = []
    baryCoordList = []

    points = OpenMaya.MPointArray()
    meshFn.getPoints(points, OpenMaya.MSpace.kWorld)

    normals = OpenMaya.MFloatVectorArray()
    meshFn.getVertexNormals(False, normals, OpenMaya.MSpace.kWorld)

    triangleCounts = OpenMaya.MIntArray()
    triangleVertexIndices = OpenMaya.MIntArray() # the size of this array is three times of the number of total triangles
    meshFn.getTriangles(triangleCounts, triangleVertexIndices)

    #-------------------------
    # Get skin cluster object 
    #-------------------------     
    skinClusterName = ''
    mesh_shape_name = meshFn.name()
    skinClusters = cmds.listHistory(mesh_shape_name)
    skinClusters = cmds.ls(skinClusters, type="skinCluster")
    if skinClusters:
        skinClusterName = skinClusters[0]
    else:
        cmds.warning('TressFX: No skin cluster found on '+ mesh_shape_name)
        return

    #print skinClusterName

    # get the MFnSkinCluster using skinClusterName
    selList = OpenMaya.MSelectionList() 
    selList.add(skinClusterName) 
    skinClusterNode = OpenMaya.MObject() 
    selList.getDependNode(0, skinClusterNode) 
    skinFn = OpenMayaAnim.MFnSkinCluster(skinClusterNode)

    dagPaths = MDagPathArray()
    skinFn.influenceObjects(dagPaths)

    # influence object is a joint
    influenceObjectsNames = []

    #do we need to remove the namespace if present?
    bRemoveNSColMesh = cmds.checkBox("removeNamespaceCM", q = True, v = True)

    #check if we need to scale the points
    sceneScaleCol = cmds.optionMenu('scalingCollisionOptionMenu', query=True, value=True)
    sceneScaleCol = float(sceneScaleCol)
    print("TressFX: Exporting Collision Mesh:scene scale multiplier = %f" % sceneScaleCol) #Scaling, must apply to collision and hair exporting


    # get joint names
    #currently a collision mesh will parse through all the joints for that skinCluster (no limiting to a subset at this time)
    for i in range(dagPaths.length()):
        influenceName = dagPaths[i].partialPathName() 
        if bRemoveNSColMesh:
            influenceNameList = re.split("[:]", influenceName)
            if len(influenceNameList) != 1: #has a namespace prefix
                influenceObjectsNames.append(influenceNameList[-1])
            else:
                influenceObjectsNames.append(influenceNameList[0])
        else:      
            influenceObjectsNames.append(influenceName) # Need to remove namespace?     
        #influenceObjectsNames.append(influenceName) # Need to remove namespace?

    skinMeshes = cmds.skinCluster(skinClusterName, query=1, geometry=1)
    geoIter = OpenMaya.MItGeometry(meshShapedagPath)
    infCount = OpenMaya.MScriptUtil()
    infCountPtr = infCount.asUintPtr()   
    numVertices = geoIter.count()

    weightArray = [0] * TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  * numVertices # joint weight array for all vertices. Each vertex will have TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  weights. 
                                                            # It is initialized with zero for empty weight in case there are less weights than TRESSFX_MAX_INFLUENTIAL_BONE_COUNT .
    jointIndexArray = [-1] * TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  * numVertices # joint index array for all vertices. It is initialized with -1 for an empty element in case 
                                                                # there are less weights than TRESSFX_MAX_INFLUENTIAL_BONE_COUNT . 

    # collect bone weights for all vertices in the mesh
    index = 0

    progressBar = ProgressBar('Collect data', numVertices)

    while geoIter.isDone() == False:
        weights = OpenMaya.MDoubleArray()
        skinFn.getWeights(meshShapedagPath, geoIter.currentItem(), weights, infCountPtr)

        weightJointIndexPairs = []

        for i in range(len(weights)):
            pair = WeightJointIndexPair()
            pair.weight = weights[i]
            pair.joint_index = i 
            weightJointIndexPairs.append(pair)
            

        weightJointIndexPairs.sort() 

        a = 0

        for j in range(min(len(weightJointIndexPairs), TRESSFX_MAX_INFLUENTIAL_BONE_COUNT )):
            weightArray[index*TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  + a] = weightJointIndexPairs[j].weight
            jointIndexArray[index*TRESSFX_MAX_INFLUENTIAL_BONE_COUNT  + a] = weightJointIndexPairs[j].joint_index
            a += 1

        index += 1
        progressBar.Increment()
        geoIter.next()

    progressBar.Kill()

    #----------------------------------------------------------    
    # We collected all necessary data. Now save them in file.  
    #----------------------------------------------------------  
    totalProgress = points.length() + triangleVertexIndices.length() / 3 + len(influenceObjectsNames)
    progressBar = ProgressBar('Export collision mesh', totalProgress)    

    f = open(filepath, "w")
    f.write("# TressFX collision mesh exported by TressFX Exporter in Maya\n")

    # Write all bone (joint) names
    f.write("numOfBones %g\n" % (len(influenceObjectsNames)))
    f.write("# bone index, bone name\n")
    for i in range(len(influenceObjectsNames)):
        f.write("%d %s\n" % (i, influenceObjectsNames[i]))
        progressBar.Increment()

    # write vertex positions and skinning data
    f.write("numOfVertices %g\n" % (points.length()))
    f.write("# vertex index, vertex position x, y, z, normal x, y, z, joint index 0, joint index 1, joint index 2, joint index 3, weight 0, weight 1, weight 2, weight 3\n")
    for vertexIndex in xrange(points.length()):
        point = points[vertexIndex]
        #do we have any scaling?
        if (sceneScaleCol != 1.0):
            #print("export: doing scaling..not 1.0")
            point.x = point.x * sceneScaleCol
            point.y = point.y * sceneScaleCol
            point.z = point.z * sceneScaleCol

        normal = normals[vertexIndex]
        weightJointIndexPairs = GetSortedWeightsFromOneVertex(TRESSFX_MAX_INFLUENTIAL_BONE_COUNT , vertexIndex, jointIndexArray, weightArray)
        f.write("%g %g %g %g %g %g %g %g %g %g %g %g %g %g %g\n" % (vertexIndex, point.x, point.y, point.z, normal.x, normal.y, normal.z, weightJointIndexPairs[0].joint_index, weightJointIndexPairs[1].joint_index, weightJointIndexPairs[2].joint_index, weightJointIndexPairs[3].joint_index,
                                                                weightJointIndexPairs[0].weight, weightJointIndexPairs[1].weight, weightJointIndexPairs[2].weight, weightJointIndexPairs[3].weight))
        #todo: make this error checking optional, most of the time, it is unneeded
        sumWts = 0.0
        sumWts = weightJointIndexPairs[0].weight + weightJointIndexPairs[1].weight + weightJointIndexPairs[2].weight+ weightJointIndexPairs[3].weight
        if sumWts <= 0.0:
            cmds.warning('TressFX:  this vertex(%d) has zero total weight or negative weights' % vertexIndex)
        if sumWts > 1.001: #python seems to think 1 is greater than 1.0, so adding a threshold
            cmds.warning('TressFX:  this vertex(%d) has bone weights that total > 1 [%f]' % (vertexIndex, sumWts))

        progressBar.Increment()

    # write triangle face indices    
    f.write("numOfTriangles %g\n" % (triangleVertexIndices.length() / 3))    
    f.write("# triangle index, vertex index 0, vertex index 1, vertex index 2\n")
    for i in range(triangleVertexIndices.length() / 3):
        f.write("%g %d %d %d\n" % (i, triangleVertexIndices[i * 3 + 0], triangleVertexIndices[i * 3 + 1], triangleVertexIndices[i * 3 + 2]))
        progressBar.Increment()

    f.close()
    progressBar.Kill()
    return

def DoExportCollisionMesh(*args):
    #------------------------------
    # Find the selected mesh shape
    #------------------------------
    meshShapedagPath = OpenMaya.MDagPath()

    if selected_mesh_shape_name == '':
        cmds.warning("TressFX: To export skin or bone data, base mesh must be set.\n")
        return

    allObject = cmds.ls(selected_mesh_shape_name)
    cmds.select(allObject) # TODO: This selection makes hair curves unselected. This is not a problem but just inconvenient for users if they want to keep the curves selected.  

    sel_list = OpenMaya.MSelectionList()
    OpenMaya.MGlobal.getActiveSelectionList(sel_list)

    if sel_list.length() == 0:
        return

    sel_list.getDagPath(0, meshShapedagPath)
    meshShapedagPath.extendToShape() # get mesh shape

    basicFilter = "*.tfxmesh"
    filepath = cmds.fileDialog2(fileFilter=basicFilter, dialogStyle=2, caption="Save a tfx collision mesh file(*.tfxmesh)", fileMode=0)

    if filepath == None or len(filepath) == 0:
        return

    ExportMesh(filepath[0], meshShapedagPath)    
    return

