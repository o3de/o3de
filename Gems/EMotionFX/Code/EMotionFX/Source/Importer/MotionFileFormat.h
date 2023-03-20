/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the shared structs
#include "SharedFileFormatStructs.h"

#include <AzCore/base.h>

namespace EMotionFX
{
    namespace FileFormat // so now we are in the namespace EMotionFX::FileFormat
    {
        // the chunks
        enum
        {
            MOTION_CHUNK_SUBMOTION             = 200,
            MOTION_CHUNK_INFO                  = 201,
            MOTION_CHUNK_MOTIONEVENTTABLE      = SHARED_CHUNK_MOTIONEVENTTABLE,
            MOTION_CHUNK_SUBMOTIONS            = 202,
            MOTION_CHUNK_MORPHSUBMOTIONS       = 204,
            MOTION_CHUNK_MOTIONDATA            = 210,   // The new motion data system.
            MOTION_CHUNK_ROOTMOTIONEXTRACTION  = 211,   // Root motion extraction settings.
            MOTION_FORCE_32BIT                 = 0xFFFFFFFF
        };

        // motion file header
        // (not aligned)
        struct Motion_Header
        {
            uint8 m_fourcc[4];   // must be "MOT " or "MOTW"
            uint8 m_hiVersion;   // high version (2  in case of v2.34)
            uint8 m_loVersion;   // low version  (34 in case of v2.34)
            uint8 m_endianType;  // the endian in which the data is saved [0=little, 1=big]
        };

        struct Motion_MotionData
        {
            AZ::u32 m_sizeInBytes; // Size of the data to follow.
            AZ::u32 m_dataVersion; // The version of the motion data.
            // Followed by:
            // string: Uuid
            // string: FriendlyName (such as "UniformMotionData")
            // byte[m_sizeInBytes]
        };
        
        // information chunk
        // (not aligned)
        struct Motion_Info
        {
            uint32  m_motionExtractionMask;      // motion extraction mask
            uint32  m_motionExtractionNodeIndex; // motion extraction node index
            uint8   m_unitType;                  // maps to EMotionFX::EUnitType
        };

        // information chunk
        // (not aligned)
        struct Motion_Info2
        {
            uint32  m_motionExtractionFlags;     // motion extraction flags
            uint32  m_motionExtractionNodeIndex; // motion extraction node index
            uint8   m_unitType;                  // maps to EMotionFX::EUnitType
        };

        // information chunk
        // (not aligned)
        struct Motion_Info3
        {
            uint32  m_motionExtractionFlags;     // motion extraction flags
            uint32  m_motionExtractionNodeIndex; // motion extraction node index
            uint8   m_unitType;                  // maps to EMotionFX::EUnitType
            uint8   m_isAdditive;                // if the motion is an additive motion [0=false, 1=true]
        };

        // skeletal submotion
        // (aligned)       
        struct Motion_SkeletalSubMotion
        {
            File16BitQuaternion m_poseRot;       // initial pose rotation
            File16BitQuaternion m_bindPoseRot;   // bind pose rotation
            FileVector3         m_posePos;       // initial pose position
            FileVector3         m_poseScale;     // initial pose scale
            FileVector3         m_bindPosePos;   // bind pose position
            FileVector3         m_bindPoseScale; // bind pose scale
            uint32              m_numPosKeys;    // number of position keyframes to follow
            uint32              m_numRotKeys;    // number of rotation keyframes to follow
            uint32              m_numScaleKeys;  // number of scale keyframes to follow

            // followed by:
            // string : motion part name
            // Motion_Vector3Key[ m_numPosKeys ]
            // Motion_16BitQuaternionKey[ m_numRotKeys ]
            // Motion_Vector3Key[ m_numScaleKeys ]
        };


        // a 3D vector key
        // (aligned)
        struct Motion_Vector3Key
        {
            FileVector3     m_value;     // the value
            float           m_time;      // the time in seconds
        };


        // a quaternion key
        // (aligned)
        struct Motion_QuaternionKey
        {
            FileQuaternion  m_value;     // the value
            float           m_time;      // the time in seconds
        };


        // a 16-bit compressed quaternion key
        // (aligned)
        struct Motion_16BitQuaternionKey
        {
            File16BitQuaternion m_value; // the value
            float               m_time;  // the time in seconds
        };


        // regular submotion header
        // (aligned)
        struct Motion_SubMotions
        {
            uint32  m_numSubMotions;// the number of skeletal motions

            // followed by:
            // Motion_SkeletalSubMotion[ m_numSubMotions ]
        };

        // morph sub motion
        // (aligned)
        struct Motion_MorphSubMotion
        {
            float   m_poseWeight;// pose weight to use in case no animation data is present
            float   m_minWeight; // minimum allowed weight value (used for unpacking the keyframe weights)
            float   m_maxWeight; // maximum allowed weight value (used for unpacking the keyframe weights)
            uint32  m_phonemeSet;// the phoneme set of the submotion, 0 if this is a normal morph target submotion
            uint32  m_numKeys;   // number of keyframes to follow

            // followed by:
            // string : name (the name of this motion part)
            // Motion_UnsignedShortKey[m_numKeys]
        };


        // a uint16 key
        // (not aligned)
        struct Motion_UnsignedShortKey
        {
            float   m_time;  // the time in seconds
            uint16  m_value; // the value
        };


        // (aligned)
        struct Motion_MorphSubMotions
        {
            uint32  m_numSubMotions;
            // followed by:
            // Motion_MorphSubMotion[ m_numSubMotions ]
        };


        // a motion event version 4
        // (not aligned)
        struct FileMotionEvent
        {
            float   m_startTime;
            float   m_endTime;
            uint32  m_eventTypeIndex;// index into the event type string table
            uint32  m_mirrorTypeIndex;// index into the event type string table
            uint16  m_paramIndex;    // index into the parameter string table
        };


        // motion event track
        // (not aligned)
        struct FileMotionEventTrack
        {
            uint32  m_numEvents;
            uint32  m_numTypeStrings;
            uint32  m_numParamStrings;
            uint32  m_numMirrorTypeStrings;
            uint8   m_isEnabled;

            // followed by:
            // String track name
            // [m_numTypeStrings] string objects
            // [m_numParamStrings] string objects
            // [m_numMirrorTypeStrings] string objects
            // FileMotionEvent[m_numEvents]
        };


        // a motion event table
        // (aligned)
        struct FileMotionEventTable
        {
            uint32  m_numTracks;

            // followed by:
            // FileMotionEventTrack[m_numTracks]
        };

        struct FileMotionEventTableSerialized
        {
            /// Use a fixed size to avoid platform-specific issues with size_t
            AZ::u64 m_size;
        };
    } // namespace FileFormat
}   // namespace EMotionFX
