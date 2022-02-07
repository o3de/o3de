/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            uint8 m_fourcc[4];   // must be "ACTR"
            uint8 m_hiVersion;   // high version (2  in case of v2.34)
            uint8 m_loVersion;   // low version  (34 in case of v2.34)
            uint8 m_endianType;  // the endian in which the data is saved [0=little, 1=big]
        };


        // (not aligned)
        struct Actor_Info
        {
            uint32  m_numLoDs;               // the number of level of details
            uint32  m_trajectoryNodeIndex;   // the node number of the trajectory node used for motion extraction (NOTE: unused as there is no more trajectory node)
            uint32  m_motionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            float   m_retargetRootOffset;
            uint8   m_unitType;              // maps to EMotionFX::EUnitType
            uint8   m_exporterHighVersion;
            uint8   m_exporterLowVersion;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };


        // (not aligned)
        struct Actor_Info2
        {
            uint32  m_numLoDs;               // the number of level of details
            uint32  m_motionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            uint32  m_retargetRootNodeIndex; // the retargeting root node index, most likely pointing to the hip or pelvis or MCORE_INVALIDINDEX32 when not set
            uint8   m_unitType;              // maps to EMotionFX::EUnitType
            uint8   m_exporterHighVersion;
            uint8   m_exporterLowVersion;

            // followed by:
            // string : source application (e.g. "3ds Max 2011", "Maya 2011")
            // string : original filename of the 3dsMax/Maya file
            // string : compilation date of the exporter
            // string : the name of the actor
        };


        // (aligned)
        struct Actor_Info3
        {
            uint32  m_numLoDs;               // the number of level of details
            uint32  m_motionExtractionNodeIndex;// the node number of the trajectory node used for motion extraction
            uint32  m_retargetRootNodeIndex; // the retargeting root node index, most likely pointing to the hip or pelvis or MCORE_INVALIDINDEX32 when not set
            uint8   m_unitType;              // maps to EMotionFX::EUnitType
            uint8   m_exporterHighVersion;
            uint8   m_exporterLowVersion;
            uint8   m_optimizeSkeleton;

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
        struct Actor_Node2
        {
            FileQuaternion  m_localQuat; // the local rotation (before hierarchy)
            FileVector3     m_localPos;  // the local translation (before hierarchy)
            FileVector3     m_localScale;// the local scale (before hierarchy)
            uint32          m_skeletalLoDs;// each bit representing if the node is active or not, in the give LOD (bit number)
            uint32          m_parentIndex;// parent node number, or 0xFFFFFFFF in case of a root node
            uint32          m_numChilds; // the number of child nodes
            uint8           m_nodeFlags; // #1 bit boolean specifies whether we have to include this node in the bounds calculation or not

            // followed by:
            // string : node name (the unique name of the node)
        };

        // uv (texture) coordinate
        // (aligned)
        struct Actor_UV
        {
            float   m_u;
            float   m_v;
        };

        //-------------------------------------------------------

        // node limit information
        // (aligned)
        struct Actor_Limit
        {
            FileVector3     m_translationMin;// the minimum translation values
            FileVector3     m_translationMax;// the maximum translation value.
            FileVector3     m_rotationMin;   // the minimum rotation values
            FileVector3     m_rotationMax;   // the maximum rotation values
            FileVector3     m_scaleMin;      // the minimum scale values
            FileVector3     m_scaleMax;      // the maximum scale values
            uint8           m_limitFlags[9]; // the limit type activation flags
            uint32          m_nodeNumber;    // the node number where this info belongs to
        };


        // a  morph target
        // (aligned)
        struct Actor_MorphTarget
        {
            float   m_rangeMin;          // the slider min
            float   m_rangeMax;          // the slider max
            uint32  m_lod;               // the level of detail to which this expression part belongs to
            uint32  m_numTransformations;// the number of transformations to follow
            uint32  m_phonemeSets;       // the number of phoneme sets to follow

            // followed by:
            // string :  morph target name
            // Actor_MorphTargetTransform[ m_numTransformations ]
        };


        // a chunk that contains all morph targets in the file
        // (aligned)
        struct Actor_MorphTargets
        {
            uint32  m_numMorphTargets;   // the number of morph targets to follow
            uint32  m_lod;               // the LOD level the morph targets are for

            // followed by:
            // Actor_MorphTarget[ m_numMorphTargets ]
        };


        // a morph target transformation
        // (aligned)
        struct Actor_MorphTargetTransform
        {
            uint32          m_nodeIndex;         // the node name where the transform belongs to
            FileQuaternion  m_rotation;          // the node rotation
            FileQuaternion  m_scaleRotation;     // the node delta scale rotation
            FileVector3     m_position;          // the node delta position
            FileVector3     m_scale;             // the node delta scale
        };


        // a node group
        // (not aligned)
        struct Actor_NodeGroup
        {
            uint16  m_numNodes;
            uint8   m_disabledOnDefault; // 0 = no, 1 = yes

            // followed by:
            // string : name
            // uint16 [m_numNodes]
        };

        // (aligned)
        struct Actor_Nodes2
        {
            uint32          m_numNodes;
            uint32          m_numRootNodes;
            // followed by Actor_Node4[m_numNodes] or Actor_NODE5[m_numNodes] (for v2)
        };

        // node motion sources used for the motion mirroring feature
        // (aligned)
        struct Actor_NodeMotionSources2
        {
            uint32 m_numNodes;
            // followed by uint16[m_numNodes]    // an index per node, which indicates the index of the node to extract the motion data from in case mirroring for a given motion is enabled. This array can be nullptr in case no mirroring data has been setup.
            // followed by uint8[m_numNodes]     // axis identifier (0=X, 1=Y, 2=Z)
            // followed by uint8[m_numNodes]     // flags identifier (see Actor::MirrorFlags)
        };


        // list of node number which are used for attachments
        // (aligned)
        struct Actor_AttachmentNodes
        {
            uint32 m_numNodes;
            // followed by uint16[m_numNodes]    // an index per attachment node
        };
    } // namespace FileFormat
}   // namespace EMotionFX
