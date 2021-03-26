#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
from maya import cmds
import math
import maya.mel

#defines the shape labels
g_shapeTypes = ['box', 'capsule', 'sphere']
g_materialNames = ['mat_proxy_phys', 'no material']
g_boxIndex = 0
g_capsuleIndex = 1
g_sphereIndex = 2

g_orientTypes = ['x', 'y', 'z', 'xCenter', 'yCenter', 'zCenter']
g_defaultOrientTypeIndex = 0
g_xIndex = 0
g_yIndex = 1
g_zIndex = 2
g_xCenterIndex = 3
g_yCenterIndex = 4
g_zCenterIndex = 5
g_centerString = 'Center'

##########
##The colors that will be used in the editor
##########
g_UIColor = [0.165, 0.165, 0.165]

g_defaultMaterialColor = [0, 1, 0]

##########
##Width and height definitions for ui elements 
##########
g_uiProxyWidthColumnWidth = 30
g_controlHeight = 22
g_listboxHeight = 35
g_buttonSpace = 4
g_rowItemHeight = 22

g_uiShapePulldownColumnWidth = 75
g_uiOrientPulldownColumnWidth = 50
g_uiMaterialNamePulldownWidth = 120
g_uiMaterialFieldColumnWidth = 125
g_scrollBarPadding = 36

g_buttonMarginPadding = 10
g_marginWidth = 5
g_boxWidth = 400

g_uiColumnWidth = [g_uiProxyWidthColumnWidth, g_uiShapePulldownColumnWidth, g_uiOrientPulldownColumnWidth, g_uiMaterialFieldColumnWidth]


#TODO: make width/height dynamic
g_windowHeight = 120
g_windowWidth = 318

#default name of display layer where proxies will reside
g_nameOfDisplayLayer = 'lyr_lumberyard_phys'

#postfix of the physics proxy
g_physPostfix = '_phys';

#the default width of proxies
g_proxyShapeDefaultWidth = 10

#estimating that the proxy width is 1/10th of the hip height
g_proxyWidthRatioToHeight = 0.1

class ProxyObject(object):

    ##This class represents both the variables and the ui of the proxy editor
    def __init__(self) :
        
        #defines the ui of the width text field
        self.uiTextFieldWidth = ''
        
        #defines the value of the width and depth
        self.width = g_proxyShapeDefaultWidth
        
        #defines the ui of the shape drop down ui
        self.menuDropDownShape = ''
        
        #defines this proxy's shape
        self.shape = g_shapeTypes[g_boxIndex]
        
        #defines this proxies material name
        self.proxyMaterialName = ''
        
        #defines the ui used to modify orientation
        self.menuDropDownOrient = ''
        
        #defines the orientation of the proxy
        self.objectOrient = g_orientTypes[g_defaultOrientTypeIndex]
        
        self.menuDropDownMaterialName = ''   

proxyCreationSettings = ProxyObject()       
        
        
def LumberyardProxyTool():
    ##Create the main window.  If it exists, delete it, and then create it
    if cmds.window('LUMBERYARDTOOL_PROXY_WINDOW', exists=True):  
        cmds.deleteUI('LUMBERYARDTOOL_PROXY_WINDOW')

    LumberyardProxyToolUI()

def LumberyardProxyToolUI():
    ##This script defines the window or primary canvas of the ui.   
    lumberyardProxyWindow = cmds.window('LUMBERYARDTOOL_PROXY_WINDOW', title = "Proxy Tool (Experimental)", sizeable = False, iconName = 'Proxy Tool', widthHeight = (g_windowWidth, g_windowHeight))
    LumberyardToolCreateProxyFrameMain()
    cmds.showWindow(lumberyardProxyWindow) 
    cmds.window(lumberyardProxyWindow, edit = True, widthHeight = (g_windowWidth, g_windowHeight))  
    
