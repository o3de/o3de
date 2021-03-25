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
################################################################################################################
#
#    mayaAnimUtilities.py
#
#    replaces mayaAnimUtilities.cpp
#
#    contains two functons
#        - AMZN_GetTimeFromFrame
#            Takes frame number as input
#            Returns time based on current Maya settings
#
#        - AMZN_DecodeAnimRangeString
#            Takes string as parameter representing Maya export settings
#            Returns array with values parced out
#
################################################################################################################








################################################################################################################
#
#    PURPOSE: To return the current time from a given frame
#
#    PROCEDURE: Using C++ api calls, create new MTime entity
#            Set the MTime entity to the current unit of time and current frame
#            returns the current time in secconds
#
#    PRESUMPTIONS: original c++ used MTime.as, which does not currently seem to be supported in Python
#            looks like MTime.asUnits is equivalent function
#
#################################################################################################################
import maya.OpenMaya as om

def AMZN_GetTimeFromFrame(frame):
    time = om.MTime() 

    time.setUnit( time.uiUnit() )
    time.setValue( frame )
    
    #original c++ uses MTime.as
    #as far as I can tell, MTime.asUnits is synonimous 
    return time.asUnits(om.MTime.kSeconds)





################################################################################################################
#
#    PURPOSE: To parse out the string containing the animation export options
#
#    PROCEDURE: First we split the string up via it's sentinal value " - ", then we take the first
#            element, which is the frame range, and pull out the start and end frames. We then
#            add them all back into one string
#
#    PRESUMPTIONS: 1) the items in the string are seperated by an " - " value
#                2) that there are no " - ' values in things lke path names (wasn't my idea for a sentinal value)
#                3) that the first element in the string is the time range in the format [startFrame-endFrame]
#
#################################################################################################################
def AMZN_DecodeAnimRangeString(str):

    returnArray = []    

    #split the export options by the sentinal value
    splitBySentinal = str.split(" - ")
    
    #first item in the array is the time range, need to parce out the numbers from the string    
    timeRange = splitBySentinal[0]
    timeRange = timeRange.translate(None, '[]')
    splitTimeRange = timeRange.split("-")
    
    del splitBySentinal[0]
    #print splitBySentinal
        
    returnArray.extend(splitTimeRange)
    returnArray.extend(splitBySentinal)
    
    return returnArray
    #print returnArray
    