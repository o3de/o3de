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
import maya.cmds as cmds


def crySetTransform(nodeName, translate, rotate):
    for i in xrange(3):
        cmds.setAttr(nodeName + '.translate' + 'XYZ'[i], translate[i])
        cmds.setAttr(nodeName + '.rotate' + 'XYZ'[i], rotate[i])


def crySetParent(parentName, oldNodeName, newNodeName):
    cmds.parent(oldNodeName, parentName)
    cmds.rename(parentName + '|' + oldNodeName, newNodeName)

# naturalOrientation == True:
#   Front View shows face of a character.
# naturalOrientation == False:
#   Front View shows back of a character. Mimics cryexporter for 3dsMax.
def cryMakeSceneRoot(naturalOrientation):
    root_name = 'SceneRoot'

    if cmds.objExists(root_name):
        parents = cmds.listRelatives(root_name, p=True)
        if parents:
            cmds.confirmDialog(
                title='CryTools',
                message='{0} node has a parent - it is not recommended.\nPlease delete the node {0} or move it to the top of the scene graph.'.format(root_name),
                button=['Ok',],
                defaultButton='Ok')
            cmds.select(root_name, r=True)
            return

        children = cmds.listRelatives(root_name, c=True)
        if children and sorted(children) != sorted(['forward', 'up']):
            cmds.confirmDialog(
                title='CryTools',
                message='A *non-standard* node {0} is found - it is not supported.\nPlease delete the node {0}.'.format(root_name),
                button=['Ok',],
                defaultButton='Ok')
            cmds.select(root_name, r=True)
            return

        cmds.delete(root_name, hi='all')

    # Creating SceneRoot node

    cmds.createNode('transform', n=root_name)

    # Creating geometry of SceneRoot,
    # including in-scene text labels for the axes.

    # Note that 'size' is actually a scaler, not a real size
    size = 1.4

    shaft_length = 60.0 * size
    shaft_radius = 2.0 * size
    arrowhead_length = 13.0 * size
    arrowhead_radius = 6.0 * size
    sphere_radius = 7.0 * size
    font_size = 40.0 * size
    font = "Arial|h-" + str(font_size) + "|w400|c0"

    obj = cmds.createNode('transform')
    crySetParent(root_name, obj, 'forward')
    obj = cmds.createNode('transform')
    crySetParent(root_name, obj, 'up')

    obj = cmds.polyCylinder(ch=True, o=True, ax=(0, 1, 0), r=shaft_radius, h=shaft_length, sc=1, cuv=3, sx=4)
    crySetTransform(obj[0], [0.0, shaft_length * 0.5, 0.0], [0.0, 0.0, 0.0])
    crySetParent(root_name+'|forward', obj[0], "shaft");
    obj = cmds.polyCylinder(ch=True, o=True, ax=(0, 0, 1), r=shaft_radius, h=shaft_length, sc=1, cuv=3, sx=4)
    crySetTransform(obj[0], [0.0, 0.0, shaft_length * 0.5], [0.0, 0.0, 0.0])
    crySetParent(root_name+'|up', obj[0], "shaft");

    obj = cmds.polyCone(ch=True, o=True, ax=(0, 1, 0), r=arrowhead_radius, h=arrowhead_length, cuv=3, sx=6)
    crySetTransform(obj[0], [0.0, shaft_length + arrowhead_length * 0.5, 0.0], [0.0, 0.0, 0.0])
    crySetParent(root_name+'|forward', obj[0], "arrowhead")
    obj = cmds.polySphere(ch=True, o=True, r=sphere_radius, sx=8, sy=8)
    crySetTransform(obj[0], [0.0, 0.0, shaft_length + sphere_radius * 0.5], [90.0, 0.0, 0.0])
    crySetParent(root_name+'|up', obj[0], "sphere")

    obj = cmds.textCurves(f=font, t="Forward")
    crySetTransform(obj[0], [-2.0 * shaft_radius, 0.1 * shaft_length, 0.0], [0.0, 0.0, 90.0])
    crySetParent(root_name+'|forward', obj[0], "text")
    obj = cmds.textCurves(f=font, t="Up")
    crySetTransform(obj[0], [-2.0 * shaft_radius, 0.0, 0.7 * shaft_length], [0.0, 90.0, 90.0])
    crySetParent(root_name+'|up', obj[0], "text")

    cmds.select(root_name+'|forward', hi=True)
    cmds.polyColorPerVertex(rgb=(0.0, 0.6, 0.0), a=1, cdo=True)
    cmds.select(root_name+'|up', hi=True)
    cmds.polyColorPerVertex(rgb=(0.0, 0.0, 1.0), a=1, cdo=True)

    # Setting orientation of SceneRoot node

    if cmds.upAxis(q=True, axis=True) == "z":
        if naturalOrientation:
            crySetTransform(root_name, [0, 0, 0], [0, 0, 180.0])
        else:
            crySetTransform(root_name, [0, 0, 0], [0, 0, 0])
    else:
        if naturalOrientation:
            crySetTransform(root_name, [0, 0, 0], [90.0, 0, 180.0])
        else:
            crySetTransform(root_name, [0, 0, 0], [-90.0, 0, 0])

    # Selecting SceneRoot node to let the user see that something happened and/or
    # to help the user delete/hide/etc. the node.
    cmds.select(root_name, r=True)