def LumberyardToolCreateProxyFrameMain():
    ##When the window is first open this is what the user will see
    cmds.columnLayout('LUMBERYARDTOOL_PROXY_LAYOUT', adjustableColumn = True)
    
    cmds.text(height = g_controlHeight, enable = True, font = 'tinyBoldLabelFont', align = 'center', width = g_uiProxyWidthColumnWidth+3, label = 'Select joints and click "Create Proxies"', backgroundColor = g_UIColor)
   
    cmds.setParent('LUMBERYARDTOOL_PROXY_WINDOW')
    
    cmds.rowLayout(numberOfColumns = 4, columnWidth4 = g_uiColumnWidth, backgroundColor = g_UIColor)
    
    #title labels for all the variable categories that we'll have control over
    cmds.text(height = g_controlHeight, enable = True, font = 'tinyBoldLabelFont', align = 'left', width = g_uiProxyWidthColumnWidth+3, label = 'width', backgroundColor = g_UIColor)
    cmds.text(height = g_controlHeight, enable = True, font = 'tinyBoldLabelFont', align = 'left', width = g_uiShapePulldownColumnWidth, label = 'shape', backgroundColor = g_UIColor)
    cmds.text(height = g_controlHeight, enable = True, font = 'tinyBoldLabelFont', align = 'left', width = g_uiOrientPulldownColumnWidth+25, label = 'orient', backgroundColor = g_UIColor)
    cmds.text(height = g_controlHeight, enable = True, font = 'tinyBoldLabelFont', align = 'left', width = g_uiMaterialFieldColumnWidth + g_marginWidth, label = 'material name', backgroundColor = g_UIColor)
    
    ##This is the editable area of the ui that will be populated by our variables.  In the beginning it is unpopulated but has instruction on what to do
    cmds.setParent('LUMBERYARDTOOL_PROXY_LAYOUT')   
    cmds.scrollLayout ('LUMBERYARDTOOL_PROXY_LISTBOX', height = g_listboxHeight, width = g_boxWidth , backgroundColor = [0, 0, 0], visible = True)
    
    cmds.setParent('LUMBERYARDTOOL_PROXY_LISTBOX')   
    
    #create a row
    cmds.rowLayout(numberOfColumns = 4, columnWidth4 = g_uiColumnWidth)
        
    #create the width ui
    LumberyardCreateWidthTextField()
    
    #create the shape ui
    LumberyardCreateShapePullDown()
    
    #create the orient ui
    LumberyardCreateOrientPullDown()
    
    LumberyardCreateMaterialPullDown()

    #create the ui underneath the editable variables window.  This is the canvas for our create button
    cmds.setParent('LUMBERYARDTOOL_PROXY_LAYOUT') 
    cmds.setParent('..')
    cmds.rowColumnLayout(numberOfRows = 1, width = g_boxWidth , columnSpacing = [1, g_buttonSpace])
    cmds.button(label = 'Create Proxies', width = g_windowWidth-4, command = LumberyardCreateProxies, backgroundColor = g_UIColor)   
  
def LumberyardCreateWidthTextField():
    ##This function creates the ui element that defines the width and depth of the proxy
    
    #get the width of the proxy from the class
    uiTextFieldProxyWidthValue = g_proxyShapeDefaultWidth
    
    #create the ui text field
    proxyCreationSettings.uiTextFieldWidth = cmds.textField(width = g_uiProxyWidthColumnWidth , backgroundColor = g_UIColor, text = uiTextFieldProxyWidthValue)
    
    #add a command that will add functionality when the textfield is changed
    cmds.textField(proxyCreationSettings.uiTextFieldWidth, edit = True, changeCommand = lambda *args: LumberyardChangeProxyWidth(cmds.textField(proxyCreationSettings.uiTextFieldWidth, query = True, text = True)))
    
    LumberyardChangeProxyWidth(g_uiProxyWidthColumnWidth)
 
def LumberyardChangeProxyWidth(widthInput):
    ##This function is run when the width text field is updated
    
    #get current width
    currentWidth = proxyCreationSettings.width
    
    #determine if this entry is a float
    if LumberyardFloatConfirm(widthInput) == True:
        
        if float(widthInput) > 0:
            #if the entry is a float and greater than zero, update the class with the new width
            proxyCreationSettings.width = widthInput
            
        else:
            #zero or negative floats are not a valid width value, so revert to previous value
            cmds.textField(proxyCreationSettings.uiTextFieldWidth, edit = True, text = currentWidth)
        
    else:
        #if the entry is not a float, revert it to its previous value
        cmds.textField(proxyCreationSettings.uiTextFieldWidth, edit = True, text = currentWidth)
        
