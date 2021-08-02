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
#include <AzFramework/FileFunc/FileFunc.h>

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
#include "../AnimGraphGameControllerSettings.h"
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
    AZ_CLASS_ALLOCATOR_IMPL(SharedData, ImporterAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ChunkProcessor, ImporterAllocator, 0)

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
        mFileHighVersion    = 1;
        mFileLowVersion     = 0;

        // allocate the string buffer used for reading in variable sized strings
        mStringStorageSize  = 256;
        mStringStorage      = (char*)MCore::Allocate(mStringStorageSize, EMFX_MEMCATEGORY_IMPORTER);
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
        if (mStringStorage)
        {
            MCore::Free(mStringStorage);
        }

        mStringStorage = nullptr;
        mStringStorageSize = 0;
    }

    const char* SharedHelperData::ReadString(MCore::Stream* file, MCore::Array<SharedData*>* sharedData, MCore::Endian::EEndianType endianType)
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
        if (helperData->mStringStorageSize < numCharacters + 1)
        {
            helperData->mStringStorageSize = numCharacters + 1;
            helperData->mStringStorage = (char*)MCore::Realloc(helperData->mStringStorage, helperData->mStringStorageSize, EMFX_MEMCATEGORY_IMPORTER);
        }

        // receive the actual string
        file->Read(helperData->mStringStorage, numCharacters * sizeof(uint8));
        helperData->mStringStorage[numCharacters] = '\0';

        //if (helperData->mIsUnicodeFile)
        //helperData->mConvertString = helperData->mStringStorage;
        //else
        //          helperData->mConvertString = helperData->mStringStorage;

        //result = helperData->mStringStorage;
        return helperData->mStringStorage;
    }

    //-----------------------------------------------------------------------------

    // constructor
    ChunkProcessor::ChunkProcessor(uint32 chunkID, uint32 version)
        : BaseObject()
    {
        mChunkID        = chunkID;
        mVersion        = version;
        mLoggingActive  = false;
    }


    // destructor
    ChunkProcessor::~ChunkProcessor()
    {
    }


    uint32 ChunkProcessor::GetChunkID() const
    {
        return mChunkID;
    }


    uint32 ChunkProcessor::GetVersion() const
    {
        return mVersion;
    }


    void ChunkProcessor::SetLogging(bool loggingActive)
    {
        mLoggingActive = loggingActive;
    }


    bool ChunkProcessor::GetLogging() const
    {
        return mLoggingActive;
    }


    //=================================================================================================


    // a chunk that contains all nodes in one chunk
    bool ChunkProcessorActorNodes::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        Importer::ActorSettings* actorSettings = importParams.mActorSettings;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        FileFormat::Actor_Nodes nodesHeader;
        file->Read(&nodesHeader, sizeof(FileFormat::Actor_Nodes));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodesHeader.mNumNodes, endianType);
        MCore::Endian::ConvertUnsignedInt32(&nodesHeader.mNumRootNodes, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMin.mX, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMin.mY, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMin.mZ, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMax.mX, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMax.mY, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMax.mZ, endianType);

        // convert endian and coord system of the static box
        AZ::Vector3 boxMin(nodesHeader.mStaticBoxMin.mX, nodesHeader.mStaticBoxMin.mY, nodesHeader.mStaticBoxMin.mZ);
        AZ::Vector3 boxMax(nodesHeader.mStaticBoxMax.mX, nodesHeader.mStaticBoxMax.mY, nodesHeader.mStaticBoxMax.mZ);

        // build the box and set it
        MCore::AABB staticBox;
        staticBox.SetMin(boxMin);
        staticBox.SetMax(boxMax);
        actor->SetStaticAABB(staticBox);

        // pre-allocate space for the nodes
        actor->SetNumNodes(nodesHeader.mNumNodes);

        // pre-allocate space for the root nodes
        skeleton->ReserveRootNodes(nodesHeader.mNumRootNodes);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Nodes: %d (%d root nodes)", nodesHeader.mNumNodes, nodesHeader.mNumRootNodes);
        }

        // add the transform
        actor->ResizeTransformData();

        // read all nodes
        for (uint32 n = 0; n < nodesHeader.mNumNodes; ++n)
        {
            // read the node header
            FileFormat::Actor_Node nodeChunk;
            file->Read(&nodeChunk, sizeof(FileFormat::Actor_Node));

            // read the node name
            const char* nodeName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.mParentIndex, endianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.mSkeletalLODs, endianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.mNumChilds, endianType);
            MCore::Endian::ConvertFloat(&nodeChunk.mOBB[0], endianType, 16);

            // show the name of the node, the parent and the number of children
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Node name = '%s'", nodeName);
                MCore::LogDetailedInfo("     - Parent = '%s'", (nodeChunk.mParentIndex != MCORE_INVALIDINDEX32) ? skeleton->GetNode(nodeChunk.mParentIndex)->GetName() : "");
                MCore::LogDetailedInfo("     - NumChild Nodes = %d", nodeChunk.mNumChilds);
            }

            // create the new node
            Node* node = Node::Create(nodeName, skeleton);

            // set the node index
            const uint32 nodeIndex = n;
            node->SetNodeIndex(nodeIndex);

            // pre-allocate space for the number of child nodes
            node->PreAllocNumChildNodes(nodeChunk.mNumChilds);

            // add it to the actor
            skeleton->SetNode(n, node);

            // create Core objects from the data
            AZ::Vector3 pos(nodeChunk.mLocalPos.mX, nodeChunk.mLocalPos.mY, nodeChunk.mLocalPos.mZ);
            AZ::Vector3 scale(nodeChunk.mLocalScale.mX, nodeChunk.mLocalScale.mY, nodeChunk.mLocalScale.mZ);
            AZ::Quaternion rot(nodeChunk.mLocalQuat.mX, nodeChunk.mLocalQuat.mY, nodeChunk.mLocalQuat.mZ, nodeChunk.mLocalQuat.mW);

            // convert endian and coordinate system
            ConvertVector3(&pos, endianType);
            ConvertScale(&scale, endianType);
            ConvertQuaternion(&rot, endianType);

            // make sure the input data is normalized
            // TODO: this isn't really needed as we already normalized?
            //rot.FastNormalize();
            //scaleRot.FastNormalize();

            // set the local transform
            Transform bindTransform;
            bindTransform.mPosition     = pos;
            bindTransform.mRotation     = rot.GetNormalized();
            EMFX_SCALECODE
            (
                bindTransform.mScale    = scale;
            )

            actor->GetBindPose()->SetLocalSpaceTransform(nodeIndex, bindTransform);

            // set the skeletal LOD levels
            if (actorSettings->mLoadSkeletalLODs)
            {
                node->SetSkeletalLODLevelBits(nodeChunk.mSkeletalLODs);
            }

            // set if this node has to be taken into the bounding volume calculation
            const bool includeInBoundsCalc = (nodeChunk.mNodeFlags & Node::ENodeFlags::FLAG_INCLUDEINBOUNDSCALC); // first bit
            node->SetIncludeInBoundsCalc(includeInBoundsCalc);

            // Set if this node is critical and cannot be optimized out.
            const bool isCritical = (nodeChunk.mNodeFlags & Node::ENodeFlags::FLAG_CRITICAL); // third bit
            node->SetIsCritical(isCritical);

            // set the parent, and add this node as child inside the parent
            if (nodeChunk.mParentIndex != MCORE_INVALIDINDEX32) // if this node has a parent and the parent node is valid
            {
                if (nodeChunk.mParentIndex < n)
                {
                    node->SetParentIndex(nodeChunk.mParentIndex);
                    Node* parentNode = skeleton->GetNode(nodeChunk.mParentIndex);
                    parentNode->AddChild(nodeIndex);
                }
                else
                {
                    MCore::LogError("Cannot assign parent node index (%d) for node '%s' as the parent node is not yet loaded. Making '%s' a root node.", nodeChunk.mParentIndex, node->GetName(), node->GetName());
                    skeleton->AddRootNode(nodeIndex);
                }
            }
            else // if this node has no parent, so when it is a root node
            {
                skeleton->AddRootNode(nodeIndex);
            }

            // OBB
            AZ::Matrix4x4 obbMatrix4x4 = AZ::Matrix4x4::CreateFromRowMajorFloat16(nodeChunk.mOBB);

            const AZ::Vector3 obbCenter = obbMatrix4x4.GetTranslation();
            const AZ::Vector3 obbExtents = obbMatrix4x4.GetRowAsVector3(3);

            // initialize the OBB
            MCore::OBB obb;
            obb.SetCenter(obbCenter);
            obb.SetExtents(obbExtents);

            // need to transpose to go from row major to column major
            const AZ::Matrix3x3 obbMatrix3x3 = AZ::Matrix3x3::CreateFromMatrix4x4(obbMatrix4x4).GetTranspose();
            const AZ::Transform obbTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(obbMatrix3x3, obbExtents);
            obb.SetTransformation(obbTransform);
            actor->SetNodeOBB(nodeIndex, obb);

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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        AZ_Assert(motion, "Expected a valid motion object.");

        // read the header
        FileFormat::Motion_SubMotions subMotionsHeader;
        file->Read(&subMotionsHeader, sizeof(FileFormat::Motion_SubMotions));
        MCore::Endian::ConvertUnsignedInt32(&subMotionsHeader.mNumSubMotions, endianType);

        // Create a uniform motion data.
        NonUniformMotionData* motionData = aznew NonUniformMotionData();
        motion->SetMotionData(motionData);
        motionData->Resize(subMotionsHeader.mNumSubMotions, motionData->GetNumMorphs(), motionData->GetNumFloats());

        // for all submotions
        for (uint32 s = 0; s < subMotionsHeader.mNumSubMotions; ++s)
        {
            FileFormat::Motion_SkeletalSubMotion fileSubMotion;
            file->Read(&fileSubMotion, sizeof(FileFormat::Motion_SkeletalSubMotion));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.mNumPosKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.mNumRotKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.mNumScaleKeys, endianType);

            // read the motion part name
            const char* motionJointName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // convert into Core objects
            AZ::Vector3 posePos(fileSubMotion.mPosePos.mX, fileSubMotion.mPosePos.mY, fileSubMotion.mPosePos.mZ);
            AZ::Vector3 poseScale(fileSubMotion.mPoseScale.mX, fileSubMotion.mPoseScale.mY, fileSubMotion.mPoseScale.mZ);
            MCore::Compressed16BitQuaternion poseRot(fileSubMotion.mPoseRot.mX, fileSubMotion.mPoseRot.mY, fileSubMotion.mPoseRot.mZ, fileSubMotion.mPoseRot.mW);

            AZ::Vector3 bindPosePos(fileSubMotion.mBindPosePos.mX, fileSubMotion.mBindPosePos.mY, fileSubMotion.mBindPosePos.mZ);
            AZ::Vector3 bindPoseScale(fileSubMotion.mBindPoseScale.mX, fileSubMotion.mBindPoseScale.mY, fileSubMotion.mBindPoseScale.mZ);
            MCore::Compressed16BitQuaternion bindPoseRot(fileSubMotion.mBindPoseRot.mX, fileSubMotion.mBindPoseRot.mY, fileSubMotion.mBindPoseRot.mZ, fileSubMotion.mBindPoseRot.mW);

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
                MCore::LogDetailedInfo("    + Num Pos Keys:          %d", fileSubMotion.mNumPosKeys);
                MCore::LogDetailedInfo("    + Num Rot Keys:          %d", fileSubMotion.mNumRotKeys);
                MCore::LogDetailedInfo("    + Num Scale Keys:        %d", fileSubMotion.mNumScaleKeys);
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
            if (fileSubMotion.mNumPosKeys > 0)
            {
                motionData->AllocateJointPositionSamples(s, fileSubMotion.mNumPosKeys);
                for (i = 0; i < fileSubMotion.mNumPosKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));

                    MCore::Endian::ConvertFloat(&key.mTime, endianType);
                    AZ::Vector3 pos(key.mValue.mX, key.mValue.mY, key.mValue.mZ);
                    ConvertVector3(&pos, endianType);

                    motionData->SetJointPositionSample(s, i, {key.mTime, pos});
                }
            }

            // now the rotation keys
            if (fileSubMotion.mNumRotKeys > 0)
            {
                motionData->AllocateJointRotationSamples(s, fileSubMotion.mNumRotKeys);
                for (i = 0; i < fileSubMotion.mNumRotKeys; ++i)
                {
                    FileFormat::Motion_16BitQuaternionKey key;
                    file->Read(&key, sizeof(FileFormat::Motion_16BitQuaternionKey));

                    MCore::Endian::ConvertFloat(&key.mTime, endianType);
                    MCore::Compressed16BitQuaternion rot(key.mValue.mX, key.mValue.mY, key.mValue.mZ, key.mValue.mW);
                    Convert16BitQuaternion(&rot, endianType);

                    motionData->SetJointRotationSample(s, i, {key.mTime, rot.ToQuaternion().GetNormalized()});
                }
            }


    #ifndef EMFX_SCALE_DISABLED
            // and the scale keys
            if (fileSubMotion.mNumScaleKeys > 0)
            {
                motionData->AllocateJointScaleSamples(s, fileSubMotion.mNumScaleKeys);
                for (i = 0; i < fileSubMotion.mNumScaleKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));

                    MCore::Endian::ConvertFloat(&key.mTime, endianType);
                    AZ::Vector3 scale(key.mValue.mX, key.mValue.mY, key.mValue.mZ);
                    ConvertScale(&scale, endianType);

                    motionData->SetJointScaleSample(s, i, {key.mTime, scale});
                }
            }
    #else   // no scaling
            // and the scale keys
            if (fileSubMotion.mNumScaleKeys > 0)
            {
                for (i = 0; i < fileSubMotion.mNumScaleKeys; ++i)
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;

        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionMask, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.mUnitType);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        motion->SetFileUnitType(motion->GetUnitType());


        // Try to remain backward compatible by still capturing height when this was enabled in the old mask system.
        if (fileInformation.mMotionExtractionMask & (1 << 2))   // The 1<<2 was the mask used for position Z in the old motion extraction mask settings
        {
            motion->SetMotionExtractionFlags(MOTIONEXTRACT_CAPTURE_Z);
        }

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionInfo2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info2 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionFlags, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.mUnitType);
            MCore::LogDetailedInfo("   + Motion Extraction Flags       = 0x%x [capZ=%d]", fileInformation.mMotionExtractionFlags, (fileInformation.mMotionExtractionFlags & EMotionFX::MOTIONEXTRACT_CAPTURE_Z) ? 1 : 0);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        motion->SetFileUnitType(motion->GetUnitType());
        motion->SetMotionExtractionFlags(static_cast<EMotionFX::EMotionExtractionFlags>(fileInformation.mMotionExtractionFlags));

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionInfo3::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info3 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info3));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionFlags, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.mUnitType);
            MCore::LogDetailedInfo("   + Is Additive Motion            = %d", fileInformation.mIsAdditive);
            MCore::LogDetailedInfo("   + Motion Extraction Flags       = 0x%x [capZ=%d]", fileInformation.mMotionExtractionFlags, (fileInformation.mMotionExtractionFlags & EMotionFX::MOTIONEXTRACT_CAPTURE_Z) ? 1 : 0);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        importParams.m_additiveMotion = (fileInformation.mIsAdditive == 0 ? false : true);
        motion->SetFileUnitType(motion->GetUnitType());
        motion->SetMotionExtractionFlags(static_cast<EMotionFX::EMotionExtractionFlags>(fileInformation.mMotionExtractionFlags));

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorPhysicsSetup::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

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
            if (importParams.mActorSettings->mOptimizeForServer)
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        AZ_Assert(actor, "Actor needs to be valid.");

        EMotionFX::FileFormat::Actor_MeshAsset meshAssetChunk;
        file->Read(&meshAssetChunk, sizeof(FileFormat::Actor_MeshAsset));
        const char* meshAssetIdString = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;

        MCORE_ASSERT(motion);

        // read the motion event table header
        FileFormat::FileMotionEventTable fileEventTable;
        file->Read(&fileEventTable, sizeof(FileFormat::FileMotionEventTable));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileEventTable.mNumTracks, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Motion Event Table:");
            MCore::LogDetailedInfo("  + Num Tracks = %d", fileEventTable.mNumTracks);
        }

        // get the motion event table the reserve the event tracks
        MotionEventTable* motionEventTable = motion->GetEventTable();
        motionEventTable->ReserveNumTracks(fileEventTable.mNumTracks);

        // read all tracks
        AZStd::string trackName;
        MCore::Array<AZStd::string> typeStrings;
        MCore::Array<AZStd::string> paramStrings;
        MCore::Array<AZStd::string> mirrorTypeStrings;
        for (uint32 t = 0; t < fileEventTable.mNumTracks; ++t)
        {
            // read the motion event table header
            FileFormat::FileMotionEventTrack fileTrack;
            file->Read(&fileTrack, sizeof(FileFormat::FileMotionEventTrack));

            // read the track name
            trackName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumEvents,             endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumTypeStrings,        endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumParamStrings,       endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumMirrorTypeStrings,  endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Motion Event Track:");
                MCore::LogDetailedInfo("   + Name       = %s", trackName.c_str());
                MCore::LogDetailedInfo("   + Num events = %d", fileTrack.mNumEvents);
                MCore::LogDetailedInfo("   + Num types  = %d", fileTrack.mNumTypeStrings);
                MCore::LogDetailedInfo("   + Num params = %d", fileTrack.mNumParamStrings);
                MCore::LogDetailedInfo("   + Num mirror = %d", fileTrack.mNumMirrorTypeStrings);
                MCore::LogDetailedInfo("   + Enabled    = %d", fileTrack.mIsEnabled);
            }

            // the even type and parameter strings
            typeStrings.Resize(fileTrack.mNumTypeStrings);
            paramStrings.Resize(fileTrack.mNumParamStrings);
            mirrorTypeStrings.Resize(fileTrack.mNumMirrorTypeStrings);

            // read all type strings
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Event types:");
            }
            uint32 i;
            for (i = 0; i < fileTrack.mNumTypeStrings; ++i)
            {
                typeStrings[i] = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
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
            for (i = 0; i < fileTrack.mNumParamStrings; ++i)
            {
                paramStrings[i] = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, paramStrings[i].c_str());
                }
            }

            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Mirror Type Strings:");
            }
            for (i = 0; i < fileTrack.mNumMirrorTypeStrings; ++i)
            {
                mirrorTypeStrings[i] = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, mirrorTypeStrings[i].c_str());
                }
            }

            // create the default event track
            MotionEventTrack* track = MotionEventTrack::Create(trackName.c_str(), motion);
            track->SetIsEnabled(fileTrack.mIsEnabled != 0);
            track->ReserveNumEvents(fileTrack.mNumEvents);
            motionEventTable->AddTrack(track);

            // read all motion events
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Motion Events:");
            }
            for (i = 0; i < fileTrack.mNumEvents; ++i)
            {
                // read the event header
                FileFormat::FileMotionEvent fileEvent;
                file->Read(&fileEvent, sizeof(FileFormat::FileMotionEvent));

                // convert endian
                MCore::Endian::ConvertUnsignedInt32(&fileEvent.mEventTypeIndex, endianType);
                MCore::Endian::ConvertUnsignedInt16(&fileEvent.mParamIndex, endianType);
                MCore::Endian::ConvertUnsignedInt32(&fileEvent.mMirrorTypeIndex, endianType);
                MCore::Endian::ConvertFloat(&fileEvent.mStartTime, endianType);
                MCore::Endian::ConvertFloat(&fileEvent.mEndTime, endianType);

                // print motion event information
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] StartTime = %f  -  EndTime = %f  -  Type = '%s'  -  Param = '%s'  -  Mirror = '%s'", i, fileEvent.mStartTime, fileEvent.mEndTime, typeStrings[fileEvent.mEventTypeIndex].c_str(), paramStrings[fileEvent.mParamIndex].c_str(), mirrorTypeStrings[fileEvent.mMirrorTypeIndex].c_str());
                }

                const AZStd::string eventTypeName = fileEvent.mEventTypeIndex != MCORE_INVALIDINDEX32 ?
                    typeStrings[fileEvent.mEventTypeIndex] : "";
                const AZStd::string mirrorTypeName = fileEvent.mMirrorTypeIndex != MCORE_INVALIDINDEX32 ?
                    mirrorTypeStrings[fileEvent.mMirrorTypeIndex] : "";
                const AZStd::string params = paramStrings[fileEvent.mParamIndex];

                // add the event
                track->AddEvent(
                    fileEvent.mStartTime,
                    fileEvent.mEndTime,
                    GetEventManager().FindOrCreateEventData<EMotionFX::TwoStringEventData>(eventTypeName, params, mirrorTypeName)
                    );
            }
        } // for all tracks

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionEventTrackTable2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        Motion* motion = importParams.mMotion;

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
        Motion* motion = importParams.mMotion;
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

        auto readJsonOutcome = AzFramework::FileFunc::ReadJsonFromString(bufferStringView);
        AZStd::string errorMsg;
        if (!readJsonOutcome.IsSuccess())
        {
            AZ_Error("EMotionFX", false, "Loading motion event table failed due to ReadJsonFromString. %s", readJsonOutcome.TakeError().c_str());
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);

        // read the chunk
        FileFormat::Actor_Info fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mTrajectoryNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mNumLODs, endianType);
        MCore::Endian::ConvertFloat(&fileInformation.mRetargetRootOffset, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        const char* name = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        actor->SetName(name);
        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
        }

        // print motion event information
        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.mExporterHighVersion, fileInformation.mExporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.mNumLODs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.mMotionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root offset   = %f", fileInformation.mRetargetRootOffset);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.mUnitType);
        }

        actor->SetMotionExtractionNodeIndex(fileInformation.mMotionExtractionNodeIndex);
        //  actor->SetRetargetOffset( fileInformation.mRetargetRootOffset );
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        actor->SetFileUnitType(actor->GetUnitType());

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorInfo2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        // read the chunk
        FileFormat::Actor_Info2 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mRetargetRootNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mNumLODs, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        const char* name = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        actor->SetName(name);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.mExporterHighVersion, fileInformation.mExporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.mNumLODs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.mMotionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root node     = %d", fileInformation.mRetargetRootNodeIndex);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.mUnitType);
        }

        actor->SetMotionExtractionNodeIndex(fileInformation.mMotionExtractionNodeIndex);
        actor->SetRetargetRootNodeIndex(fileInformation.mRetargetRootNodeIndex);
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        actor->SetFileUnitType(actor->GetUnitType());

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorInfo3::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        // read the chunk
        FileFormat::Actor_Info3 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info3));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mRetargetRootNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mNumLODs, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        const char* name = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        actor->SetName(name);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.mExporterHighVersion, fileInformation.mExporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.mNumLODs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.mMotionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root node     = %d", fileInformation.mRetargetRootNodeIndex);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.mUnitType);
        }

        actor->SetMotionExtractionNodeIndex(fileInformation.mMotionExtractionNodeIndex);
        actor->SetRetargetRootNodeIndex(fileInformation.mRetargetRootNodeIndex);
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        actor->SetFileUnitType(actor->GetUnitType());
        actor->SetOptimizeSkeleton(fileInformation.mOptimizeSkeleton == 0? false : true);

        return true;
    }

    //=================================================================================================

    // morph targets
    bool ChunkProcessorActorProgMorphTarget::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the expression part from disk
        FileFormat::Actor_MorphTarget morphTargetChunk;
        file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

        // convert endian
        MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMin, endianType);
        MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMax, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mLOD, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mNumTransformations, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mPhonemeSets, endianType);

        // get the expression name
        const char* morphTargetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        // get the level of detail of the expression part
        const uint32 morphTargetLOD = morphTargetChunk.mLOD;

        if (GetLogging())
        {
            MCore::LogDetailedInfo(" - Morph Target:");
            MCore::LogDetailedInfo("    + Name               = '%s'", morphTargetName);
            MCore::LogDetailedInfo("    + LOD Level          = %d", morphTargetChunk.mLOD);
            MCore::LogDetailedInfo("    + RangeMin           = %f", morphTargetChunk.mRangeMin);
            MCore::LogDetailedInfo("    + RangeMax           = %f", morphTargetChunk.mRangeMax);
            MCore::LogDetailedInfo("    + NumTransformations = %d", morphTargetChunk.mNumTransformations);
            MCore::LogDetailedInfo("    + PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets).c_str());
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
        morphTarget->SetRangeMin(morphTargetChunk.mRangeMin);
        morphTarget->SetRangeMax(morphTargetChunk.mRangeMax);

        // set the phoneme sets
        morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets);

        // add the morph target
        actor->GetMorphSetup(morphTargetLOD)->AddMorphTarget(morphTarget);

        // read the facial transformations
        for (uint32 i = 0; i < morphTargetChunk.mNumTransformations; ++i)
        {
            // read the facial transformation from disk
            FileFormat::Actor_MorphTargetTransform transformChunk;
            file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

            // create Core objects from the data
            AZ::Vector3 pos(transformChunk.mPosition.mX, transformChunk.mPosition.mY, transformChunk.mPosition.mZ);
            AZ::Vector3 scale(transformChunk.mScale.mX, transformChunk.mScale.mY, transformChunk.mScale.mZ);
            AZ::Quaternion rot(transformChunk.mRotation.mX, transformChunk.mRotation.mY, transformChunk.mRotation.mZ, transformChunk.mRotation.mW);
            AZ::Quaternion scaleRot(transformChunk.mScaleRotation.mX, transformChunk.mScaleRotation.mY, transformChunk.mScaleRotation.mZ, transformChunk.mScaleRotation.mW);

            // convert endian and coordinate system
            ConvertVector3(&pos, endianType);
            ConvertScale(&scale, endianType);
            ConvertQuaternion(&rot, endianType);
            ConvertQuaternion(&scaleRot, endianType);
            MCore::Endian::ConvertUnsignedInt32(&transformChunk.mNodeIndex, endianType);

            // create our transformation
            MorphTargetStandard::Transformation transform;
            transform.mPosition         = pos;
            transform.mScale            = scale;
            transform.mRotation         = rot;
            transform.mScaleRotation    = scaleRot;
            transform.mNodeIndex        = transformChunk.mNodeIndex;

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    - Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.mNodeIndex)->GetName(), transform.mNodeIndex);
                MCore::LogDetailedInfo("       + Pos:      %f, %f, %f",
                    static_cast<float>(transform.mPosition.GetX()),
                    static_cast<float>(transform.mPosition.GetY()),
                    static_cast<float>(transform.mPosition.GetZ()));
                MCore::LogDetailedInfo("       + Rotation: %f, %f, %f %f", 
                    static_cast<float>(transform.mRotation.GetX()),
                    static_cast<float>(transform.mRotation.GetY()),
                    static_cast<float>(transform.mRotation.GetZ()),
                    static_cast<float>(transform.mRotation.GetW()));
                MCore::LogDetailedInfo("       + Scale:    %f, %f, %f",
                    static_cast<float>(transform.mScale.GetX()),
                    static_cast<float>(transform.mScale.GetY()),
                    static_cast<float>(transform.mScale.GetZ()));
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

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
            MCore::Endian::ConvertUnsignedInt16(&fileGroup.mNumNodes, endianType);

            // read the group name
            const char* groupName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // log some info
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Group '%s'", groupName);
                MCore::LogDetailedInfo("     - Num nodes: %d", fileGroup.mNumNodes);
                MCore::LogDetailedInfo("     - Disabled on default: %s", fileGroup.mDisabledOnDefault ? "Yes" : "No");
            }

            // create the new group inside the actor
            NodeGroup* newGroup = NodeGroup::Create(groupName, fileGroup.mNumNodes, fileGroup.mDisabledOnDefault ? false : true);

            // read the node numbers
            uint16 nodeIndex;
            for (uint16 n = 0; n < fileGroup.mNumNodes; ++n)
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        AZ_Assert(motion, "Expecting a valid motion pointer.");

        // cast to  morph motion
        AZ_Assert(motion->GetMotionData(), "Expecting to have motion data allocated.");
        NonUniformMotionData* motionData = azdynamic_cast<NonUniformMotionData*>(motion->GetMotionData());
        AZ_Assert(motionData, "Expected motion data to be of non-uniform motion data type.");

        FileFormat::Motion_MorphSubMotions subMotionsHeader;
        file->Read(&subMotionsHeader, sizeof(FileFormat::Motion_MorphSubMotions));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&subMotionsHeader.mNumSubMotions, endianType);

        // pre-allocate the number of submotions
        motionData->SetAdditive(importParams.m_additiveMotion);
        motionData->Resize(motionData->GetNumJoints(), subMotionsHeader.mNumSubMotions, motionData->GetNumFloats());

        // for all submotions
        for (uint32 s = 0; s < subMotionsHeader.mNumSubMotions; ++s)
        {
            // get the  morph motion part
            FileFormat::Motion_MorphSubMotion morphSubMotionChunk;
            file->Read(&morphSubMotionChunk, sizeof(FileFormat::Motion_MorphSubMotion));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&morphSubMotionChunk.mNumKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphSubMotionChunk.mPhonemeSet, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.mPoseWeight, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.mMinWeight, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.mMaxWeight, endianType);

            // read the name of the submotion
            const char* name = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            motionData->SetMorphName(s, name);
            motionData->AllocateMorphSamples(s, morphSubMotionChunk.mNumKeys);
            motionData->SetMorphStaticValue(s, morphSubMotionChunk.mPoseWeight);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    - Morph Submotion: %s", name);
                MCore::LogDetailedInfo("       + NrKeys             = %d", morphSubMotionChunk.mNumKeys);
                MCore::LogDetailedInfo("       + Pose Weight        = %f", morphSubMotionChunk.mPoseWeight);
                MCore::LogDetailedInfo("       + Minimum Weight     = %f", morphSubMotionChunk.mMinWeight);
                MCore::LogDetailedInfo("       + Maximum Weight     = %f", morphSubMotionChunk.mMaxWeight);
                MCore::LogDetailedInfo("       + PhonemeSet         = %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphSubMotionChunk.mPhonemeSet).c_str());
            }

            // add keyframes
            for (uint32 i = 0; i < morphSubMotionChunk.mNumKeys; ++i)
            {
                FileFormat::Motion_UnsignedShortKey keyframeChunk;

                file->Read(&keyframeChunk, sizeof(FileFormat::Motion_UnsignedShortKey));
                MCore::Endian::ConvertFloat(&keyframeChunk.mTime, endianType);
                MCore::Endian::ConvertUnsignedInt16(&keyframeChunk.mValue, endianType);

                const float value = keyframeChunk.mValue / static_cast<float>(std::numeric_limits<uint16_t>::max());
                motionData->SetMorphSample(s, i, {keyframeChunk.mTime, value});
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the header
        FileFormat::Actor_MorphTargets morphTargetsHeader;
        file->Read(&morphTargetsHeader, sizeof(FileFormat::Actor_MorphTargets));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.mNumMorphTargets, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.mLOD, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Morph targets: %d (LOD=%d)", morphTargetsHeader.mNumMorphTargets, morphTargetsHeader.mLOD);
        }

        // check if the morph setup has already been created, if not create it
        if (actor->GetMorphSetup(morphTargetsHeader.mLOD) == nullptr)
        {
            // create the morph setup
            MorphSetup* morphSetup = MorphSetup::Create();

            // set the morph setup
            actor->SetMorphSetup(morphTargetsHeader.mLOD, morphSetup);
        }

        // pre-allocate the morph targets
        MorphSetup* setup = actor->GetMorphSetup(morphTargetsHeader.mLOD);
        setup->ReserveMorphTargets(morphTargetsHeader.mNumMorphTargets);

        // read in all morph targets
        for (uint32 mt = 0; mt < morphTargetsHeader.mNumMorphTargets; ++mt)
        {
            // read the expression part from disk
            FileFormat::Actor_MorphTarget morphTargetChunk;
            file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

            // convert endian
            MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMin, endianType);
            MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMax, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mLOD, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mNumTransformations, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mPhonemeSets, endianType);

            // make sure they match
            MCORE_ASSERT(morphTargetChunk.mLOD == morphTargetsHeader.mLOD);

            // get the expression name
            const char* morphTargetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // get the level of detail of the expression part
            const uint32 morphTargetLOD = morphTargetChunk.mLOD;

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + Morph Target:");
                MCore::LogDetailedInfo("     - Name               = '%s'", morphTargetName);
                MCore::LogDetailedInfo("     - LOD Level          = %d", morphTargetChunk.mLOD);
                MCore::LogDetailedInfo("     - RangeMin           = %f", morphTargetChunk.mRangeMin);
                MCore::LogDetailedInfo("     - RangeMax           = %f", morphTargetChunk.mRangeMax);
                MCore::LogDetailedInfo("     - NumTransformations = %d", morphTargetChunk.mNumTransformations);
                MCore::LogDetailedInfo("     - PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets).c_str());
            }

            // create the morph target
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphTargetName);

            // set the slider range
            morphTarget->SetRangeMin(morphTargetChunk.mRangeMin);
            morphTarget->SetRangeMax(morphTargetChunk.mRangeMax);

            // set the phoneme sets
            morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets);

            // add the morph target
            setup->AddMorphTarget(morphTarget);

            // the same for the transformations
            morphTarget->ReserveTransformations(morphTargetChunk.mNumTransformations);

            // read the facial transformations
            for (uint32 i = 0; i < morphTargetChunk.mNumTransformations; ++i)
            {
                // read the facial transformation from disk
                FileFormat::Actor_MorphTargetTransform transformChunk;
                file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

                // create Core objects from the data
                AZ::Vector3 pos(transformChunk.mPosition.mX, transformChunk.mPosition.mY, transformChunk.mPosition.mZ);
                AZ::Vector3 scale(transformChunk.mScale.mX, transformChunk.mScale.mY, transformChunk.mScale.mZ);
                AZ::Quaternion rot(transformChunk.mRotation.mX, transformChunk.mRotation.mY, transformChunk.mRotation.mZ, transformChunk.mRotation.mW);
                AZ::Quaternion scaleRot(transformChunk.mScaleRotation.mX, transformChunk.mScaleRotation.mY, transformChunk.mScaleRotation.mZ, transformChunk.mScaleRotation.mW);

                // convert endian and coordinate system
                ConvertVector3(&pos, endianType);
                ConvertScale(&scale, endianType);
                ConvertQuaternion(&rot, endianType);
                ConvertQuaternion(&scaleRot, endianType);
                MCore::Endian::ConvertUnsignedInt32(&transformChunk.mNodeIndex, endianType);

                // create our transformation
                MorphTargetStandard::Transformation transform;
                transform.mPosition         = pos;
                transform.mScale            = scale;
                transform.mRotation         = rot;
                transform.mScaleRotation    = scaleRot;
                transform.mNodeIndex        = transformChunk.mNodeIndex;

                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     + Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.mNodeIndex)->GetName(), transform.mNodeIndex);
                    MCore::LogDetailedInfo("        - Pos:      %f, %f, %f",
                        static_cast<float>(transform.mPosition.GetX()),
                        static_cast<float>(transform.mPosition.GetY()),
                        static_cast<float>(transform.mPosition.GetZ()));
                    MCore::LogDetailedInfo("        - Rotation: %f, %f, %f %f",
                        static_cast<float>(transform.mRotation.GetX()),
                        static_cast<float>(transform.mRotation.GetY()),
                        static_cast<float>(transform.mRotation.GetZ()),
                        static_cast<float>(transform.mRotation.GetW()));
                    MCore::LogDetailedInfo("        - Scale:    %f, %f, %f",
                        static_cast<float>(transform.mScale.GetX()),
                        static_cast<float>(transform.mScale.GetY()),
                        static_cast<float>(transform.mScale.GetZ()));
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the header
        FileFormat::Actor_MorphTargets morphTargetsHeader;
        file->Read(&morphTargetsHeader, sizeof(FileFormat::Actor_MorphTargets));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.mNumMorphTargets, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.mLOD, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Morph targets: %d (LOD=%d)", morphTargetsHeader.mNumMorphTargets, morphTargetsHeader.mLOD);
        }

        // check if the morph setup has already been created, if not create it
        if (actor->GetMorphSetup(morphTargetsHeader.mLOD) == nullptr)
        {
            // create the morph setup
            MorphSetup* morphSetup = MorphSetup::Create();

            // set the morph setup
            actor->SetMorphSetup(morphTargetsHeader.mLOD, morphSetup);
        }

        // pre-allocate the morph targets
        MorphSetup* setup = actor->GetMorphSetup(morphTargetsHeader.mLOD);
        setup->ReserveMorphTargets(morphTargetsHeader.mNumMorphTargets);

        // read in all morph targets
        for (uint32 mt = 0; mt < morphTargetsHeader.mNumMorphTargets; ++mt)
        {
            // read the expression part from disk
            FileFormat::Actor_MorphTarget morphTargetChunk;
            file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

            // convert endian
            MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMin, endianType);
            MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMax, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mLOD, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mNumTransformations, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mPhonemeSets, endianType);

            // make sure they match
            MCORE_ASSERT(morphTargetChunk.mLOD == morphTargetsHeader.mLOD);

            // get the expression name
            const char* morphTargetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // get the level of detail of the expression part
            const uint32 morphTargetLOD = morphTargetChunk.mLOD;

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + Morph Target:");
                MCore::LogDetailedInfo("     - Name               = '%s'", morphTargetName);
                MCore::LogDetailedInfo("     - LOD Level          = %d", morphTargetChunk.mLOD);
                MCore::LogDetailedInfo("     - RangeMin           = %f", morphTargetChunk.mRangeMin);
                MCore::LogDetailedInfo("     - RangeMax           = %f", morphTargetChunk.mRangeMax);
                MCore::LogDetailedInfo("     - NumTransformations = %d", morphTargetChunk.mNumTransformations);
                MCore::LogDetailedInfo("     - PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets).c_str());
            }

            // create the morph target
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphTargetName);

            // set the slider range
            morphTarget->SetRangeMin(morphTargetChunk.mRangeMin);
            morphTarget->SetRangeMax(morphTargetChunk.mRangeMax);

            // set the phoneme sets
            morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets);

            // add the morph target
            setup->AddMorphTarget(morphTarget);

            // the same for the transformations
            morphTarget->ReserveTransformations(morphTargetChunk.mNumTransformations);

            // read the facial transformations
            for (uint32 i = 0; i < morphTargetChunk.mNumTransformations; ++i)
            {
                // read the facial transformation from disk
                FileFormat::Actor_MorphTargetTransform transformChunk;
                file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

                // create Core objects from the data
                AZ::Vector3 pos(transformChunk.mPosition.mX, transformChunk.mPosition.mY, transformChunk.mPosition.mZ);
                AZ::Vector3 scale(transformChunk.mScale.mX, transformChunk.mScale.mY, transformChunk.mScale.mZ);
                AZ::Quaternion rot(transformChunk.mRotation.mX, transformChunk.mRotation.mY, transformChunk.mRotation.mZ, transformChunk.mRotation.mW);
                AZ::Quaternion scaleRot(transformChunk.mScaleRotation.mX, transformChunk.mScaleRotation.mY, transformChunk.mScaleRotation.mZ, transformChunk.mScaleRotation.mW);

                // convert endian and coordinate system
                ConvertVector3(&pos, endianType);
                ConvertScale(&scale, endianType);
                ConvertQuaternion(&rot, endianType);
                ConvertQuaternion(&scaleRot, endianType);
                MCore::Endian::ConvertUnsignedInt32(&transformChunk.mNodeIndex, endianType);

                // create our transformation
                MorphTargetStandard::Transformation transform;
                transform.mPosition         = pos;
                transform.mScale            = scale;
                transform.mRotation         = rot;
                transform.mScaleRotation    = scaleRot;
                transform.mNodeIndex        = transformChunk.mNodeIndex;

                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     + Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.mNodeIndex)->GetName(), transform.mNodeIndex);
                    MCore::LogDetailedInfo("        - Pos:      %f, %f, %f",
                        static_cast<float>(transform.mPosition.GetX()),
                        static_cast<float>(transform.mPosition.GetY()),
                        static_cast<float>(transform.mPosition.GetZ()));
                    MCore::LogDetailedInfo("        - Rotation: %f, %f, %f %f", 
                        static_cast<float>(transform.mRotation.GetX()),
                        static_cast<float>(transform.mRotation.GetY()),
                        static_cast<float>(transform.mRotation.GetZ()),
                        static_cast<float>(transform.mRotation.GetW()));
                    MCore::LogDetailedInfo("        - Scale:    %f, %f, %f",
                        static_cast<float>(transform.mScale.GetX()),
                        static_cast<float>(transform.mScale.GetY()),
                        static_cast<float>(transform.mScale.GetZ()));
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        uint32 i;
        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the file data
        FileFormat::Actor_NodeMotionSources2 nodeMotionSourcesChunk;
        file->Read(&nodeMotionSourcesChunk, sizeof(FileFormat::Actor_NodeMotionSources2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeMotionSourcesChunk.mNumNodes, endianType);
        const uint32 numNodes = nodeMotionSourcesChunk.mNumNodes;
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
            actor->GetNodeMirrorInfo(i).mSourceNode = sourceNode;
        }

        // read all axes
        for (i = 0; i < numNodes; ++i)
        {
            uint8 axis;
            file->Read(&axis, sizeof(uint8));
            actor->GetNodeMirrorInfo(i).mAxis = axis;
        }

        // read all flags
        for (i = 0; i < numNodes; ++i)
        {
            uint8 flags;
            file->Read(&flags, sizeof(uint8));
            actor->GetNodeMirrorInfo(i).mFlags = flags;
        }

        // log details
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node Motion Sources (%i):", numNodes);
            for (i = 0; i < numNodes; ++i)
            {
                if (actor->GetNodeMirrorInfo(i).mSourceNode != MCORE_INVALIDINDEX16)
                {
                    MCore::LogDetailedInfo("   + '%s' (%i) -> '%s' (%i) [axis=%d] [flags=%d]", skeleton->GetNode(i)->GetName(), i, skeleton->GetNode(actor->GetNodeMirrorInfo(i).mSourceNode)->GetName(), actor->GetNodeMirrorInfo(i).mSourceNode, actor->GetNodeMirrorInfo(i).mAxis, actor->GetNodeMirrorInfo(i).mFlags);
                }
            }
        }

        return true;
    }


    //----------------------------------------------------------------------------------------------------------

    bool ChunkProcessorActorAttachmentNodes::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        uint32 i;
        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the file data
        FileFormat::Actor_AttachmentNodes attachmentNodesChunk;
        file->Read(&attachmentNodesChunk, sizeof(FileFormat::Actor_AttachmentNodes));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&attachmentNodesChunk.mNumNodes, endianType);
        const uint32 numAttachmentNodes = attachmentNodesChunk.mNumNodes;

        // read all node attachment nodes
        for (i = 0; i < numAttachmentNodes; ++i)
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

            const uint32 numNodes = actor->GetNumNodes();
            for (i = 0; i < numNodes; ++i)
            {
                // get the current node
                Node* node = skeleton->GetNode(i);

                // only log the attachment nodes
                if (node->GetIsAttachmentNode())
                {
                    MCore::LogDetailedInfo("   + '%s' (%i)", node->GetName(), node->GetNodeIndex());
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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;

        // read the header
        FileFormat::NodeMapChunk nodeMapChunk;
        file->Read(&nodeMapChunk, sizeof(FileFormat::NodeMapChunk));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeMapChunk.mNumEntries, endianType);

        // load the source actor filename string, but discard it
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        // log some info
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node Map:");
            MCore::LogDetailedInfo("  + Num entries = %d", nodeMapChunk.mNumEntries);
        }

        // for all entries
        const uint32 numEntries = nodeMapChunk.mNumEntries;
        importParams.mNodeMap->Reserve(numEntries);
        AZStd::string firstName;
        AZStd::string secondName;
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // read both names
            firstName  = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            secondName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + [%d] '%s' -> '%s'", i, firstName.c_str(), secondName.c_str());
            }

            // create the entry
            if (importParams.mNodeMapSettings->mLoadNodes)
            {
                importParams.mNodeMap->AddEntry(firstName.c_str(), secondName.c_str());
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
        MCore::Endian::ConvertUnsignedInt32(&dataHeader.m_sizeInBytes, importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&dataHeader.m_dataVersion, importParams.mEndianType);

        // Read the strings.
        const AZStd::string uuidString = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
        const AZStd::string className = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);

        // Create the motion data of this type.
        const AZ::Uuid uuid = AZ::Uuid::CreateString(uuidString.c_str(), uuidString.size());
        MotionData* motionData = GetMotionManager().GetMotionDataFactory().Create(uuid);

        // Check if we could create it.
        if (!motionData)
        {
            AZ_Assert(false, "Unsupported motion data type '%s' using uuid '%s'", className.c_str(), uuidString.c_str());
            motionData = aznew UniformMotionData(); // Create an empty dummy motion data, so we don't break things.
            importParams.mMotion->SetMotionData(motionData);
            file->Forward(dataHeader.m_sizeInBytes);
            return false;
        }

        // Read the data.
        MotionData::ReadSettings readSettings;
        readSettings.m_sourceEndianType = importParams.mEndianType;
        readSettings.m_logDetails = GetLogging();
        readSettings.m_version = dataHeader.m_dataVersion;
        if (!motionData->Read(file, readSettings))
        {
            AZ_Error("EMotionFX", false, "Failed to load motion data of type '%s'", className.c_str());
            motionData = aznew UniformMotionData(); // Create an empty dummy motion data, so we don't break things.
            importParams.mMotion->SetMotionData(motionData);
            return false;
        }

        importParams.mMotion->SetMotionData(motionData);
        return true;
    }

} // namespace EMotionFX
