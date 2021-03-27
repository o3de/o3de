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
#cryLib.py
#some useful functions and classes

from pyfbsdk import *
from euclid import *

#casts point3 strings to pyEuclid vectors
def vec3(point3):
    return Vector3(point3[0], point3[1], point3[2])

#casts a pyEuclid vector to FBVector3d
def fbv(point3):
    return FBVector3d(point3.x, point3.y, point3.z)

#returns average position of an FBModelList as FBVector3d
def avgPos(models):
    mLen = len(models)
    if mLen == 1:
        return models[0].Translation
    total = vec3(models[0].Translation)
    for i in range (1, mLen):
        total += vec3(models[i].Translation)
    avgTranslation = total/mLen
    return fbv(avgTranslation)

#returns an array of models when given an array of model names
#useful with external apps/telnetlib ui
def modelsFromStrings(modelNames):
    output = []
    for name in modelNames:
        output.append(FBFindModelByName(name))
    return output

#stabilizes face markers, input 4 FBModelList arrays, leaveOrig <bool> for lraving original markers
def stab(right,left,center,markers,leaveOrig):
    
    pMatrix = FBMatrix()
    lSystem=FBSystem()
    lScene = lSystem.Scene
    newMarkers = []
    
    def faceOrient():
        lScene.Evaluate()
        
        Rpos = vec3(avgPos(right))
        Lpos = vec3(avgPos(left))
        Cpos = vec3(avgPos(center))
        
        faceAttach.GetMatrix(pMatrix)
        xVec = (Cpos - Rpos)
        xVec = xVec.normalize()
        zVec = ((Cpos - vec3(faceAttach.Translation)).normalize()).cross(xVec)
        zVec = zVec.normalize()
        yVec = xVec.cross(zVec)
        yVec = yVec.normalize()
        facePos = (Rpos + Lpos)/2
        
        pMatrix[0] = xVec.x
        pMatrix[1] = xVec.y
        pMatrix[2] = xVec.z
        
        pMatrix[4] = yVec.x
        pMatrix[5] = yVec.y
        pMatrix[6] = yVec.z
        
        pMatrix[8] = zVec.x
        pMatrix[9] = zVec.y
        pMatrix[10] = zVec.z
        
        pMatrix[12] = facePos.x
        pMatrix[13] = facePos.y
        pMatrix[14] = facePos.z
        
        faceAttach.SetMatrix(pMatrix,FBModelTransformationMatrix.kModelTransformation,True)
        lScene.Evaluate()
        
    def keyTransRot(animNodeList):
        for lNode in animNodeList:
            if (lNode.Name == 'Lcl Translation'):
                lNode.KeyCandidate()
            if (lNode.Name == 'Lcl Rotation'):
                lNode.KeyCandidate()
        
    Rpos = vec3(avgPos(right))
    Lpos = vec3(avgPos(left))
    Cpos = vec3(avgPos(center))
    
    faceAttach = FBModelNull("faceAttach")
    faceAttach.Show = True
    faceAttach.Translation = fbv((Rpos + Lpos)/2)
    faceOrient()
    
    for obj in markers:
        new = FBModelNull(obj.Name + '_stab')
        newTran = vec3(obj.Translation)
        new.Translation = fbv(newTran)
        new.Show = True
        new.Size = 20
        new.Parent = faceAttach
        newMarkers.append(new)
    
    Take = lScene.Takes[1]
    FStart = int(Take.LocalTimeSpan.GetStart().GetFrame(True))
    FStop = int(Take.LocalTimeSpan.GetStop().GetFrame(True)+1)
    lPlayerControl = FBPlayerControl()
    lPlayerControl.GotoStart()
    
    animNodes = faceAttach.AnimationNode.Nodes
    
    lFbp = FBProgress()
    lFbp.Caption = "Stabilization"
    lFbp.Text = "Progress..."
    
    for frame in range(FStart,FStop):
        
        faceOrient()
        
        for m in range (0,len(newMarkers)):
            markerAnimNodes = newMarkers[m].AnimationNode.Nodes
            newMarkers[m].SetVector(markers[m].Translation.Data)
            lScene.Evaluate()
            keyTransRot(markerAnimNodes)
        
        keyTransRot(animNodes)

        lPlayerControl.StepForward()
        lVal = (frame/FStop)*10
        lFbp.Percent = lVal
    