def LumberyardCreateOrientPullDown():
    ##This function creates the ui pulldown for orientation
    
    #create the ui pulldown
    orientOptionPullDown = cmds.optionMenu(label = '', width = g_uiShapePulldownColumnWidth, backgroundColor = g_UIColor, changeCommand = lambda *args: LumberyardProxySetOrientType())
    
    #add it to the class
    proxyCreationSettings.menuDropDownOrient = orientOptionPullDown
    
    #populate the pulldown with the predefined orient types
    for orient in g_orientTypes:
        cmds.menuItem('button_' + orient, label = orient)
        
    LumberyardProxySetOrientType()

def LumberyardProxySetOrientType():
    ##This function is launched when the orient value is changed in the editor
    
    #get the new value from the menu
    orientString =  (cmds.optionMenu(proxyCreationSettings.menuDropDownOrient, query = True, value = True))
    
    #store the new value in the class
    proxyCreationSettings.objectOrient = orientString

def LumberyardCreateShapePullDown():
    ##This function creates the ui pulldown for shape types in the editor
    
    #create the ui pulldown
    shapeOptionPullDown = cmds.optionMenu(label = '', width = g_uiShapePulldownColumnWidth, backgroundColor = g_UIColor,changeCommand = lambda *args: LumberyardProxySetShapeType())
    
    #add it to the class
    proxyCreationSettings.menuDropDownShape = shapeOptionPullDown
    
    #populate the pulldown with the predefined shape types
    for shapeType in g_shapeTypes:
        cmds.menuItem('button_' + shapeType, label = shapeType)
    
    LumberyardProxySetShapeType()
    
def LumberyardCreateMaterialPullDown():
    ##This function creates the ui pulldown for material name
    
    #create the ui pulldown
    materialOptionPullDown = cmds.optionMenu(label = '', width = g_uiMaterialFieldColumnWidth, backgroundColor = g_UIColor, changeCommand = lambda *args: LumberyardProxySetMaterialName())
    
    #add it to the class
    proxyCreationSettings.menuDropDownMaterialName = materialOptionPullDown
    
    #populate the pulldown with the predefined materialName types
    for materialName in g_materialNames :
        cmds.menuItem('button_' + materialName, label = materialName)
        
    LumberyardProxySetMaterialName() 

def LumberyardProxySetMaterialName():

    materialNameString = (cmds.optionMenu(proxyCreationSettings.menuDropDownMaterialName, query = True, value = True))
    
    proxyCreationSettings.proxyMaterialName = materialNameString
    
def LumberyardProxySetShapeType():
    ##This function is launched when the shape value is changed in the editor
    
    #get the new value from the menu
    shapeString = (cmds.optionMenu(proxyCreationSettings.menuDropDownShape, query = True, value = True))
    
    #store the new value in the class
    proxyCreationSettings.shape = shapeString
  
def LumberyardAddMaterialToProxy(inputProxy):
    ##This function adds a material to a proxy shape object
    
    #get the material name from the class
    thisMaterialName = proxyCreationSettings.proxyMaterialName
    
    #get the shading group
    shadingGroup = thisMaterialName + 'SG'
    
    #variable that defines the shader
    shader = ''
    
    #if the material does not exist, create a shader group that supports it
    if cmds.objExists(thisMaterialName) == False:    
        shader = cmds.shadingNode('phong', asShader = True, name = thisMaterialName)
        #add attribute that will define this proxy material
        cmds.addAttr(shader, longName = 'lumberyardPhysMaterial', defaultValue = 1)
        shadingGroup = cmds.sets(renderable = True, noSurfaceShader = True, empty = True, name = shadingGroup) 
        cmds.connectAttr(shader +'.outColor', shadingGroup + '.surfaceShader')
        cmds.setAttr(shader + '.color', g_defaultMaterialColor[0], g_defaultMaterialColor[1], g_defaultMaterialColor[2], type = 'double3')
    else:
        shader = 'thisMaterialName'    
    #apply the shading group to the proxy object
    cmds.sets(inputProxy[0], edit = True, forceElement = shadingGroup )

