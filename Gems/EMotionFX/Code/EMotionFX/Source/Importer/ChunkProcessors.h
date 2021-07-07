/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "../EMotionFXConfig.h"
#include <MCore/Source/Array.h>
#include <MCore/Source/CompressedQuaternion.h>
#include "../MemoryCategories.h"
#include "../BaseObject.h"
#include "SharedFileFormatStructs.h"
#include "ActorFileFormat.h"
#include "MotionFileFormat.h"
#include "AnimGraphFileFormat.h"
#include "MotionSetFileFormat.h"
#include "NodeMapFileFormat.h"
#include "Importer.h"
#include <AzCore/std/containers/map.h>


namespace EMotionFX
{
    // forward declarations
    class Node;
    class Actor;
    class Motion;
    class Importer;
    class AnimGraphNode;


    /**
     * Shared importer data class.
     * Chunks can load data, which might be shared between other chunks during import.
     * All types of shared data are inherited from this class, and can be added to the importer
     * during import time.
     */
    class EMFX_API SharedData
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Get the unique attribute type.
         * @result The unique attribute ID.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Get rid of the information provided by the shared data.
         */
        virtual void Reset() {}

    protected:
        /**
         * The constructor.
         */
        SharedData()
            : BaseObject() {}

        /**
         * The destructor.
         */
        virtual ~SharedData() { Reset(); }
    };



    /**
     * Helper class for reading strings from files and file information storage.
     */
    class EMFX_API SharedHelperData
        : public SharedData
    {
        MCORE_MEMORYOBJECTCATEGORY(SharedHelperData, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_IMPORTER);

    public:
        // the type returned by GetType()
        enum
        {
            TYPE_ID = 0x00000001
        };

        static SharedHelperData* Create();

        /**
         * Get the unique attribute type.
         * @result The attribute ID.
         */
        uint32 GetType() const      { return TYPE_ID; }

        /**
         * Get rid of the information provided by the shared data.
         */
        void Reset();

        /**
         * Read a string chunk from a file.
         * @param file The stream to read the string chunk from.
         * @param sharedData The array which holds the shared data objects.
         * @param endianType The endian type to read the string in.
         * @return The actual string.
         */
        static const char* ReadString(MCore::Stream* file, MCore::Array<SharedData*>* sharedData, MCore::Endian::EEndianType endianType);

        /**
         * Get the array of currently loaded anim graph nodes.
         * @param sharedData The array which holds the shared data objects.
         * @result The array of currently loaded anim graph nodes.
         */
        static MCore::Array<AnimGraphNode*>& GetBlendNodes(MCore::Array<SharedData*>* sharedData);

        /**
        * Get the table to look up a state machine that needs an entry state as they are created.
        * @param sharedData The array which holds the shared data objects.
        * @result The map whose keys are the indices of entry states and the values are the IDs of the state machines that need those as entry states.
        */
        static AZStd::map<AZ::u64, uint32>& GetEntryStateToStateMachineTable(MCore::Array<SharedData*>* sharedData);

        /**
         * Checks if the strings in the file are encoded using unicode or multi-byte based on the exporter date.
         * The first official EMotion FX version to use unicode was compiled on 26th November 2012.
         */
        static bool GetIsUnicodeFile(const char* dateString, MCore::Array<SharedData*>* sharedData);

    public:
        uint32          mFileHighVersion;           /**< The high file version. For example 3 in case of v3.10. */
        uint32          mFileLowVersion;            /**< The low file version. For example 10 in case of v3.10. */
        uint32          mStringStorageSize;         /**< The size of the string buffer. */
        bool            mIsUnicodeFile;             /**< True in case strings in the file are saved using unicode character set, false in case they are saved using multi-byte. */
        char*           mStringStorage;             /**< The shared string buffer. */
        MCore::Array<AnimGraphNode*> mBlendNodes;  /**< Array of read anim graph nodes. */
        AZStd::map<AZ::u64, uint32>  m_entryNodeIndexToStateMachineIdLookupTable;
    protected:
        /**
         * The constructor.
         */
        SharedHelperData();

        /**
         * The destructor.
         */
        ~SharedHelperData();
    };


    //-----------------------------------------------------------------------------------------


    /**
     * The chunk processor base class.
     * Chunk processors read in a specific chunk, convert them into EMotion FX objects and apply them to either actors
     * or motions. Chunk processors have got a version number, so that there is the possibility of having several
     * processor implementations with different version numbers for one type of chunk. This gives us backward compatibility.
     * Logging can be actived or deactived by functions provided by the base class.
     */
    class EMFX_API ChunkProcessor
        : public BaseObject
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        /**
         * Read and process a chunk. This is the main method.
         * It will return false when we have reached the end of the file or something bad has happened while reading.
         * This is pure virtual method, which has to be overloaded by the processors inherited from this base class.
         * @param file The file from which the processor reads out the chunk.
         & @param importParams The import parameters.
         * @return False when we reached the end of the file, true if not.
         */
        virtual bool Process(MCore::File* file, Importer::ImportParameters& importParams) = 0;

        /**
         * Return the id of the chunk processor.
         * @return The id of the chunk processor.
         */
        uint32 GetChunkID() const;

        /**
         * Return the version number of the chunk processor.
         * @return The version number of the chunk processor.
         */
        uint32 GetVersion() const;

        /**
         * Set the log status.
         * @param The log status. True if the chunk shall log events, false if not.
         */
        void SetLogging(bool loggingActive);

        /**
         * Return the log status.
         * @return The log status. True if the chunk shall log events, false if not.
         */
        bool GetLogging() const;

        /**
         * Convert a Vector3 object.
         * This will convert the coordinate system and the endian type.
         * @param value The vector to convert.
         * @param endianType The endian type in which the current object is stored.
         * @param count The number of items to convert.
         */
        static MCORE_INLINE void ConvertVector3(AZ::Vector3* value, MCore::Endian::EEndianType endianType, uint32 count = 1)
        {
            MCore::Endian::ConvertVector3(value, endianType, count);
        }

        /**
         * Convert a Quaternion object.
         * This will convert the coordinate system and the endian type.
         * @param value The quaternion to convert.
         * @param endianType The endian type in which the current object is stored.
         * @param count The number of items to convert.
         */
        static MCORE_INLINE void ConvertQuaternion(AZ::Quaternion* value, MCore::Endian::EEndianType endianType, uint32 count = 1)
        {
            MCore::Endian::ConvertQuaternion(value, endianType, count);

            // normalize and make sure their w components are positive
            for (uint32 i = 0; i < count; ++i)
            {
                value[i].Normalize();
                if (value[i].GetW() < 0.0f)
                {
                    value[i] = -value[i];
                }
            }
        }

        /**
         * Convert a 16 bit compressed Quaternion object.
         * This will convert the coordinate system and the endian type.
         * @param value The quaternion to convert.
         * @param endianType The endian type in which the current object is stored.
         * @param count The number of items to convert.
         */
        static MCORE_INLINE void Convert16BitQuaternion(MCore::Compressed16BitQuaternion* value, MCore::Endian::EEndianType endianType, uint32 count = 1)
        {
            MCore::Endian::Convert16BitQuaternion(value, endianType, count);

            // rmalize and make sure their w components are positive
            for (uint32 i = 0; i < count; ++i)
            {
                if (value[i].mW < 0)
                {
                    value[i].mX = -value[i].mX;
                    value[i].mY = -value[i].mY;
                    value[i].mZ = -value[i].mZ;
                    value[i].mW = -value[i].mW;
                }
            }
        }

        /**
         * Convert a Vector3 object as scale.
         * This is a bit different from the ConvertVector3, as it will not invert any of the x, y or z components
         * if the coordinate system will require this.
         * This will convert the coordinate system and the endian type.
         * @param value The vector to convert.
         * @param endianType The endian type in which the current object is stored.
         * @param count The number of items to convert.
         */
        static MCORE_INLINE void ConvertScale(AZ::Vector3* value, MCore::Endian::EEndianType endianType, uint32 count = 1)
        {
            MCore::Endian::ConvertVector3(value, endianType, count);
        }

    protected:
        uint32                      mChunkID;       /**< The id of the chunk processor. */
        uint32                      mVersion;       /**< The version number of the chunk processor, to provide backward compatibility. */
        bool                        mLoggingActive; /**< When set to true the processor chunk will log events, otherwise no logging will be performed. */

        /**
         * The constructor.
         * @param chunkID The ID of the chunk processor.
         * @param version The version number of the chunk processor, so what version number of the given chunk
         *                this processor can read and process.
         */
        ChunkProcessor(uint32 chunkID, uint32 version);

        /**
         * Destructor.
         */
        virtual ~ChunkProcessor();
    };


    //-------------------------------------------------------------------------------------------------

    /**
     * The macro to define a chunk processor.
     * After using this macro you only have to implement the Process method.
     */
