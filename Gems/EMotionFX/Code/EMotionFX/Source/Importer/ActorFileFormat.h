/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SharedFileFormatStructs.h"

namespace EMotionFX
{
    namespace FileFormat // so now we are in the namespace EMotionFX::FileFormat
    {
        // collection of actor chunk IDs
        enum
        {
            ACTOR_CHUNK_NODE                  = 0,
            ACTOR_CHUNK_LIMIT                 = 6,
            ACTOR_CHUNK_INFO                  = 7,
            ACTOR_CHUNK_STDPROGMORPHTARGET    = 9,
            ACTOR_CHUNK_NODEGROUPS            = 10,
            ACTOR_CHUNK_NODES                 = 11,       // Actor_Nodes
            ACTOR_CHUNK_STDPMORPHTARGETS      = 12,       // Actor_MorphTargets
            ACTOR_CHUNK_NODEMOTIONSOURCES     = 14,       // Actor_NodeMotionSources
            ACTOR_CHUNK_ATTACHMENTNODES       = 15,       // Actor_AttachmentNodes
            ACTOR_CHUNK_PHYSICSSETUP          = 18,
            ACTOR_CHUNK_SIMULATEDOBJECTSETUP  = 19,
            ACTOR_CHUNK_MESHASSET             = 20,
            ACTOR_FORCE_32BIT                 = 0xFFFFFFFF
        };

        // When a struct is not aligned, memset the object to 0 before using it, otherwise you might end up with some garbage paddings.

        // the actor file type header
        // (aligned)
        struct Actor_Header
        {
            uint8 mFourcc[4];   // must be "ACTR"
            uint8 mHiVersion;   // high version (2  in case of v2.34)
            uint8 mLoVersion;   // low version  (34 in case of v2.34)
            uint8 mEndianType;  // the endian in which the data is saved [0=little, 1=big]
        };


        // (not aligned)
        struct Actor_Info
        {
            uint32  mNumLODs;               // the number of level of details
            uint32  mTrajectoryNodeIndex;   // the node number of the trajectory node used for motion extraction (NOTE: unused as there is no more trajectory node)
            uint32  mMotionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            float   mRetargetRootOffset;
            uint8   mUnitType;              // maps to EMotionFX::EUnitType
            uint8   mExporterHighVersion;
            uint8   mExporterLowVersion;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };


        // (not aligned)
        struct Actor_Info2
        {
            uint32  mNumLODs;               // the number of level of details
            uint32  mMotionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            uint32  mRetargetRootNodeIndex; // the retargeting root node index, most likely pointing to the hip or pelvis or MCORE_INVALIDINDEX32 when not set
            uint8   mUnitType;              // maps to EMotionFX::EUnitType
            uint8   mExporterHighVersion;
            uint8   mExporterLowVersion;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };


        // (aligned)
        struct Actor_Info3
        {
            uint32  mNumLODs;               // the number of level of details
            uint32  mMotionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            uint32  mRetargetRootNodeIndex; // the retargeting root node index, most likely pointing to the hip or pelvis or MCORE_INVALIDINDEX32 when not set
            uint8   mUnitType;              // maps to EMotionFX::EUnitType
            uint8   mExporterHighVersion;
            uint8   mExporterLowVersion;
            uint8   mOptimizeSkeleton;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };

        struct Actor_MeshAsset
        {
            // followed by:
            // string : Mesh asset id
        };

        // a node header
        // (not aligned)
        struct Actor_Node
        {
            FileQuaternion  mLocalQuat; // the local rotation (before hierarchy)
            FileVector3     mLocalPos;  // the local translation (before hierarchy)
            FileVector3     mLocalScale;// the local scale (before hierarchy)
            uint32          mSkeletalLODs;// each bit representing if the node is active or not, in the give LOD (bit number)
            uint32          mParentIndex;// parent node number, or 0xFFFFFFFF in case of a root node
            uint32          mNumChilds; // the number of child nodes
            uint8           mNodeFlags; // #1 bit boolean specifies whether we have to include this node in the bounds calculation or not
            float           mOBB[16];

            // followed by:
            // string : node name (the unique name of the node)
        };

        // uv (texture) coordinate
        // (aligned)
        struct Actor_UV
        {
            float   mU;
            float   mV;
        };

        //-------------------------------------------------------

        // node limit information
        // (aligned)
        struct Actor_Limit
        {
            FileVector3     mTranslationMin;// the minimum translation values
            FileVector3     mTranslationMax;// the maximum translation value.
            FileVector3     mRotationMin;   // the minimum rotation values
            FileVector3     mRotationMax;   // the maximum rotation values
            FileVector3     mScaleMin;      // the minimum scale values
            FileVector3     mScaleMax;      // the maximum scale values
            uint8           mLimitFlags[9]; // the limit type activation flags
            uint32          mNodeNumber;    // the node number where this info belongs to
        };


        // a  morph target
        // (aligned)
        struct Actor_MorphTarget
        {
            float   mRangeMin;          // the slider min
            float   mRangeMax;          // the slider max
            uint32  mLOD;               // the level of detail to which this expression part belongs to
            uint32  mNumTransformations;// the number of transformations to follow
            uint32  mPhonemeSets;       // the number of phoneme sets to follow

            // followed by:
            // string :  morph target name
            // Actor_MorphTargetTransform[ mNumTransformations ]
        };


        // a chunk that contains all morph targets in the file
        // (aligned)
        struct Actor_MorphTargets
        {
            uint32  mNumMorphTargets;   // the number of morph targets to follow
            uint32  mLOD;               // the LOD level the morph targets are for

            // followed by:
            // Actor_MorphTarget[ mNumMorphTargets ]
        };


        // a morph target transformation
        // (aligned)
        struct Actor_MorphTargetTransform
        {
            uint32          mNodeIndex;         // the node name where the transform belongs to
            FileQuaternion  mRotation;          // the node rotation
            FileQuaternion  mScaleRotation;     // the node delta scale rotation
            FileVector3     mPosition;          // the node delta position
            FileVector3     mScale;             // the node delta scale
        };


        // a node group
        // (not aligned)
        struct Actor_NodeGroup
        {
            uint16  mNumNodes;
            uint8   mDisabledOnDefault; // 0 = no, 1 = yes

            // followed by:
            // string : name
            // uint16 [mNumNodes]
        };


        // (aligned)
        struct Actor_Nodes
        {
            uint32          mNumNodes;
            uint32          mNumRootNodes;
            FileVector3     mStaticBoxMin;
            FileVector3     mStaticBoxMax;
            // followed by Actor_Node4[mNumNodes] or Actor_NODE5[mNumNodes] (for v2)
        };

        // node motion sources used for the motion mirroring feature
        // (aligned)
        struct Actor_NodeMotionSources2
        {
            uint32 mNumNodes;
            // followed by uint16[mNumNodes]    // an index per node, which indicates the index of the node to extract the motion data from in case mirroring for a given motion is enabled. This array can be nullptr in case no mirroring data has been setup.
            // followed by uint8[mNumNodes]     // axis identifier (0=X, 1=Y, 2=Z)
            // followed by uint8[mNumNodes]     // flags identifier (see Actor::MirrorFlags)
        };


        // list of node number which are used for attachments
        // (aligned)
        struct Actor_AttachmentNodes
        {
            uint32 mNumNodes;
            // followed by uint16[mNumNodes]    // an index per attachment node
        };
    } // namespace FileFormat
}   // namespace EMotionFX
