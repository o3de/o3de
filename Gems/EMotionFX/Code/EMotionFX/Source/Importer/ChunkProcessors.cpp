/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/Distance.h>
#include <MCore/Source/File.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/MCoreSystem.h>
#include "ChunkProcessors.h"
#include "Importer.h"
#include "../EventManager.h"
#include "../Node.h"
#include "../NodeGroup.h"
#include "../Motion.h"
#include "../MotionEventTrack.h"
#include "../MotionSet.h"
#include "../MotionManager.h"
#include "../Actor.h"
#include "../MorphTarget.h"
#include "../MorphSetup.h"
#include "../MorphTargetStandard.h"
#include "../MotionEventTable.h"
#include "../Skeleton.h"
#include "../AnimGraph.h"
#include "../AnimGraphManager.h"
#include "../AnimGraphObjectFactory.h"
#include "../AnimGraphNode.h"
#include "../AnimGraphNodeGroup.h"
#include "../BlendTree.h"
#include "../AnimGraphStateMachine.h"
#include "../AnimGraphStateTransition.h"
#include "../AnimGraphTransitionCondition.h"
#include "../MCore/Source/Endian.h"
#include "../NodeMap.h"

#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Source/MotionData/MotionDataFactory.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SharedData, ImporterAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SharedHelperData, ImporterAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ChunkProcessor, ImporterAllocator)

    bool ForwardFullAttribute(MCore::File* file, MCore::Endian::EEndianType endianType)
    {
        // read the attribute type
        uint32 attributeType;
        if (file->Read(&attributeType, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&attributeType, endianType);

        // read the attribute size
        uint32 attributeSize;
        if (file->Read(&attributeSize, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&attributeSize, endianType);

        uint32 numCharacters;
        if (file->Read(&numCharacters, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        if (numCharacters && !file->Forward(numCharacters))
        {
            return false;
        }
        if (attributeSize && !file->Forward(attributeSize))
        {
            return false;
        }

        return true;
    }

    bool ForwardAttributeSettings(MCore::File* file, MCore::Endian::EEndianType endianType)
    {
        // read the version of the attribute settings format
        uint8 version;
        if (file->Read(&version, sizeof(uint8)) != sizeof(uint8))
        {
            return false;
        }

        if (version == 2)
        {
            // read the flags (new in version 2)
            if (!file->Forward(sizeof(uint16)))
            {
                return false;
            }
        }
        if (version == 1 || version == 2)
        {
            // read the internal name
            uint32 numChars;
            if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
            if (numChars && !file->Forward(numChars))
            {
                return false;
            }
            // read name
            if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
            if (numChars && !file->Forward(numChars)) // name
            {
                return false;
            }
            // read the description
            if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
            if (numChars && !file->Forward(numChars))
            {
                return false;
            }
            // interface type
            if (!file->Forward(sizeof(uint32)))
            {
                return false;
            }
            // read the number of combobox values
            uint32 numComboValues;
            if (file->Read(&numComboValues, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numComboValues, endianType);
            // read the combo strings
            for (uint32 i = 0; i < numComboValues; ++i)
            {
                if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
                {
                    return false;
                }
                MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
                if (numChars && !file->Forward(numChars))
                {
                    return false;
                }
            }
            // full attributes means that it saves the type, size, version and its data
            // the default value
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
            // the minimum value
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
            // the maximum value
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
        }
        else
        {
            AZ_Assert(false, "Unknown attribute version");
            return false;
        }

        return true;
    }

    bool ForwardAttributes(MCore::File* file, MCore::Endian::EEndianType endianType, uint32 numAttributes, bool hasAttributeSettings = false)
    {
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (hasAttributeSettings && !ForwardAttributeSettings(file, endianType))
            {
                return false;
            }
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
        }
        return true;
    }

    bool ForwardAttributeSet(MCore::File* file, MCore::Endian::EEndianType endianType)
    {
        // version
        if (!file->Forward(sizeof(uint8)))
        {
            return false;
        }
        uint32 numAttributes;
        if (file->Read(&numAttributes, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numAttributes, endianType);
        return ForwardAttributes(file, endianType, numAttributes, true);
    }


    // constructor
    SharedHelperData::SharedHelperData()
    {
        m_fileHighVersion    = 1;
        m_fileLowVersion     = 0;

        // allocate the string buffer used for reading in variable sized strings
        m_stringStorageSize  = 256;
        m_stringStorage      = (char*)MCore::Allocate(m_stringStorageSize, EMFX_MEMCATEGORY_IMPORTER);
    }


    // destructor
    SharedHelperData::~SharedHelperData()
    {
        Reset();
    }


    // creation
    SharedHelperData* SharedHelperData::Create()
    {
        return new SharedHelperData();
    }


    // reset the shared data
    void SharedHelperData::Reset()
    {
        // free the string buffer
        if (m_stringStorage)
        {
            MCore::Free(m_stringStorage);
        }

        m_stringStorage = nullptr;
        m_stringStorageSize = 0;
    }

    const char* SharedHelperData::ReadString(MCore::Stream* file, AZStd::vector<SharedData*>* sharedData, MCore::Endian::EEndianType endianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(sharedData);

        // find the helper data
        SharedData* data                = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData    = static_cast<SharedHelperData*>(data);

        // get the size of the string (number of characters)
        uint32 numCharacters;
        file->Read(&numCharacters, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);

        // if we need to enlarge the buffer
        if (helperData->m_stringStorageSize < numCharacters + 1)
        {
            helperData->m_stringStorageSize = numCharacters + 1;
            helperData->m_stringStorage = (char*)MCore::Realloc(helperData->m_stringStorage, helperData->m_stringStorageSize, EMFX_MEMCATEGORY_IMPORTER);
        }

        // receive the actual string
        file->Read(helperData->m_stringStorage, numCharacters * sizeof(uint8));
        helperData->m_stringStorage[numCharacters] = '\0';

        return helperData->m_stringStorage;
    }

    //-----------------------------------------------------------------------------

    // constructor
    ChunkProcessor::ChunkProcessor(uint32 chunkID, uint32 version)
        : MCore::RefCounted()
    {
        m_chunkId        = chunkID;
        m_version        = version;
        m_loggingActive  = false;
    }


    // destructor
    ChunkProcessor::~ChunkProcessor()
    {
    }


    uint32 ChunkProcessor::GetChunkID() const
    {
        return m_chunkId;
    }


    uint32 ChunkProcessor::GetVersion() const
    {
        return m_version;
    }


    void ChunkProcessor::SetLogging(bool loggingActive)
    {
        m_loggingActive = loggingActive;
    }


    bool ChunkProcessor::GetLogging() const
    {
        return m_loggingActive;
    }

    //=================================================================================================

    bool ChunkProcessorActorNodes2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;
        Importer::ActorSettings* actorSettings = importParams.m_actorSettings;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        FileFormat::Actor_Nodes2 nodesHeader;
        file->Read(&nodesHeader, sizeof(FileFormat::Actor_Nodes2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodesHeader.m_numNodes, endianType);
        MCore::Endian::ConvertUnsignedInt32(&nodesHeader.m_numRootNodes, endianType);

        // pre-allocate space for the nodes
        actor->SetNumNodes(nodesHeader.m_numNodes);

        // pre-allocate space for the root nodes
        skeleton->ReserveRootNodes(nodesHeader.m_numRootNodes);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Nodes: %d (%d root nodes)", nodesHeader.m_numNodes, nodesHeader.m_numRootNodes);
        }

        // add the transform
        actor->ResizeTransformData();

        // read all nodes
        for (uint32 n = 0; n < nodesHeader.m_numNodes; ++n)
        {
            // read the node header
            FileFormat::Actor_Node2 nodeChunk;
            file->Read(&nodeChunk, sizeof(FileFormat::Actor_Node2));

            // read the node name
            const char* nodeName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.m_parentIndex, endianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.m_skeletalLoDs, endianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.m_numChilds, endianType);

            // show the name of the node, the parent and the number of children
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Node name = '%s'", nodeName);
                MCore::LogDetailedInfo("     - Parent = '%s'", (nodeChunk.m_parentIndex != MCORE_INVALIDINDEX32) ? skeleton->GetNode(nodeChunk.m_parentIndex)->GetName() : "");
                MCore::LogDetailedInfo("     - NumChild Nodes = %d", nodeChunk.m_numChilds);
            }

            // create the new node
            Node* node = Node::Create(nodeName, skeleton);

            // set the node index
            const uint32 nodeIndex = n;
            node->SetNodeIndex(nodeIndex);

            // pre-allocate space for the number of child nodes
            node->PreAllocNumChildNodes(nodeChunk.m_numChilds);

            // add it to the actor
            skeleton->SetNode(n, node);

            // create Core objects from the data
            AZ::Vector3 pos(nodeChunk.m_localPos.m_x, nodeChunk.m_localPos.m_y, nodeChunk.m_localPos.m_z);
            AZ::Vector3 scale(nodeChunk.m_localScale.m_x, nodeChunk.m_localScale.m_y, nodeChunk.m_localScale.m_z);
            AZ::Quaternion rot(nodeChunk.m_localQuat.m_x, nodeChunk.m_localQuat.m_y, nodeChunk.m_localQuat.m_z, nodeChunk.m_localQuat.m_w);

            // convert endian and coordinate system
            ConvertVector3(&pos, endianType);
            ConvertScale(&scale, endianType);
            ConvertQuaternion(&rot, endianType);

            // set the local transform
            Transform bindTransform;
            bindTransform.m_position     = pos;
            bindTransform.m_rotation     = rot.GetNormalized();
            EMFX_SCALECODE
            (
                bindTransform.m_scale    = scale;
            )

            actor->GetBindPose()->SetLocalSpaceTransform(nodeIndex, bindTransform);

            // set the skeletal LOD levels
            if (actorSettings->m_loadSkeletalLoDs)
            {
                node->SetSkeletalLODLevelBits(nodeChunk.m_skeletalLoDs);
            }

            // set if this node has to be taken into the bounding volume calculation
            const bool includeInBoundsCalc = (nodeChunk.m_nodeFlags & Node::ENodeFlags::FLAG_INCLUDEINBOUNDSCALC); // first bit
            node->SetIncludeInBoundsCalc(includeInBoundsCalc);

            // Set if this node is critical and cannot be optimized out.
            const bool isCritical = (nodeChunk.m_nodeFlags & Node::ENodeFlags::FLAG_CRITICAL); // third bit
            node->SetIsCritical(isCritical);

            // set the parent, and add this node as child inside the parent
            if (nodeChunk.m_parentIndex != MCORE_INVALIDINDEX32) // if this node has a parent and the parent node is valid
            {
                if (nodeChunk.m_parentIndex < n)
                {
                    node->SetParentIndex(nodeChunk.m_parentIndex);
                    Node* parentNode = skeleton->GetNode(nodeChunk.m_parentIndex);
                    parentNode->AddChild(nodeIndex);
                }
                else
                {
                    MCore::LogError("Cannot assign parent node index (%d) for node '%s' as the parent node is not yet loaded. Making '%s' a root node.", nodeChunk.m_parentIndex, node->GetName(), node->GetName());
                    skeleton->AddRootNode(nodeIndex);
                }
            }
            else // if this node has no parent, so when it is a root node
            {
                skeleton->AddRootNode(nodeIndex);
            }

            if (GetLogging())
            {
                MCore::LogDetailedInfo("      - Position:      x=%f, y=%f, z=%f",
                    static_cast<float>(pos.GetX()),
                    static_cast<float>(pos.GetY()),
                    static_cast<float>(pos.GetZ()));
                MCore::LogDetailedInfo("      - Rotation:      x=%f, y=%f, z=%f, w=%f",
                    static_cast<float>(rot.GetX()),
                    static_cast<float>(rot.GetY()),
                    static_cast<float>(rot.GetZ()),
                    static_cast<float>(rot.GetW()));
                MCore::LogDetailedInfo("      - Scale:         x=%f, y=%f, z=%f",
                    static_cast<float>(scale.GetX()),
                    static_cast<float>(scale.GetY()),
                    static_cast<float>(scale.GetZ()));
                MCore::LogDetailedInfo("      - IncludeInBoundsCalc: %d", includeInBoundsCalc);
            }
        } // for all nodes

        return true;
    }

    //=================================================================================================

    // read all submotions in one chunk
    bool ChunkProcessorMotionSubMotions::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Motion* motion = importParams.m_motion;
        AZ_Assert(motion, "Expected a valid motion object.");

        // read the header
        FileFormat::Motion_SubMotions subMotionsHeader;
        file->Read(&subMotionsHeader, sizeof(FileFormat::Motion_SubMotions));
        MCore::Endian::ConvertUnsignedInt32(&subMotionsHeader.m_numSubMotions, endianType);

        // Create a uniform motion data.
        NonUniformMotionData* motionData = aznew NonUniformMotionData();
        motion->SetMotionData(motionData);
        motionData->Resize(subMotionsHeader.m_numSubMotions, motionData->GetNumMorphs(), motionData->GetNumFloats());

        // for all submotions
        for (uint32 s = 0; s < subMotionsHeader.m_numSubMotions; ++s)
        {
            FileFormat::Motion_SkeletalSubMotion fileSubMotion;
            file->Read(&fileSubMotion, sizeof(FileFormat::Motion_SkeletalSubMotion));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.m_numPosKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.m_numRotKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.m_numScaleKeys, endianType);

            // read the motion part name
            const char* motionJointName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            // convert into Core objects
            AZ::Vector3 posePos(fileSubMotion.m_posePos.m_x, fileSubMotion.m_posePos.m_y, fileSubMotion.m_posePos.m_z);
            AZ::Vector3 poseScale(fileSubMotion.m_poseScale.m_x, fileSubMotion.m_poseScale.m_y, fileSubMotion.m_poseScale.m_z);
            MCore::Compressed16BitQuaternion poseRot(fileSubMotion.m_poseRot.m_x, fileSubMotion.m_poseRot.m_y, fileSubMotion.m_poseRot.m_z, fileSubMotion.m_poseRot.m_w);

            AZ::Vector3 bindPosePos(fileSubMotion.m_bindPosePos.m_x, fileSubMotion.m_bindPosePos.m_y, fileSubMotion.m_bindPosePos.m_z);
            AZ::Vector3 bindPoseScale(fileSubMotion.m_bindPoseScale.m_x, fileSubMotion.m_bindPoseScale.m_y, fileSubMotion.m_bindPoseScale.m_z);
            MCore::Compressed16BitQuaternion bindPoseRot(fileSubMotion.m_bindPoseRot.m_x, fileSubMotion.m_bindPoseRot.m_y, fileSubMotion.m_bindPoseRot.m_z, fileSubMotion.m_bindPoseRot.m_w);

            // convert endian and coordinate system
            ConvertVector3(&posePos, endianType);
            ConvertVector3(&bindPosePos, endianType);
            ConvertScale(&poseScale, endianType);
            ConvertScale(&bindPoseScale, endianType);
            Convert16BitQuaternion(&poseRot, endianType);
            Convert16BitQuaternion(&bindPoseRot, endianType);

            if (GetLogging())
            {
                const AZ::Quaternion uncompressedPoseRot = poseRot.ToQuaternion().GetNormalized();
                const AZ::Quaternion uncompressedBindPoseRot = bindPoseRot.ToQuaternion().GetNormalized();

                MCore::LogDetailedInfo("- Motion Joint = '%s'", motionJointName);
                MCore::LogDetailedInfo("    + Pose Position:         x=%f, y=%f, z=%f",
                    static_cast<float>(posePos.GetX()),
                    static_cast<float>(posePos.GetY()),
                    static_cast<float>(posePos.GetZ()));
                MCore::LogDetailedInfo("    + Pose Rotation:         x=%f, y=%f, z=%f, w=%f",
                    static_cast<float>(uncompressedPoseRot.GetX()),
                    static_cast<float>(uncompressedPoseRot.GetY()),
                    static_cast<float>(uncompressedPoseRot.GetZ()),
                    static_cast<float>(uncompressedPoseRot.GetW()));
                MCore::LogDetailedInfo("    + Pose Scale:            x=%f, y=%f, z=%f",
                    static_cast<float>(poseScale.GetX()),
                    static_cast<float>(poseScale.GetY()),
                    static_cast<float>(poseScale.GetZ()));
                MCore::LogDetailedInfo("    + Bind Pose Position:    x=%f, y=%f, z=%f",
                    static_cast<float>(bindPosePos.GetX()),
                    static_cast<float>(bindPosePos.GetY()),
                    static_cast<float>(bindPosePos.GetZ()));
                MCore::LogDetailedInfo("    + Bind Pose Rotation:    x=%f, y=%f, z=%f, w=%f",
                    static_cast<float>(uncompressedBindPoseRot.GetX()),
                    static_cast<float>(uncompressedBindPoseRot.GetY()),
                    static_cast<float>(uncompressedBindPoseRot.GetZ()),
                    static_cast<float>(uncompressedBindPoseRot.GetW()));
                MCore::LogDetailedInfo("    + Bind Pose Scale:       x=%f, y=%f, z=%f",
                    static_cast<float>(bindPoseScale.GetX()),
                    static_cast<float>(bindPoseScale.GetY()),
                    static_cast<float>(bindPoseScale.GetZ()));
                MCore::LogDetailedInfo("    + Num Pos Keys:          %d", fileSubMotion.m_numPosKeys);
                MCore::LogDetailedInfo("    + Num Rot Keys:          %d", fileSubMotion.m_numRotKeys);
                MCore::LogDetailedInfo("    + Num Scale Keys:        %d", fileSubMotion.m_numScaleKeys);
            }

            motionData->SetJointName(s, motionJointName);
            motionData->SetJointStaticPosition(s, posePos);
            motionData->SetJointStaticRotation(s, poseRot.ToQuaternion().GetNormalized());
            motionData->SetJointBindPosePosition(s, bindPosePos);
            motionData->SetJointBindPoseRotation(s, bindPoseRot.ToQuaternion().GetNormalized());
            EMFX_SCALECODE
            (
                motionData->SetJointStaticScale(s, poseScale);
                motionData->SetJointBindPoseScale(s, bindPoseScale);
            )

            // now read the animation data
            uint32 i;
            if (fileSubMotion.m_numPosKeys > 0)
            {
                motionData->AllocateJointPositionSamples(s, fileSubMotion.m_numPosKeys);
                for (i = 0; i < fileSubMotion.m_numPosKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));

                    MCore::Endian::ConvertFloat(&key.m_time, endianType);
                    AZ::Vector3 pos(key.m_value.m_x, key.m_value.m_y, key.m_value.m_z);
                    ConvertVector3(&pos, endianType);

                    motionData->SetJointPositionSample(s, i, {key.m_time, pos});
                }
            }

            // now the rotation keys
            if (fileSubMotion.m_numRotKeys > 0)
            {
                motionData->AllocateJointRotationSamples(s, fileSubMotion.m_numRotKeys);
                for (i = 0; i < fileSubMotion.m_numRotKeys; ++i)
                {
                    FileFormat::Motion_16BitQuaternionKey key;
                    file->Read(&key, sizeof(FileFormat::Motion_16BitQuaternionKey));

                    MCore::Endian::ConvertFloat(&key.m_time, endianType);
                    MCore::Compressed16BitQuaternion rot(key.m_value.m_x, key.m_value.m_y, key.m_value.m_z, key.m_value.m_w);
                    Convert16BitQuaternion(&rot, endianType);

                    motionData->SetJointRotationSample(s, i, {key.m_time, rot.ToQuaternion().GetNormalized()});
                }
            }


    #ifndef EMFX_SCALE_DISABLED
            // and the scale keys
            if (fileSubMotion.m_numScaleKeys > 0)
            {
                motionData->AllocateJointScaleSamples(s, fileSubMotion.m_numScaleKeys);
                for (i = 0; i < fileSubMotion.m_numScaleKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));

                    MCore::Endian::ConvertFloat(&key.m_time, endianType);
                    AZ::Vector3 scale(key.m_value.m_x, key.m_value.m_y, key.m_value.m_z);
                    ConvertScale(&scale, endianType);

                    motionData->SetJointScaleSample(s, i, {key.m_time, scale});
                }
            }
    #else   // no scaling
            // and the scale keys
            if (fileSubMotion.m_numScaleKeys > 0)
            {
                for (i = 0; i < fileSubMotion.m_numScaleKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));
                }
            }
    #endif
        } // for all submotions

        motion->UpdateDuration();
        AZ_Assert(motion->GetMotionData()->VerifyIntegrity(), "Data integrity issue in animation '%s'.", motion->GetName());
        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionInfo::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Motion* motion = importParams.m_motion;

        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionMask, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.m_unitType);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.m_unitType));
        motion->SetFileUnitType(motion->GetUnitType());


        // Try to remain backward compatible by still capturing height when this was enabled in the old mask system.
        if (fileInformation.m_motionExtractionMask & (1 << 2))   // The 1<<2 was the mask used for position Z in the old motion extraction mask settings
        {
            motion->SetMotionExtractionFlags(MOTIONEXTRACT_CAPTURE_Z);
        }

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionInfo2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Motion* motion = importParams.m_motion;
        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info2 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionFlags, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.m_unitType);
            MCore::LogDetailedInfo("   + Motion Extraction Flags       = 0x%x [capZ=%d]", fileInformation.m_motionExtractionFlags, (fileInformation.m_motionExtractionFlags & EMotionFX::MOTIONEXTRACT_CAPTURE_Z) ? 1 : 0);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.m_unitType));
        motion->SetFileUnitType(motion->GetUnitType());
        motion->SetMotionExtractionFlags(static_cast<EMotionFX::EMotionExtractionFlags>(fileInformation.m_motionExtractionFlags));

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionInfo3::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Motion* motion = importParams.m_motion;
        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info3 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info3));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionFlags, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.m_unitType);
            MCore::LogDetailedInfo("   + Is Additive Motion            = %d", fileInformation.m_isAdditive);
            MCore::LogDetailedInfo("   + Motion Extraction Flags       = 0x%x [capZ=%d]", fileInformation.m_motionExtractionFlags, (fileInformation.m_motionExtractionFlags & EMotionFX::MOTIONEXTRACT_CAPTURE_Z) ? 1 : 0);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.m_unitType));
        importParams.m_additiveMotion = (fileInformation.m_isAdditive == 0 ? false : true);
        motion->SetFileUnitType(motion->GetUnitType());
        motion->SetMotionExtractionFlags(static_cast<EMotionFX::EMotionExtractionFlags>(fileInformation.m_motionExtractionFlags));

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorPhysicsSetup::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        AZ::u32 bufferSize;
        file->Read(&bufferSize, sizeof(AZ::u32));
        MCore::Endian::ConvertUnsignedInt32(&bufferSize, endianType);

        AZStd::vector<AZ::u8> buffer;
        buffer.resize(bufferSize);
        file->Read(&buffer[0], bufferSize);

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return false;
        }

        AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        EMotionFX::PhysicsSetup* resultPhysicsSetup = AZ::Utils::LoadObjectFromBuffer<EMotionFX::PhysicsSetup>(buffer.data(), buffer.size(), serializeContext, loadFilter);

        if (resultPhysicsSetup)
        {
            if (importParams.m_actorSettings->m_optimizeForServer)
            {
                resultPhysicsSetup->OptimizeForServer();
            }
            actor->SetPhysicsSetup(AZStd::shared_ptr<EMotionFX::PhysicsSetup>(resultPhysicsSetup));
        }

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorSimulatedObjectSetup::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        AZ::u32 bufferSize;
        file->Read(&bufferSize, sizeof(AZ::u32));
        MCore::Endian::ConvertUnsignedInt32(&bufferSize, endianType);

        AZStd::vector<AZ::u8> buffer;
        buffer.resize(bufferSize);
        file->Read(&buffer[0], bufferSize);

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return false;
        }

        AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        EMotionFX::SimulatedObjectSetup* resultSimulatedObjectSetup = AZ::Utils::LoadObjectFromBuffer<EMotionFX::SimulatedObjectSetup>(buffer.data(), buffer.size(), serializeContext, loadFilter);
        if (resultSimulatedObjectSetup)
        {
            actor->SetSimulatedObjectSetup(AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>(resultSimulatedObjectSetup));
        }

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMeshAsset::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;
        AZ_Assert(actor, "Actor needs to be valid.");

        EMotionFX::FileFormat::Actor_MeshAsset meshAssetChunk;
        file->Read(&meshAssetChunk, sizeof(FileFormat::Actor_MeshAsset));
        const char* meshAssetIdString = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        const AZ::Data::AssetId meshAssetId = AZ::Data::AssetId::CreateString(meshAssetIdString);
        if (meshAssetId.IsValid())
        {
            actor->SetMeshAssetId(meshAssetId);
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("    - Mesh asset");
            MCore::LogDetailedInfo("       + AssetId  = %s", meshAssetIdString);
        }

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionEventTrackTable::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Motion* motion = importParams.m_motion;

        MCORE_ASSERT(motion);

        // read the motion event table header
        FileFormat::FileMotionEventTable fileEventTable;
        file->Read(&fileEventTable, sizeof(FileFormat::FileMotionEventTable));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileEventTable.m_numTracks, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Motion Event Table:");
            MCore::LogDetailedInfo("  + Num Tracks = %d", fileEventTable.m_numTracks);
        }

        // get the motion event table the reserve the event tracks
        MotionEventTable* motionEventTable = motion->GetEventTable();
        motionEventTable->ReserveNumTracks(fileEventTable.m_numTracks);

        // read all tracks
        AZStd::string trackName;
        AZStd::vector<AZStd::string> typeStrings;
        AZStd::vector<AZStd::string> paramStrings;
        AZStd::vector<AZStd::string> mirrorTypeStrings;
        for (uint32 t = 0; t < fileEventTable.m_numTracks; ++t)
        {
            // read the motion event table header
            FileFormat::FileMotionEventTrack fileTrack;
            file->Read(&fileTrack, sizeof(FileFormat::FileMotionEventTrack));

            // read the track name
            trackName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.m_numEvents,             endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.m_numTypeStrings,        endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.m_numParamStrings,       endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.m_numMirrorTypeStrings,  endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Motion Event Track:");
                MCore::LogDetailedInfo("   + Name       = %s", trackName.c_str());
                MCore::LogDetailedInfo("   + Num events = %d", fileTrack.m_numEvents);
                MCore::LogDetailedInfo("   + Num types  = %d", fileTrack.m_numTypeStrings);
                MCore::LogDetailedInfo("   + Num params = %d", fileTrack.m_numParamStrings);
                MCore::LogDetailedInfo("   + Num mirror = %d", fileTrack.m_numMirrorTypeStrings);
                MCore::LogDetailedInfo("   + Enabled    = %d", fileTrack.m_isEnabled);
            }

            // the even type and parameter strings
            typeStrings.resize(fileTrack.m_numTypeStrings);
            paramStrings.resize(fileTrack.m_numParamStrings);
            mirrorTypeStrings.resize(fileTrack.m_numMirrorTypeStrings);

            // read all type strings
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Event types:");
            }
            uint32 i;
            for (i = 0; i < fileTrack.m_numTypeStrings; ++i)
            {
                typeStrings[i] = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, typeStrings[i].c_str());
                }
            }

            // read all param strings
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Parameters:");
            }
            for (i = 0; i < fileTrack.m_numParamStrings; ++i)
            {
                paramStrings[i] = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, paramStrings[i].c_str());
                }
            }

            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Mirror Type Strings:");
            }
            for (i = 0; i < fileTrack.m_numMirrorTypeStrings; ++i)
            {
                mirrorTypeStrings[i] = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, mirrorTypeStrings[i].c_str());
                }
            }

            // create the default event track
            MotionEventTrack* track = MotionEventTrack::Create(trackName.c_str(), motion);
            track->SetIsEnabled(fileTrack.m_isEnabled != 0);
            track->ReserveNumEvents(fileTrack.m_numEvents);
            motionEventTable->AddTrack(track);

            // read all motion events
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Motion Events:");
            }
            for (i = 0; i < fileTrack.m_numEvents; ++i)
            {
                // read the event header
                FileFormat::FileMotionEvent fileEvent;
                file->Read(&fileEvent, sizeof(FileFormat::FileMotionEvent));

                // convert endian
                MCore::Endian::ConvertUnsignedInt32(&fileEvent.m_eventTypeIndex, endianType);
                MCore::Endian::ConvertUnsignedInt16(&fileEvent.m_paramIndex, endianType);
                MCore::Endian::ConvertUnsignedInt32(&fileEvent.m_mirrorTypeIndex, endianType);
                MCore::Endian::ConvertFloat(&fileEvent.m_startTime, endianType);
                MCore::Endian::ConvertFloat(&fileEvent.m_endTime, endianType);

                // print motion event information
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] StartTime = %f  -  EndTime = %f  -  Type = '%s'  -  Param = '%s'  -  Mirror = '%s'", i, fileEvent.m_startTime, fileEvent.m_endTime, typeStrings[fileEvent.m_eventTypeIndex].c_str(), paramStrings[fileEvent.m_paramIndex].c_str(), mirrorTypeStrings[fileEvent.m_mirrorTypeIndex].c_str());
                }

                const AZStd::string eventTypeName = fileEvent.m_eventTypeIndex != MCORE_INVALIDINDEX32 ?
                    typeStrings[fileEvent.m_eventTypeIndex] : "";
                const AZStd::string mirrorTypeName = fileEvent.m_mirrorTypeIndex != MCORE_INVALIDINDEX32 ?
                    mirrorTypeStrings[fileEvent.m_mirrorTypeIndex] : "";
                const AZStd::string params = paramStrings[fileEvent.m_paramIndex];

                // add the event
                track->AddEvent(
                    fileEvent.m_startTime,
                    fileEvent.m_endTime,
                    GetEventManager().FindOrCreateEventData<EMotionFX::TwoStringEventData>(eventTypeName, params, mirrorTypeName)
                    );
            }
        } // for all tracks

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionEventTrackTable2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        Motion* motion = importParams.m_motion;

        MCORE_ASSERT(motion);

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            return false;
        }

        // read the motion event table header
        FileFormat::FileMotionEventTableSerialized fileEventTable;
        file->Read(&fileEventTable, sizeof(FileFormat::FileMotionEventTableSerialized));

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Motion Event Table:");
            MCore::LogDetailedInfo("  + size = %d", fileEventTable.m_size);
        }

        AZStd::vector<char> buffer(fileEventTable.m_size);
        file->Read(&buffer[0], fileEventTable.m_size);

        auto motionEventTable = AZStd::unique_ptr<MotionEventTable>(AZ::Utils::LoadObjectFromBuffer<MotionEventTable>(&buffer[0], buffer.size(), context));
        if (motionEventTable)
        {
            motion->SetEventTable(AZStd::move(motionEventTable));
            motion->GetEventTable()->InitAfterLoading(motion);
            return true;
        }

        return false;
    }

    //=================================================================================================

    bool ChunkProcessorMotionEventTrackTable3::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        Motion* motion = importParams.m_motion;
        MCORE_ASSERT(motion);

        FileFormat::FileMotionEventTableSerialized fileEventTable;
        file->Read(&fileEventTable, sizeof(FileFormat::FileMotionEventTableSerialized));

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Motion Event Table:");
            MCore::LogDetailedInfo("  + size = %d", fileEventTable.m_size);
        }

        AZStd::vector<char> buffer(fileEventTable.m_size);
        file->Read(&buffer[0], fileEventTable.m_size);
        AZStd::string_view bufferStringView(&buffer[0], buffer.size());

        auto readJsonOutcome = AZ::JsonSerializationUtils::ReadJsonString(bufferStringView);
        AZStd::string errorMsg;
        if (!readJsonOutcome.IsSuccess())
        {
            AZ_Error("EMotionFX", false, "Loading motion event table failed due to ReadJsonString. %s", readJsonOutcome.TakeError().c_str());
            return false;
        }
        rapidjson::Document document = readJsonOutcome.TakeValue();

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            return false;
        }

        AZ::JsonDeserializerSettings settings;
        settings.m_serializeContext = context;

        MotionEventTable* motionEventTable = motion->GetEventTable();
        AZ::JsonSerializationResult::ResultCode jsonResult = AZ::JsonSerialization::Load(*motionEventTable, document, settings);
        if (jsonResult.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            AZ_Error("EMotionFX", false, "Loading motion event table failed due to AZ::JsonSerialization::Load.");
            return false;
        }

        motionEventTable->InitAfterLoading(motion);
        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorInfo::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        MCORE_ASSERT(actor);

        // read the chunk
        FileFormat::Actor_Info fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_trajectoryNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_numLoDs, endianType);
        MCore::Endian::ConvertFloat(&fileInformation.m_retargetRootOffset, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

        const char* name = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        actor->SetName(name);
        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
        }

        // print motion event information
        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.m_exporterHighVersion, fileInformation.m_exporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.m_numLoDs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.m_motionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root offset   = %f", fileInformation.m_retargetRootOffset);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.m_unitType);
        }

        actor->SetMotionExtractionNodeIndex(fileInformation.m_motionExtractionNodeIndex);
        if (fileInformation.m_motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            actor->SetMotionExtractionNodeIndex(fileInformation.m_motionExtractionNodeIndex);
        }
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.m_unitType));
        actor->SetFileUnitType(actor->GetUnitType());

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorInfo2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        // read the chunk
        FileFormat::Actor_Info2 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_retargetRootNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_numLoDs, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

        const char* name = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        actor->SetName(name);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.m_exporterHighVersion, fileInformation.m_exporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.m_numLoDs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.m_motionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root node     = %d", fileInformation.m_retargetRootNodeIndex);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.m_unitType);
        }

        if (fileInformation.m_motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            actor->SetMotionExtractionNodeIndex(fileInformation.m_motionExtractionNodeIndex);
        }
        if (fileInformation.m_retargetRootNodeIndex != MCORE_INVALIDINDEX32)
        {
            actor->SetRetargetRootNodeIndex(fileInformation.m_retargetRootNodeIndex);
        }
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.m_unitType));
        actor->SetFileUnitType(actor->GetUnitType());

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorInfo3::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        // read the chunk
        FileFormat::Actor_Info3 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info3));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_motionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_retargetRootNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.m_numLoDs, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

        const char* name = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
        actor->SetName(name);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.m_exporterHighVersion, fileInformation.m_exporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.m_numLoDs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.m_motionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root node     = %d", fileInformation.m_retargetRootNodeIndex);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.m_unitType);
        }

        if (fileInformation.m_motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            actor->SetMotionExtractionNodeIndex(fileInformation.m_motionExtractionNodeIndex);
        }
        if (fileInformation.m_retargetRootNodeIndex != MCORE_INVALIDINDEX32)
        {
            actor->SetRetargetRootNodeIndex(fileInformation.m_retargetRootNodeIndex);
        }
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.m_unitType));
        actor->SetFileUnitType(actor->GetUnitType());
        actor->SetOptimizeSkeleton(fileInformation.m_optimizeSkeleton == 0? false : true);

        return true;
    }

    //=================================================================================================

    // morph targets
    bool ChunkProcessorActorProgMorphTarget::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the expression part from disk
        FileFormat::Actor_MorphTarget morphTargetChunk;
        file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

        // convert endian
        MCore::Endian::ConvertFloat(&morphTargetChunk.m_rangeMin, endianType);
        MCore::Endian::ConvertFloat(&morphTargetChunk.m_rangeMax, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_lod, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_numTransformations, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_phonemeSets, endianType);

        // get the expression name
        const char* morphTargetName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

        // get the level of detail of the expression part
        const uint32 morphTargetLOD = morphTargetChunk.m_lod;

        if (GetLogging())
        {
            MCore::LogDetailedInfo(" - Morph Target:");
            MCore::LogDetailedInfo("    + Name               = '%s'", morphTargetName);
            MCore::LogDetailedInfo("    + LOD Level          = %d", morphTargetChunk.m_lod);
            MCore::LogDetailedInfo("    + RangeMin           = %f", morphTargetChunk.m_rangeMin);
            MCore::LogDetailedInfo("    + RangeMax           = %f", morphTargetChunk.m_rangeMax);
            MCore::LogDetailedInfo("    + NumTransformations = %d", morphTargetChunk.m_numTransformations);
            MCore::LogDetailedInfo("    + PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.m_phonemeSets).c_str());
        }

        // check if the morph setup has already been created, if not create it
        if (actor->GetMorphSetup(morphTargetLOD) == nullptr)
        {
            // create the morph setup
            MorphSetup* morphSetup = MorphSetup::Create();

            // set the morph setup
            actor->SetMorphSetup(morphTargetLOD, morphSetup);
        }

        // create the morph target
        MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphTargetName);

        // set the slider range
        morphTarget->SetRangeMin(morphTargetChunk.m_rangeMin);
        morphTarget->SetRangeMax(morphTargetChunk.m_rangeMax);

        // set the phoneme sets
        morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.m_phonemeSets);

        // add the morph target
        actor->GetMorphSetup(morphTargetLOD)->AddMorphTarget(morphTarget);

        // read the facial transformations
        for (uint32 i = 0; i < morphTargetChunk.m_numTransformations; ++i)
        {
            // read the facial transformation from disk
            FileFormat::Actor_MorphTargetTransform transformChunk;
            file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

            // create Core objects from the data
            AZ::Vector3 pos(transformChunk.m_position.m_x, transformChunk.m_position.m_y, transformChunk.m_position.m_z);
            AZ::Vector3 scale(transformChunk.m_scale.m_x, transformChunk.m_scale.m_y, transformChunk.m_scale.m_z);
            AZ::Quaternion rot(transformChunk.m_rotation.m_x, transformChunk.m_rotation.m_y, transformChunk.m_rotation.m_z, transformChunk.m_rotation.m_w);
            AZ::Quaternion scaleRot(transformChunk.m_scaleRotation.m_x, transformChunk.m_scaleRotation.m_y, transformChunk.m_scaleRotation.m_z, transformChunk.m_scaleRotation.m_w);

            // convert endian and coordinate system
            ConvertVector3(&pos, endianType);
            ConvertScale(&scale, endianType);
            ConvertQuaternion(&rot, endianType);
            ConvertQuaternion(&scaleRot, endianType);
            MCore::Endian::ConvertUnsignedInt32(&transformChunk.m_nodeIndex, endianType);

            // create our transformation
            MorphTargetStandard::Transformation transform;
            transform.m_position         = pos;
            transform.m_scale            = scale;
            transform.m_rotation         = rot;
            transform.m_scaleRotation    = scaleRot;
            transform.m_nodeIndex        = transformChunk.m_nodeIndex;

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    - Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.m_nodeIndex)->GetName(), transform.m_nodeIndex);
                MCore::LogDetailedInfo("       + Pos:      %f, %f, %f",
                    static_cast<float>(transform.m_position.GetX()),
                    static_cast<float>(transform.m_position.GetY()),
                    static_cast<float>(transform.m_position.GetZ()));
                MCore::LogDetailedInfo("       + Rotation: %f, %f, %f %f",
                    static_cast<float>(transform.m_rotation.GetX()),
                    static_cast<float>(transform.m_rotation.GetY()),
                    static_cast<float>(transform.m_rotation.GetZ()),
                    static_cast<float>(transform.m_rotation.GetW()));
                MCore::LogDetailedInfo("       + Scale:    %f, %f, %f",
                    static_cast<float>(transform.m_scale.GetX()),
                    static_cast<float>(transform.m_scale.GetY()),
                    static_cast<float>(transform.m_scale.GetZ()));
                MCore::LogDetailedInfo("       + ScaleRot: %f, %f, %f %f",
                    static_cast<float>(scaleRot.GetX()),
                    static_cast<float>(scaleRot.GetY()),
                    static_cast<float>(scaleRot.GetZ()),
                    static_cast<float>(scaleRot.GetW()));
            }

            // add the transformation to the bones expression part
            morphTarget->AddTransformation(transform);
        }

        return true;
    }

    //-----------------

    // the node groups chunk
    bool ChunkProcessorActorNodeGroups::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        MCORE_ASSERT(actor);

        // read the number of groups to follow
        uint16 numGroups;
        file->Read(&numGroups, sizeof(uint16));
        MCore::Endian::ConvertUnsignedInt16(&numGroups, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node groups: %d", numGroups);
        }

        // read all groups
        for (uint32 i = 0; i < numGroups; ++i)
        {
            // read the group header
            FileFormat::Actor_NodeGroup fileGroup;
            file->Read(&fileGroup, sizeof(FileFormat::Actor_NodeGroup));
            MCore::Endian::ConvertUnsignedInt16(&fileGroup.m_numNodes, endianType);

            // read the group name
            const char* groupName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            // log some info
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Group '%s'", groupName);
                MCore::LogDetailedInfo("     - Num nodes: %d", fileGroup.m_numNodes);
                MCore::LogDetailedInfo("     - Disabled on default: %s", fileGroup.m_disabledOnDefault ? "Yes" : "No");
            }

            // create the new group inside the actor
            NodeGroup* newGroup = aznew NodeGroup(groupName, fileGroup.m_numNodes, fileGroup.m_disabledOnDefault ? false : true);

            // read the node numbers
            uint16 nodeIndex;
            for (uint16 n = 0; n < fileGroup.m_numNodes; ++n)
            {
                file->Read(&nodeIndex, sizeof(uint16));
                MCore::Endian::ConvertUnsignedInt16(&nodeIndex, endianType);
                newGroup->SetNode(n, nodeIndex);
            }

            // add the group to the actor
            actor->AddNodeGroup(newGroup);
        }

        return true;
    }


    // all submotions in one chunk
    bool ChunkProcessorMotionMorphSubMotions::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Motion* motion = importParams.m_motion;
        AZ_Assert(motion, "Expecting a valid motion pointer.");

        // cast to  morph motion
        AZ_Assert(motion->GetMotionData(), "Expecting to have motion data allocated.");
        NonUniformMotionData* motionData = azdynamic_cast<NonUniformMotionData*>(motion->GetMotionData());
        AZ_Assert(motionData, "Expected motion data to be of non-uniform motion data type.");

        FileFormat::Motion_MorphSubMotions subMotionsHeader;
        file->Read(&subMotionsHeader, sizeof(FileFormat::Motion_MorphSubMotions));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&subMotionsHeader.m_numSubMotions, endianType);

        // pre-allocate the number of submotions
        motionData->SetAdditive(importParams.m_additiveMotion);
        motionData->Resize(motionData->GetNumJoints(), subMotionsHeader.m_numSubMotions, motionData->GetNumFloats());

        // for all submotions
        for (uint32 s = 0; s < subMotionsHeader.m_numSubMotions; ++s)
        {
            // get the  morph motion part
            FileFormat::Motion_MorphSubMotion morphSubMotionChunk;
            file->Read(&morphSubMotionChunk, sizeof(FileFormat::Motion_MorphSubMotion));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&morphSubMotionChunk.m_numKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphSubMotionChunk.m_phonemeSet, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.m_poseWeight, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.m_minWeight, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.m_maxWeight, endianType);

            // read the name of the submotion
            const char* name = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            motionData->SetMorphName(s, name);
            motionData->AllocateMorphSamples(s, morphSubMotionChunk.m_numKeys);
            motionData->SetMorphStaticValue(s, morphSubMotionChunk.m_poseWeight);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    - Morph Submotion: %s", name);
                MCore::LogDetailedInfo("       + NrKeys             = %d", morphSubMotionChunk.m_numKeys);
                MCore::LogDetailedInfo("       + Pose Weight        = %f", morphSubMotionChunk.m_poseWeight);
                MCore::LogDetailedInfo("       + Minimum Weight     = %f", morphSubMotionChunk.m_minWeight);
                MCore::LogDetailedInfo("       + Maximum Weight     = %f", morphSubMotionChunk.m_maxWeight);
                MCore::LogDetailedInfo("       + PhonemeSet         = %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphSubMotionChunk.m_phonemeSet).c_str());
            }

            // add keyframes
            for (uint32 i = 0; i < morphSubMotionChunk.m_numKeys; ++i)
            {
                FileFormat::Motion_UnsignedShortKey keyframeChunk;

                file->Read(&keyframeChunk, sizeof(FileFormat::Motion_UnsignedShortKey));
                MCore::Endian::ConvertFloat(&keyframeChunk.m_time, endianType);
                MCore::Endian::ConvertUnsignedInt16(&keyframeChunk.m_value, endianType);

                const float value = keyframeChunk.m_value / static_cast<float>(std::numeric_limits<uint16_t>::max());
                motionData->SetMorphSample(s, i, {keyframeChunk.m_time, value});
            }
        } // for all submotions

        motion->UpdateDuration();
        AZ_Assert(motion->GetMotionData()->VerifyIntegrity(), "Data integrity issue in animation '%s'.", motion->GetName());
        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // morph targets
    bool ChunkProcessorActorProgMorphTargets::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the header
        FileFormat::Actor_MorphTargets morphTargetsHeader;
        file->Read(&morphTargetsHeader, sizeof(FileFormat::Actor_MorphTargets));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.m_numMorphTargets, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.m_lod, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Morph targets: %d (LOD=%d)", morphTargetsHeader.m_numMorphTargets, morphTargetsHeader.m_lod);
        }

        // check if the morph setup has already been created, if not create it
        if (actor->GetMorphSetup(morphTargetsHeader.m_lod) == nullptr)
        {
            // create the morph setup
            MorphSetup* morphSetup = MorphSetup::Create();

            // set the morph setup
            actor->SetMorphSetup(morphTargetsHeader.m_lod, morphSetup);
        }

        // pre-allocate the morph targets
        MorphSetup* setup = actor->GetMorphSetup(morphTargetsHeader.m_lod);
        setup->ReserveMorphTargets(morphTargetsHeader.m_numMorphTargets);

        // read in all morph targets
        for (uint32 mt = 0; mt < morphTargetsHeader.m_numMorphTargets; ++mt)
        {
            // read the expression part from disk
            FileFormat::Actor_MorphTarget morphTargetChunk;
            file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

            // convert endian
            MCore::Endian::ConvertFloat(&morphTargetChunk.m_rangeMin, endianType);
            MCore::Endian::ConvertFloat(&morphTargetChunk.m_rangeMax, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_lod, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_numTransformations, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_phonemeSets, endianType);

            // make sure they match
            MCORE_ASSERT(morphTargetChunk.m_lod == morphTargetsHeader.m_lod);

            // get the expression name
            const char* morphTargetName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + Morph Target:");
                MCore::LogDetailedInfo("     - Name               = '%s'", morphTargetName);
                MCore::LogDetailedInfo("     - LOD Level          = %d", morphTargetChunk.m_lod);
                MCore::LogDetailedInfo("     - RangeMin           = %f", morphTargetChunk.m_rangeMin);
                MCore::LogDetailedInfo("     - RangeMax           = %f", morphTargetChunk.m_rangeMax);
                MCore::LogDetailedInfo("     - NumTransformations = %d", morphTargetChunk.m_numTransformations);
                MCore::LogDetailedInfo("     - PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.m_phonemeSets).c_str());
            }

            // create the morph target
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphTargetName);

            // set the slider range
            morphTarget->SetRangeMin(morphTargetChunk.m_rangeMin);
            morphTarget->SetRangeMax(morphTargetChunk.m_rangeMax);

            // set the phoneme sets
            morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.m_phonemeSets);

            // add the morph target
            setup->AddMorphTarget(morphTarget);

            // the same for the transformations
            morphTarget->ReserveTransformations(morphTargetChunk.m_numTransformations);

            // read the facial transformations
            for (uint32 i = 0; i < morphTargetChunk.m_numTransformations; ++i)
            {
                // read the facial transformation from disk
                FileFormat::Actor_MorphTargetTransform transformChunk;
                file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

                // create Core objects from the data
                AZ::Vector3 pos(transformChunk.m_position.m_x, transformChunk.m_position.m_y, transformChunk.m_position.m_z);
                AZ::Vector3 scale(transformChunk.m_scale.m_x, transformChunk.m_scale.m_y, transformChunk.m_scale.m_z);
                AZ::Quaternion rot(transformChunk.m_rotation.m_x, transformChunk.m_rotation.m_y, transformChunk.m_rotation.m_z, transformChunk.m_rotation.m_w);
                AZ::Quaternion scaleRot(transformChunk.m_scaleRotation.m_x, transformChunk.m_scaleRotation.m_y, transformChunk.m_scaleRotation.m_z, transformChunk.m_scaleRotation.m_w);

                // convert endian and coordinate system
                ConvertVector3(&pos, endianType);
                ConvertScale(&scale, endianType);
                ConvertQuaternion(&rot, endianType);
                ConvertQuaternion(&scaleRot, endianType);
                MCore::Endian::ConvertUnsignedInt32(&transformChunk.m_nodeIndex, endianType);

                // create our transformation
                MorphTargetStandard::Transformation transform;
                transform.m_position         = pos;
                transform.m_scale            = scale;
                transform.m_rotation         = rot;
                transform.m_scaleRotation    = scaleRot;
                transform.m_nodeIndex        = transformChunk.m_nodeIndex;

                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     + Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.m_nodeIndex)->GetName(), transform.m_nodeIndex);
                    MCore::LogDetailedInfo("        - Pos:      %f, %f, %f",
                        static_cast<float>(transform.m_position.GetX()),
                        static_cast<float>(transform.m_position.GetY()),
                        static_cast<float>(transform.m_position.GetZ()));
                    MCore::LogDetailedInfo("        - Rotation: %f, %f, %f %f",
                        static_cast<float>(transform.m_rotation.GetX()),
                        static_cast<float>(transform.m_rotation.GetY()),
                        static_cast<float>(transform.m_rotation.GetZ()),
                        static_cast<float>(transform.m_rotation.GetW()));
                    MCore::LogDetailedInfo("        - Scale:    %f, %f, %f",
                        static_cast<float>(transform.m_scale.GetX()),
                        static_cast<float>(transform.m_scale.GetY()),
                        static_cast<float>(transform.m_scale.GetZ()));
                    MCore::LogDetailedInfo("        - ScaleRot: %f, %f, %f %f",
                        static_cast<float>(scaleRot.GetX()),
                        static_cast<float>(scaleRot.GetY()),
                        static_cast<float>(scaleRot.GetZ()),
                        static_cast<float>(scaleRot.GetW()));
                }

                // add the transformation to the bones expression part
                morphTarget->AddTransformation(transform);
            }
        } // for all morph targets

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // morph targets
    bool ChunkProcessorActorProgMorphTargets2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the header
        FileFormat::Actor_MorphTargets morphTargetsHeader;
        file->Read(&morphTargetsHeader, sizeof(FileFormat::Actor_MorphTargets));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.m_numMorphTargets, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.m_lod, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Morph targets: %d (LOD=%d)", morphTargetsHeader.m_numMorphTargets, morphTargetsHeader.m_lod);
        }

        // check if the morph setup has already been created, if not create it
        if (actor->GetMorphSetup(morphTargetsHeader.m_lod) == nullptr)
        {
            // create the morph setup
            MorphSetup* morphSetup = MorphSetup::Create();

            // set the morph setup
            actor->SetMorphSetup(morphTargetsHeader.m_lod, morphSetup);
        }

        // pre-allocate the morph targets
        MorphSetup* setup = actor->GetMorphSetup(morphTargetsHeader.m_lod);
        setup->ReserveMorphTargets(morphTargetsHeader.m_numMorphTargets);

        // read in all morph targets
        for (uint32 mt = 0; mt < morphTargetsHeader.m_numMorphTargets; ++mt)
        {
            // read the expression part from disk
            FileFormat::Actor_MorphTarget morphTargetChunk;
            file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

            // convert endian
            MCore::Endian::ConvertFloat(&morphTargetChunk.m_rangeMin, endianType);
            MCore::Endian::ConvertFloat(&morphTargetChunk.m_rangeMax, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_lod, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_numTransformations, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.m_phonemeSets, endianType);

            // make sure they match
            MCORE_ASSERT(morphTargetChunk.m_lod == morphTargetsHeader.m_lod);

            // get the expression name
            const char* morphTargetName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + Morph Target:");
                MCore::LogDetailedInfo("     - Name               = '%s'", morphTargetName);
                MCore::LogDetailedInfo("     - LOD Level          = %d", morphTargetChunk.m_lod);
                MCore::LogDetailedInfo("     - RangeMin           = %f", morphTargetChunk.m_rangeMin);
                MCore::LogDetailedInfo("     - RangeMax           = %f", morphTargetChunk.m_rangeMax);
                MCore::LogDetailedInfo("     - NumTransformations = %d", morphTargetChunk.m_numTransformations);
                MCore::LogDetailedInfo("     - PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.m_phonemeSets).c_str());
            }

            // create the morph target
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphTargetName);

            // set the slider range
            morphTarget->SetRangeMin(morphTargetChunk.m_rangeMin);
            morphTarget->SetRangeMax(morphTargetChunk.m_rangeMax);

            // set the phoneme sets
            morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.m_phonemeSets);

            // add the morph target
            setup->AddMorphTarget(morphTarget);

            // the same for the transformations
            morphTarget->ReserveTransformations(morphTargetChunk.m_numTransformations);

            // read the facial transformations
            for (uint32 i = 0; i < morphTargetChunk.m_numTransformations; ++i)
            {
                // read the facial transformation from disk
                FileFormat::Actor_MorphTargetTransform transformChunk;
                file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

                // create Core objects from the data
                AZ::Vector3 pos(transformChunk.m_position.m_x, transformChunk.m_position.m_y, transformChunk.m_position.m_z);
                AZ::Vector3 scale(transformChunk.m_scale.m_x, transformChunk.m_scale.m_y, transformChunk.m_scale.m_z);
                AZ::Quaternion rot(transformChunk.m_rotation.m_x, transformChunk.m_rotation.m_y, transformChunk.m_rotation.m_z, transformChunk.m_rotation.m_w);
                AZ::Quaternion scaleRot(transformChunk.m_scaleRotation.m_x, transformChunk.m_scaleRotation.m_y, transformChunk.m_scaleRotation.m_z, transformChunk.m_scaleRotation.m_w);

                // convert endian and coordinate system
                ConvertVector3(&pos, endianType);
                ConvertScale(&scale, endianType);
                ConvertQuaternion(&rot, endianType);
                ConvertQuaternion(&scaleRot, endianType);
                MCore::Endian::ConvertUnsignedInt32(&transformChunk.m_nodeIndex, endianType);

                // create our transformation
                MorphTargetStandard::Transformation transform;
                transform.m_position         = pos;
                transform.m_scale            = scale;
                transform.m_rotation         = rot;
                transform.m_scaleRotation    = scaleRot;
                transform.m_nodeIndex        = transformChunk.m_nodeIndex;

                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     + Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.m_nodeIndex)->GetName(), transform.m_nodeIndex);
                    MCore::LogDetailedInfo("        - Pos:      %f, %f, %f",
                        static_cast<float>(transform.m_position.GetX()),
                        static_cast<float>(transform.m_position.GetY()),
                        static_cast<float>(transform.m_position.GetZ()));
                    MCore::LogDetailedInfo("        - Rotation: %f, %f, %f %f",
                        static_cast<float>(transform.m_rotation.GetX()),
                        static_cast<float>(transform.m_rotation.GetY()),
                        static_cast<float>(transform.m_rotation.GetZ()),
                        static_cast<float>(transform.m_rotation.GetW()));
                    MCore::LogDetailedInfo("        - Scale:    %f, %f, %f",
                        static_cast<float>(transform.m_scale.GetX()),
                        static_cast<float>(transform.m_scale.GetY()),
                        static_cast<float>(transform.m_scale.GetZ()));
                    MCore::LogDetailedInfo("        - ScaleRot: %f, %f, %f %f",
                        static_cast<float>(scaleRot.GetX()),
                        static_cast<float>(scaleRot.GetY()),
                        static_cast<float>(scaleRot.GetZ()),
                        static_cast<float>(scaleRot.GetW()));

                }

                // add the transformation to the bones expression part
                morphTarget->AddTransformation(transform);
            }
        } // for all morph targets

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // the node motion sources chunk
    bool ChunkProcessorActorNodeMotionSources::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        uint32 i;
        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the file data
        FileFormat::Actor_NodeMotionSources2 nodeMotionSourcesChunk;
        file->Read(&nodeMotionSourcesChunk, sizeof(FileFormat::Actor_NodeMotionSources2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeMotionSourcesChunk.m_numNodes, endianType);
        const uint32 numNodes = nodeMotionSourcesChunk.m_numNodes;
        if (numNodes == 0)
        {
            return true;
        }

        // allocate the node motion sources array and recheck the number of nodes with the
        // information given in this chunk
        MCORE_ASSERT(actor->GetNumNodes() == numNodes);
        actor->AllocateNodeMirrorInfos();

        // read all node motion sources and convert endian
        for (i = 0; i < numNodes; ++i)
        {
            uint16 sourceNode;
            file->Read(&sourceNode, sizeof(uint16));
            MCore::Endian::ConvertUnsignedInt16(&sourceNode, endianType);
            actor->GetNodeMirrorInfo(i).m_sourceNode = sourceNode;
        }

        // read all axes
        for (i = 0; i < numNodes; ++i)
        {
            uint8 axis;
            file->Read(&axis, sizeof(uint8));
            actor->GetNodeMirrorInfo(i).m_axis = axis;
        }

        // read all flags
        for (i = 0; i < numNodes; ++i)
        {
            uint8 flags;
            file->Read(&flags, sizeof(uint8));
            actor->GetNodeMirrorInfo(i).m_flags = flags;
        }

        // log details
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node Motion Sources (%i):", numNodes);
            for (i = 0; i < numNodes; ++i)
            {
                if (actor->GetNodeMirrorInfo(i).m_sourceNode != MCORE_INVALIDINDEX16)
                {
                    MCore::LogDetailedInfo("   + '%s' (%i) -> '%s' (%i) [axis=%d] [flags=%d]", skeleton->GetNode(i)->GetName(), i, skeleton->GetNode(actor->GetNodeMirrorInfo(i).m_sourceNode)->GetName(), actor->GetNodeMirrorInfo(i).m_sourceNode, actor->GetNodeMirrorInfo(i).m_axis, actor->GetNodeMirrorInfo(i).m_flags);
                }
            }
        }

        return true;
    }


    //----------------------------------------------------------------------------------------------------------

    bool ChunkProcessorActorAttachmentNodes::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        Actor* actor = importParams.m_actor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the file data
        FileFormat::Actor_AttachmentNodes attachmentNodesChunk;
        file->Read(&attachmentNodesChunk, sizeof(FileFormat::Actor_AttachmentNodes));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&attachmentNodesChunk.m_numNodes, endianType);
        const uint32 numAttachmentNodes = attachmentNodesChunk.m_numNodes;

        // read all node attachment nodes
        for (uint32 i = 0; i < numAttachmentNodes; ++i)
        {
            // get the attachment node index and endian convert it
            uint16 nodeNr;
            file->Read(&nodeNr, sizeof(uint16));
            MCore::Endian::ConvertUnsignedInt16(&nodeNr, endianType);

            // get the attachment node from the actor
            MCORE_ASSERT(nodeNr < actor->GetNumNodes());
            Node* node = skeleton->GetNode(nodeNr);

            // enable the attachment node flag
            node->SetIsAttachmentNode(true);
        }

        // log details
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Attachment Nodes (%i):", numAttachmentNodes);

            const size_t numNodes = actor->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                // get the current node
                Node* node = skeleton->GetNode(i);

                // only log the attachment nodes
                if (node->GetIsAttachmentNode())
                {
                    MCore::LogDetailedInfo("   + '%s' (%zu)", node->GetName(), node->GetNodeIndex());
                }
            }
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------
    // NodeMap
    //----------------------------------------------------------------------------------------------------------

    // node map
    bool ChunkProcessorNodeMap::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;

        // read the header
        FileFormat::NodeMapChunk nodeMapChunk;
        file->Read(&nodeMapChunk, sizeof(FileFormat::NodeMapChunk));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeMapChunk.m_numEntries, endianType);

        // load the source actor filename string, but discard it
        SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

        // log some info
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node Map:");
            MCore::LogDetailedInfo("  + Num entries = %d", nodeMapChunk.m_numEntries);
        }

        // for all entries
        const uint32 numEntries = nodeMapChunk.m_numEntries;
        importParams.m_nodeMap->Reserve(numEntries);
        AZStd::string firstName;
        AZStd::string secondName;
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // read both names
            firstName  = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);
            secondName = SharedHelperData::ReadString(file, importParams.m_sharedData, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + [%d] '%s' -> '%s'", i, firstName.c_str(), secondName.c_str());
            }

            // create the entry
            if (importParams.m_nodeMapSettings->m_loadNodes)
            {
                importParams.m_nodeMap->AddEntry(firstName.c_str(), secondName.c_str());
            }
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------
    // MotionData
    //----------------------------------------------------------------------------------------------------------
    bool ChunkProcessorMotionData::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        // Read the header.
        FileFormat::Motion_MotionData dataHeader;
        if (file->Read(&dataHeader, sizeof(FileFormat::Motion_MotionData)) == 0)
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&dataHeader.m_sizeInBytes, importParams.m_endianType);
        MCore::Endian::ConvertUnsignedInt32(&dataHeader.m_dataVersion, importParams.m_endianType);

        // Read the strings.
        const AZStd::string uuidString = SharedHelperData::ReadString(file, importParams.m_sharedData, importParams.m_endianType);
        const AZStd::string className = SharedHelperData::ReadString(file, importParams.m_sharedData, importParams.m_endianType);

        // Create the motion data of this type.
        const AZ::Uuid uuid = AZ::Uuid::CreateString(uuidString.c_str(), uuidString.size());
        MotionData* motionData = GetMotionManager().GetMotionDataFactory().Create(uuid);

        // Check if we could create it.
        if (!motionData)
        {
            AZ_Assert(false, "Unsupported motion data type '%s' using uuid '%s'", className.c_str(), uuidString.c_str());
            motionData = aznew UniformMotionData(); // Create an empty dummy motion data, so we don't break things.
            importParams.m_motion->SetMotionData(motionData);
            file->Forward(dataHeader.m_sizeInBytes);
            return false;
        }

        // Read the data.
        MotionData::ReadSettings readSettings;
        readSettings.m_sourceEndianType = importParams.m_endianType;
        readSettings.m_logDetails = GetLogging();
        readSettings.m_version = dataHeader.m_dataVersion;
        if (!motionData->Read(file, readSettings))
        {
            AZ_Error("EMotionFX", false, "Failed to load motion data of type '%s'", className.c_str());
            motionData = aznew UniformMotionData(); // Create an empty dummy motion data, so we don't break things.
            importParams.m_motion->SetMotionData(motionData);
            return false;
        }

        importParams.m_motion->SetMotionData(motionData);
        return true;
    }

    //----------------------------------------------------------------------------------------------------------
    // RootMotionExtraction
    //----------------------------------------------------------------------------------------------------------
    bool ChunkProcessorRootMotionExtraction::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return false;
        }

        AZ::u32 bufferSize;
        file->Read(&bufferSize, sizeof(bufferSize));
        MCore::Endian::ConvertUnsignedInt32(&bufferSize, importParams.m_endianType);

        AZStd::vector<AZ::u8> buffer;
        buffer.resize(bufferSize);
        file->Read(&buffer[0], bufferSize);

        // Read root motion extraction
        AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        EMotionFX::RootMotionExtractionData* resultRootMotionExtractionData =
            AZ::Utils::LoadObjectFromBuffer<EMotionFX::RootMotionExtractionData>(
                buffer.data(), buffer.size(), serializeContext, loadFilter);
        if (resultRootMotionExtractionData)
        {
            importParams.m_motion->SetRootMotionExtractionData(
                AZStd::shared_ptr<EMotionFX::RootMotionExtractionData>(resultRootMotionExtractionData));
        }

        return true;
    }

} // namespace EMotionFX