#define EMFX_CHUNKPROCESSOR(className, chunkID, chunkVersion)                        \
    class EMFX_API className                                                         \
        : public ChunkProcessor                                                      \
    {                                                                                \
    public:                                                                          \
        className()                                                                  \
            : ChunkProcessor(chunkID, chunkVersion) {}                               \
        ~className() {}                                                              \
        bool Process(MCore::File * file, Importer::ImportParameters & importParams); \
    };

    //-------------------------------------------------------------------------------------------------

    // shared file format chunk processors
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionEventTrackTable,   FileFormat::SHARED_CHUNK_MOTIONEVENTTABLE,      1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionEventTrackTable2,  FileFormat::SHARED_CHUNK_MOTIONEVENTTABLE,      2)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionEventTrackTable3,  FileFormat::SHARED_CHUNK_MOTIONEVENTTABLE,      3)

    // Actor file format chunk processors
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorInfo,                 FileFormat::ACTOR_CHUNK_INFO,                 1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorInfo2,                FileFormat::ACTOR_CHUNK_INFO,                 2)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorInfo3,                FileFormat::ACTOR_CHUNK_INFO,                 3)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorProgMorphTarget,      FileFormat::ACTOR_CHUNK_STDPROGMORPHTARGET,   1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorNodeGroups,           FileFormat::ACTOR_CHUNK_NODEGROUPS,           1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorNodes,                FileFormat::ACTOR_CHUNK_NODES,                1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorProgMorphTargets,     FileFormat::ACTOR_CHUNK_STDPMORPHTARGETS,     1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorProgMorphTargets2,    FileFormat::ACTOR_CHUNK_STDPMORPHTARGETS,     2)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorNodeMotionSources,    FileFormat::ACTOR_CHUNK_NODEMOTIONSOURCES,    1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorAttachmentNodes,      FileFormat::ACTOR_CHUNK_ATTACHMENTNODES,      1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorPhysicsSetup,         FileFormat::ACTOR_CHUNK_PHYSICSSETUP,         1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorActorSimulatedObjectSetup, FileFormat::ACTOR_CHUNK_SIMULATEDOBJECTSETUP, 1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMeshAsset,                 FileFormat::ACTOR_CHUNK_MESHASSET,            1)

    // Motion skeletal motion file format chunk processors
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionInfo,                     FileFormat::MOTION_CHUNK_INFO,                 1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionInfo2,                    FileFormat::MOTION_CHUNK_INFO,                 2)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionInfo3,                    FileFormat::MOTION_CHUNK_INFO,                 3)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionSubMotions,               FileFormat::MOTION_CHUNK_SUBMOTIONS,           1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionMorphSubMotions,          FileFormat::MOTION_CHUNK_MORPHSUBMOTIONS,      1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionData,                     FileFormat::MOTION_CHUNK_MOTIONDATA,           1)

    // Anim graph file format chunk processors
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphStateTransitions,         FileFormat::ANIMGRAPH_CHUNK_STATETRANSITIONS,         1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphNodeConnections,          FileFormat::ANIMGRAPH_CHUNK_NODECONNECTIONS,          1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphParameters,               FileFormat::ANIMGRAPH_CHUNK_PARAMETERS,               1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphNodeGroups,               FileFormat::ANIMGRAPH_CHUNK_NODEGROUPS,               1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphGroupParameters,          FileFormat::ANIMGRAPH_CHUNK_GROUPPARAMETERS,          1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphGameControllerSettings,   FileFormat::ANIMGRAPH_CHUNK_GAMECONTROLLERSETTINGS,   1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphNode,                     FileFormat::ANIMGRAPH_CHUNK_BLENDNODE,                1)
    EMFX_CHUNKPROCESSOR(ChunkProcessorAnimGraphAdditionalInfo,           FileFormat::ANIMGRAPH_CHUNK_ADDITIONALINFO,           1)

    // motion set file format chunk processors
    EMFX_CHUNKPROCESSOR(ChunkProcessorMotionSet,                   FileFormat::CHUNK_MOTIONSET,    1)

    // node map file format chunk processors
    EMFX_CHUNKPROCESSOR(ChunkProcessorNodeMap,                     FileFormat::CHUNK_NODEMAP,      1)


    //-------------------------------------------------------------------------------------------------
} // namespace EMotionFX
