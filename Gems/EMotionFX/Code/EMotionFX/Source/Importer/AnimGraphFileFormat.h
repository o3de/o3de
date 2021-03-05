/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "SharedFileFormatStructs.h"
#include "AzCore/RTTI/TypeInfo.h"

namespace EMotionFX
{
    namespace FileFormat // so now we are in the namespace EMotionFX::FileFormat
    {
        // collection of animGraph chunk IDs
        enum
        {
            ANIMGRAPH_CHUNK_BLENDNODE                 = 400,
            ANIMGRAPH_CHUNK_STATETRANSITIONS          = 401,
            ANIMGRAPH_CHUNK_NODECONNECTIONS           = 402,
            ANIMGRAPH_CHUNK_PARAMETERS                = 403,
            ANIMGRAPH_CHUNK_NODEGROUPS                = 404,
            ANIMGRAPH_CHUNK_GROUPPARAMETERS           = 405,
            ANIMGRAPH_CHUNK_GAMECONTROLLERSETTINGS    = 406,
            ANIMGRAPH_CHUNK_ADDITIONALINFO            = 407,
            ANIMGRAPH_FORCE_32BIT                     = 0xFFFFFFFF
        };

        enum
        {
            ANIMGRAPH_NODEFLAG_COLLAPSED          = 1 << 0,
            ANIMGRAPH_NODEFLAG_VISUALIZED         = 1 << 1,
            ANIMGRAPH_NODEFLAG_DISABLED           = 1 << 2,
            ANIMGRAPH_NODEFLAG_VIRTUALFINALOUTPUT = 1 << 3
        };

        /*
            AnimGraph_Header

            ANIMGRAPH_CHUNK_PARAMETERS: (global animgraph parameters)
                uint32 numParameters
                AnimGraph_ParamInfo[numParameters]

            ANIMGRAPH_CHUNK_BLENDNODE:
                AnimGraph_NodeHeader

            ANIMGRAPH_CHUNK_NODECONNECTIONS: (for last loaded BLENDNODE)
                uint32 numConnections
                AnimGraph_NodeConnection[numConnections]

            ANIMGRAPH_CHUNK_STATETRANSITIONS: (for last loaded node, assumed to be a state machine)
                uint32 numStateTransitions
                uint32 blendNodeIndex   (the state machine the transitions are for)
                AnimGraph_StateTransition[numStateTransitions]

            ANIMGRAPH_CHUNK_NODEGROUPS:
                uint32 numNodeGroups
                AnimGraph_NodeGroup[numNodeGroups]

            ANIMGRAPH_CHUNK_GAMECONTROLLERSETTINGS:
                uint32 activePresetIndex
                uint32 numPresets
                AnimGraph_GameControllerPreset[numPresets]
        */

        // AnimGraph file header
        struct AnimGraph_Header
        {
            char    mFourCC[4];
            uint8   mEndianType;
            uint32  mFileVersion;
            uint32  mNumNodes;
            uint32  mNumStateTransitions;
            uint32  mNumNodeConnections;
            uint32  mNumParameters;

            // followed by:
            //      string  mName;
            //      string  mCopyright;
            //      string  mDescription;
            //      string  mCompany;
            //      string  mEMFXVersion;
            //      string  mEMStudioBuildDate;
        };

        // additional info
        struct AnimGraph_AdditionalInfo
        {
            uint8   mUnitType;
        };


        // the node header
        struct AnimGraph_NodeHeader
        {
            uint32  mTypeID;
            uint32  mParentIndex;
            uint32  mVersion;
            uint32  mNumCustomDataBytes;        // number of bytes of node custom data to follow
            uint32  mNumChildNodes;
            uint32  mNumAttributes;
            int32   mVisualPosX;
            int32   mVisualPosY;
            uint32  mVisualizeColor;
            uint8   mFlags;

            // followed by:
            //      string mName;
            //      animGraphNode->Save(...) or animGraphNode->Load(...), writing or reading mNumBytes bytes
        };


        struct AnimGraph_ParameterInfo
        {
            uint32  mNumComboValues;
            uint32  mInterfaceType;
            uint32  mAttributeType;
            uint16  mFlags;
            char    mHasMinMax;

            // followed by:
            //      string mName
            //      string mInternalName
            //      string mDescription
            //      if (mHasMinMax == 1)
            //      {
            //          AnimGraph_Attribute mMinValue
            //          AnimGraph_Attribute mMaxValue
            //      }
            //      AnimGraph_Attribute mDefaultValue
            //      string mComboValues[mNumComboValues]
        };


        // a node connection
        struct AnimGraph_NodeConnection
        {
            uint32  mSourceNode;
            uint32  mTargetNode;
            uint16  mSourceNodePort;
            uint16  mTargetNodePort;
        };


        // a state transition
        struct AnimGraph_StateTransition
        {
            uint32  mSourceNode;
            uint32  mDestNode;
            int32   mStartOffsetX;
            int32   mStartOffsetY;
            int32   mEndOffsetX;
            int32   mEndOffsetY;
            uint32  mNumConditions;

            // followed by:
            // AnimGraph_NodeHeader (and its followed by data, EXCEPT THE NAME STRING, which is skipped)
            // AnimGraph_NodeHeader[mConditions] (and its followed by data, EXCEPT THE NAME STRING, which is skipped)
        };


        // a node group
        struct AnimGraph_NodeGroup
        {
            FileColor   mColor;
            uint8       mIsVisible;
            uint32      mNumNodes;

            // followed by:
            //      string mName
            //      uint32[mNumNodes] (node indices that belong to the group)
        };


        // a group parameter
        struct AnimGraph_GroupParameter
        {
            uint32      mNumParameters;
            uint8       mCollapsed;

            // followed by:
            //      string mName
            //      uint32[mNumParameters] (parameter indices that belong to the group)
        };


        // a game controller parameter info
        struct AnimGraph_GameControllerParameterInfo
        {
            uint8   mAxis;
            uint8   mMode;
            uint8   mInvert;

            // followed by:
            //      string mName
        };


        // a game controller button info
        struct AnimGraph_GameControllerButtonInfo
        {
            uint8   mButtonIndex;
            uint8   mMode;

            // followed by:
            //      string mString
        };


        // a game controller preset
        struct AnimGraph_GameControllerPreset
        {
            uint32      mNumParameterInfos;
            uint32      mNumButtonInfos;

            // followed by:
            //      string mName
            //      AnimGraph_GameControllerParameterInfo[mNumParameterInfos]
            //      AnimGraph_GameControllerButtonInfo[mNumButtonInfos]
        };

        // Conversion functions to support attributes with the old serialization.
        // Once we deprecate the old format we can remove these two functions.
        AZ::TypeId GetParameterTypeIdForInterfaceType(uint32 interfaceType);
        uint32 GetInterfaceTypeForParameterTypeId(const AZ::TypeId& parameterTypeId);

    } // namespace FileFormat
} // namespace EMotionFX