def CreateProxyPositionFromChildren(jointNode):
    #get the children. the joint's children define the length of the proxy
    jointChildren = cmds.listRelatives(jointNode, children = True, path = True, type = 'joint')
            
    #get the joint node's position in world space
    jointStartPosition = cmds.xform(jointNode, query = True, worldSpace = True, translation = True)
            
    #get the average joint position for all of the child joints this will determine the proxy's length
    sumOfJointPositionX = 0.0
    sumOfJointPositionY = 0.0      
    sumOfJointPositionZ = 0.0
    proxyPositionX = jointStartPosition[0]
    proxyPositionY = jointStartPosition[1]
    proxyPositionZ = jointStartPosition[2]
           
    #if a joint has children, find out the length of the average child joints
    if (jointChildren):
        for jointChild in jointChildren:
            jointPositionWorldSpace = cmds.xform( jointChild, query = True, worldSpace = True, translation = True)  
            sumOfJointPositionX = sumOfJointPositionX + jointPositionWorldSpace[0] 
            sumOfJointPositionY = sumOfJointPositionY + jointPositionWorldSpace[1]
            sumOfJointPositionZ = sumOfJointPositionZ + jointPositionWorldSpace[2]
        averageJointPositionX = sumOfJointPositionX / len(jointChildren) 
        averageJointPositionY = sumOfJointPositionY / len(jointChildren)
        averageJointPositionZ = sumOfJointPositionZ / len(jointChildren)                              
        #move to the midpoint of joint and averaged child joints
        proxyPositionX = ((averageJointPositionX + jointStartPosition[0])/2)
        proxyPositionY = ((averageJointPositionY + jointStartPosition[1])/2)
        proxyPositionZ = ((averageJointPositionZ + jointStartPosition[2])/2)
        
    proxyPosition = [proxyPositionX, proxyPositionY, proxyPositionZ]
    return proxyPosition
    
def LumberyardCreateProxies(*args):
    ##This function creates the proxy objects and parents them to joints
  
    #create display layers for proxy's visibility to easily be toggled
    LumberyardCreateProxyLayer()
    
    #variable for the display layer
    layerPhys = g_nameOfDisplayLayer
    
    #make a list of all of the selected objects
    selectedObjects = cmds.ls(selection = True)
    
    #if nothing is selected throw a warning
    if (len(selectedObjects) == 0): 
        cmds.warning('No joints selected.  Select Joints and press the Add Joints button.')
        
    else:
        for selectedObject in selectedObjects:
        
            if cmds.objectType(selectedObject, isType = 'joint' ) == True:
                
                shapeOfProxy = proxyCreationSettings.shape
                
                #get the joint node's position in world space
                jointStartPosition = cmds.xform(selectedObject, query = True, worldSpace = True, translation = True)
                proxyPosition = jointStartPosition
                proxyWidth = float(proxyCreationSettings.width)
                proxyLength = proxyWidth

                if (g_centerString in proxyCreationSettings.objectOrient) == False:
                
                    proxyPosition = CreateProxyPositionFromChildren(selectedObject)
                    
                    #find out the length of the average child joints
                    distanceBetweenPoints = math.sqrt(math.pow((jointStartPosition[0] - proxyPosition[0]), 2) + math.pow((jointStartPosition[1] - proxyPosition[1]) ,2) + math.pow((jointStartPosition[2] - proxyPosition[2]), 2))
                    proxyLength = distanceBetweenPoints * 2

                #create the proxy shape
                if (shapeOfProxy == g_shapeTypes[g_boxIndex]):
                    proxyShape = cmds.polyCube(height = proxyLength, width = proxyWidth, depth = proxyWidth, createUVs = 1,constructionHistory = True)  
                else:
                    proxyRadius = proxyWidth/2
                    if (shapeOfProxy == g_shapeTypes[g_capsuleIndex]):
                        if (proxyLength > proxyWidth):
                            proxyShape = cmds.polyCylinder(radius = proxyRadius, height = proxyLength - (proxyRadius *2), roundCap = True, subdivisionsCaps = 3)
                        else:
                            proxyShape = cmds.polySphere(radius = proxyRadius * 2, subdivisionsAxis = 16, subdivisionsHeight = 16)
                    else: 
                        proxyShape = cmds.polySphere(radius = proxyRadius * 2, subdivisionsAxis = 16, subdivisionsHeight = 16)
            
                #add object to the phys display layer 
                cmds.editDisplayLayerMembers( layerPhys, proxyShape[0])
                
                #add an attribute to easily find our created proxy
                cmds.addAttr(proxyShape, longName = 'lumberyardPhysProxy', defaultValue = 1)
            
                #add material to proxy
                if (proxyCreationSettings.proxyMaterialName != 'no material'):
                
                    LumberyardAddMaterialToProxy(proxyShape)
            
                #add orientation to proxy
                LumberyardOrientProxy(proxyShape[0])
                 
                #move proxy into place
                cmds.move(proxyPosition[0], proxyPosition[1], proxyPosition[2], proxyShape, worldSpace = True)
                
                #orient the proxy so its aiming toward the parent joint. can we do this with vector math instead?
                aimConsProxy = cmds.aimConstraint(selectedObject, proxyShape)
                cmds.delete(aimConsProxy)
                orientConsProxy = cmds.orientConstraint(selectedObject, proxyShape, skip = 'none')
                cmds.delete(orientConsProxy)
            
                #if the orient pulldown defines this as a center, center its orient on the joint.
                if (g_centerString in proxyCreationSettings.objectOrient): 
                    cmds.move(jointStartPosition[0], jointStartPosition[1],jointStartPosition[2], proxyShape[0], worldSpace = True)
                else:
                    cmds.move(jointStartPosition[0], jointStartPosition[1], jointStartPosition[2], proxyShape[0] + ".scalePivot", proxyShape[0] + ".rotatePivot", worldSpace = True)
                    
                #parent it to the joint
                cmds.parent(proxyShape[0], selectedObject)
                
                #freeze the proxy.  this will zero out its translate values relative to its parent joint
                cmds.makeIdentity(proxyShape[0], apply = True, scale = 1, rotate = 1, translate = 1, normal = 0)
                
                #delete construction history
                cmds.delete(proxyShape[0],constructionHistory=True)
                
                #rename the proxy with the proper name
                cmds.rename(proxyShape[0], selectedObject + g_physPostfix)
            else:
                cmds.warning(selectedObject + " is not a joint.  No proxy created.")
                
def LumberyardOrientProxy(proxyObjectInput):
    ##This function orients the proxy based on the settings in the editor ui
    
    if proxyCreationSettings.objectOrient == g_orientTypes[g_xIndex] or proxyCreationSettings.objectOrient == g_orientTypes[g_xCenterIndex]:
        cmds.xform(proxyObjectInput, rotateAxis = [0, 90, 90])

    if proxyCreationSettings.objectOrient == g_orientTypes[g_yIndex] or proxyCreationSettings.objectOrient == g_orientTypes[g_yCenterIndex]:
        cmds.xform(proxyObjectInput, rotateAxis = [0, 0, 0])

    if proxyCreationSettings.objectOrient == g_orientTypes[g_zIndex] or proxyCreationSettings.objectOrient == g_orientTypes[g_zCenterIndex]:
        cmds.xform(proxyObjectInput, rotateAxis = [90, 0, 0])
   
def LumberyardCreateProxyLayer():
    ##This function creates a layer for our proxy objects so visibility may be easily toggled
    #if there is not already a display layer for the proxies, create one
    if cmds.objExists(g_nameOfDisplayLayer) == False:
        cmds.createDisplayLayer(name = g_nameOfDisplayLayer)

def LumberyardFloatConfirm(inputString):
    ##This function checks to see if the entered string is a float value and returns true or false
    try:
        float(inputString)
    except ValueError:
        return False
    else:
        return True
       