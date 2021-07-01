/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include "LegacyAnimGraphNodeParser.h"

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
        mIsUnicodeFile      = true;

        // allocate the string buffer used for reading in variable sized strings
        mStringStorageSize  = 256;
        mStringStorage      = (char*)MCore::Allocate(mStringStorageSize, EMFX_MEMCATEGORY_IMPORTER);
        mBlendNodes.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
        mBlendNodes.Reserve(1024);
        //mConvertString.Reserve( 256 );
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
        //mConvertString.Clear();

        // get rid of the blend nodes array
        mBlendNodes.Clear();

        m_entryNodeIndexToStateMachineIdLookupTable.clear();
    }


    // check if the strings in the file are encoded using unicode or multi-byte
    bool SharedHelperData::GetIsUnicodeFile(const char* dateString, MCore::Array<SharedData*>* sharedData)
    {
        // find the helper data
        SharedData* data = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData = static_cast<SharedHelperData*>(data);

        AZStd::vector<AZStd::string> dateParts;
        AzFramework::StringFunc::Tokenize(dateString, dateParts, MCore::CharacterConstants::space, false /* keep empty strings */, true /* keep space strings */);

        // decode the month
        int32 month = 0;
        const AZStd::string& monthString = dateParts[0];
        const char* monthStrings[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        for (int32 i = 0; i < 12; ++i)
        {
            if (monthString == monthStrings[i])
            {
                month = i + 1;
                break;
            }
        }

        //int32 day = dateParts[1].ToInt();
        int32 year;
        if (!AzFramework::StringFunc::LooksLikeInt(dateParts[2].c_str(), &year))
        {
            return false;
        }

        // set if the file contains unicode strings or not based on the compilcation date
        if (year < 2012 || (year == 2012 && month < 11))
        {
            helperData->mIsUnicodeFile = false;
        }

        //LogInfo( "String: '%s', Decoded: %i.%i.%i - isUnicode=%i", dateString, day, month, year, helperData->mIsUnicodeFile );

        return helperData->mIsUnicodeFile;
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


    // get the array of anim graph nodes
    MCore::Array<AnimGraphNode*>& SharedHelperData::GetBlendNodes(MCore::Array<SharedData*>* sharedData)
    {
        // find the helper data
        SharedData* data                = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData    = static_cast<SharedHelperData*>(data);
        return helperData->mBlendNodes;
    }

    // Get the table of entry state indices to state machines IDs
    AZStd::map<AZ::u64, uint32>& SharedHelperData::GetEntryStateToStateMachineTable(MCore::Array<SharedData*>* sharedData)
    {
        // Find the helper data
        SharedData* data = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData = static_cast<SharedHelperData*>(data);
        return helperData->m_entryNodeIndexToStateMachineIdLookupTable;
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

    //
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

    // animGraph state transitions
    bool ChunkProcessorAnimGraphStateTransitions::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        // read the number of transitions to follow
        uint32 numTransitions;
        file->Read(&numTransitions, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numTransitions, importParams.mEndianType);

        // read the state machine index
        uint32 stateMachineIndex;
        file->Read(&stateMachineIndex, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&stateMachineIndex, importParams.mEndianType);

        // get the loaded anim graph nodes
        MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(importParams.mSharedData);
        if (stateMachineIndex >= blendNodes.GetLength())
        {
            if (GetLogging())
            {
                AZ_Error("EMotionFX", false, "State machine refers to invalid blend node, state machine index: %d, amount of blend node: %d", stateMachineIndex, blendNodes.GetLength());
            }
            return false;
        }
        AZ_Assert(azrtti_typeid(blendNodes[stateMachineIndex]) == azrtti_typeid<AnimGraphStateMachine>(), "ChunkProcessorAnimGraphStateTransitions::Process : Unexpected node type expected AnimGraphStateMachine. Found %u instead", azrtti_typeid(blendNodes[stateMachineIndex]));
        AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(blendNodes[stateMachineIndex]);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num transitions for state machine '%s' = %d", blendNodes[stateMachineIndex]->GetName(), numTransitions);
        }

        stateMachine->ReserveTransitions(numTransitions);

        // read the transitions
        FileFormat::AnimGraph_StateTransition transition;
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // read the transition
            file->Read(&transition, sizeof(FileFormat::AnimGraph_StateTransition));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&transition.mSourceNode,   importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&transition.mDestNode,     importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&transition.mNumConditions, importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mStartOffsetX,   importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mStartOffsetY,   importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mEndOffsetX,     importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mEndOffsetY,     importParams.mEndianType);

            //----------------------------------------------
            // read the node header
            FileFormat::AnimGraph_NodeHeader nodeHeader;
            file->Read(&nodeHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mTypeID,               importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mParentIndex,          importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVersion,              importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumCustomDataBytes,   importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumChildNodes,        importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumAttributes,        importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosX,             importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosY,             importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVisualizeColor,       importParams.mEndianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("- State Transition Node:");
                MCore::LogDetailedInfo("  + Type            = %d", nodeHeader.mTypeID);
                MCore::LogDetailedInfo("  + Version         = %d", nodeHeader.mVersion);
                MCore::LogDetailedInfo("  + Num data bytes  = %d", nodeHeader.mNumCustomDataBytes);
                MCore::LogDetailedInfo("  + Num attributes  = %d", nodeHeader.mNumAttributes);
                MCore::LogDetailedInfo("  + Num conditions  = %d", transition.mNumConditions);
                MCore::LogDetailedInfo("  + Source node     = %d", transition.mSourceNode);
                MCore::LogDetailedInfo("  + Dest node       = %d", transition.mDestNode);
            }

            // create the transition object
            AnimGraphStateTransition* emfxTransition = nullptr;
            if (GetNewTypeIdByOldNodeTypeId(nodeHeader.mTypeID) == azrtti_typeid<AnimGraphStateTransition>())
            {
                emfxTransition = aznew AnimGraphStateTransition();
            }

            if (emfxTransition)
            {
                if (transition.mDestNode >= blendNodes.GetLength())
                {
                    if (GetLogging())
                    {
                        AZ_Error("EMotionFX", false, "State machine transition refers to invalid destination blend node, transition index %d, blend node: %d", i, transition.mDestNode);
                    }
                    delete emfxTransition;
                    emfxTransition = nullptr;
                }
                // A source node index of MCORE_INVALIDINDEX32 indicates that the transition is a wildcard transition. Don't go into error state in this case.
                else if (transition.mSourceNode != MCORE_INVALIDINDEX32 && transition.mSourceNode >= blendNodes.GetLength())
                {
                    if (GetLogging())
                    {
                        AZ_Error("EMotionFX", false, "State machine transition refers to invalid source blend node, transition index %d, blend node: %d", i, transition.mSourceNode);
                    }
                    delete emfxTransition;
                    emfxTransition = nullptr;
                }
                else
                {
                    AnimGraphNode* targetNode = blendNodes[transition.mDestNode];
                    if (targetNode == nullptr)
                    {
                        delete emfxTransition;
                        emfxTransition = nullptr;
                    }
                    else
                    {
                        AZ_Assert(azrtti_istypeof<AnimGraphStateTransition>(emfxTransition), "ChunkProcessorAnimGraphStateTransitions::Process : Unexpected node type expected AnimGraphStateTransition. Found %u instead", azrtti_typeid(blendNodes[stateMachineIndex]));

                        // Now apply the transition settings
                        // Check if we are dealing with a wildcard transition
                        if (transition.mSourceNode == MCORE_INVALIDINDEX32)
                        {
                            emfxTransition->SetSourceNode(nullptr);
                            emfxTransition->SetIsWildcardTransition(true);
                        }
                        else
                        {
                            // set the source node
                            emfxTransition->SetSourceNode(blendNodes[transition.mSourceNode]);
                        }

                        // set the destination node
                        emfxTransition->SetTargetNode(targetNode);

                        emfxTransition->SetVisualOffsets(transition.mStartOffsetX, transition.mStartOffsetY, transition.mEndOffsetX, transition.mEndOffsetY);

                        // now read the attributes
                        if (!LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateTransition>(file, nodeHeader.mNumAttributes, importParams.mEndianType, importParams, *emfxTransition))
                        {
                            delete emfxTransition;
                            emfxTransition = nullptr;
                            AZ_Error("EMotionFX", false, "Unable to parse state transition");
                            return false;
                        }
                        // add the transition to the state machine
                        stateMachine->AddTransition(emfxTransition);
                    }
                }
            }

            if (emfxTransition)
            {
                // iterate through all conditions
                for (uint32 c = 0; c < transition.mNumConditions; ++c)
                {
                    // read the condition node header
                    FileFormat::AnimGraph_NodeHeader conditionHeader;
                    file->Read(&conditionHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

                    // convert endian
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mTypeID,              importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mVersion,             importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumCustomDataBytes,  importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumAttributes,       importParams.mEndianType);

                    if (GetLogging())
                    {
                        MCore::LogDetailedInfo("   - Transition Condition:");
                        MCore::LogDetailedInfo("     + Type            = %d", conditionHeader.mTypeID);
                        MCore::LogDetailedInfo("     + Version         = %d", conditionHeader.mVersion);
                        MCore::LogDetailedInfo("     + Num data bytes  = %d", conditionHeader.mNumCustomDataBytes);
                        MCore::LogDetailedInfo("     + Num attributes  = %d", conditionHeader.mNumAttributes);
                    }

                    AnimGraphTransitionCondition* emfxCondition = nullptr;
                    if (!LegacyAnimGraphNodeParser::ParseTransitionConditionChunk(file, importParams, conditionHeader, emfxCondition))
                    {
                        AZ_Error("EMotionFX", false, "Unable to parse Transition condition of type %u in legacy file", azrtti_typeid(emfxCondition));
                        delete emfxCondition;
                        emfxCondition = nullptr;
                        return false;
                    }
                    // add the condition to the transition
                    emfxTransition->AddCondition(emfxCondition);
                }

                //emfxTransition->Init( animGraph );
            }
            // something went wrong with creating the transition
            else
            {
                MCore::LogWarning("Cannot load and instantiate state transition. State transition from %d to %d will be skipped.", transition.mSourceNode, transition.mDestNode);

                // skip reading the attributes
                if (!ForwardAttributes(file, importParams.mEndianType, nodeHeader.mNumAttributes))
                {
                    return false;
                }

                // skip reading the node custom data
                if (file->Forward(nodeHeader.mNumCustomDataBytes) == false)
                {
                    return false;
                }

                // iterate through all conditions and skip them as well
                for (uint32 c = 0; c < transition.mNumConditions; ++c)
                {
                    // read the condition node header
                    FileFormat::AnimGraph_NodeHeader conditionHeader;
                    file->Read(&conditionHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

                    // convert endian
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mTypeID,              importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mVersion,             importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumCustomDataBytes,  importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumAttributes,       importParams.mEndianType);

                    // skip reading the attributes
                    if (!ForwardAttributes(file, importParams.mEndianType, conditionHeader.mNumAttributes))
                    {
                        return false;
                    }

                    // skip reading the node custom data
                    if (file->Forward(conditionHeader.mNumCustomDataBytes) == false)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph state transitions
    bool ChunkProcessorAnimGraphAdditionalInfo::Process(MCore::File* file, Importer::ImportParameters& /*importParams*/)
    {
        return file->Forward(sizeof(FileFormat::AnimGraph_AdditionalInfo));
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph node connections
    bool ChunkProcessorAnimGraphNodeConnections::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        // read the number of transitions to follow
        uint32 numConnections;
        file->Read(&numConnections, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numConnections, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num node connections = %d", numConnections);
        }

        // get the array of currently loaded nodes
        MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(importParams.mSharedData);

        // read the connections
        FileFormat::AnimGraph_NodeConnection connection;
        for (uint32 i = 0; i < numConnections; ++i)
        {
            // read the transition
            file->Read(&connection, sizeof(FileFormat::AnimGraph_NodeConnection));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&connection.mSourceNode,       importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&connection.mTargetNode,       importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt16(&connection.mSourceNodePort,   importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt16(&connection.mTargetNodePort,   importParams.mEndianType);

            // log details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + Connection #%d = From node %d (port id %d) into node %d (port id %d)", i, connection.mSourceNode, connection.mSourceNodePort, connection.mTargetNode, connection.mTargetNodePort);
            }

            // get the source and the target node and check if they are valid
            AnimGraphNode* sourceNode = blendNodes[connection.mSourceNode];
            AnimGraphNode* targetNode = blendNodes[connection.mTargetNode];
            if (sourceNode == nullptr || targetNode == nullptr)
            {
                MCore::LogWarning("EMotionFX::ChunkProcessorAnimGraphNodeConnections() - Connection cannot be created because the source or target node is invalid! (sourcePortID=%d targetPortID=%d sourceNode=%d targetNode=%d)", connection.mSourceNodePort, connection.mTargetNodePort, connection.mSourceNode, connection.mTargetNode);
                continue;
            }

            // create the connection
            const uint32 sourcePort = blendNodes[connection.mSourceNode]->FindOutputPortByID(connection.mSourceNodePort);
            const uint32 targetPort = blendNodes[connection.mTargetNode]->FindInputPortByID(connection.mTargetNodePort);
            if (sourcePort != MCORE_INVALIDINDEX32 && targetPort != MCORE_INVALIDINDEX32)
            {
                blendNodes[connection.mTargetNode]->AddConnection(blendNodes[connection.mSourceNode], static_cast<uint16>(sourcePort), static_cast<uint16>(targetPort));
            }
            else
            {
                MCore::LogWarning("EMotionFX::ChunkProcessorAnimGraphNodeConnections() - Connection cannot be created because the source or target port doesn't exist! (sourcePortID=%d targetPortID=%d sourceNode='%s' targetNode=%s')", connection.mSourceNodePort, connection.mTargetNodePort, blendNodes[connection.mSourceNode]->GetName(), blendNodes[connection.mTargetNode]->GetName());
            }
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph node
    bool ChunkProcessorAnimGraphNode::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the node header
        FileFormat::AnimGraph_NodeHeader nodeHeader;
        file->Read(&nodeHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mTypeID,               importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mParentIndex,          importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVersion,              importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumCustomDataBytes,   importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumChildNodes,        importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumAttributes,        importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVisualizeColor,       importParams.mEndianType);
        MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosX,           importParams.mEndianType);
        MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosY,           importParams.mEndianType);

        const char* nodeName = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Blend Node:");
            MCore::LogDetailedInfo("  + Name            = %s", nodeName);
            MCore::LogDetailedInfo("  + Parent index    = %d", nodeHeader.mParentIndex);
            MCore::LogDetailedInfo("  + Type            = %d", nodeHeader.mTypeID);
            MCore::LogDetailedInfo("  + Version         = %d", nodeHeader.mVersion);
            MCore::LogDetailedInfo("  + Num data bytes  = %d", nodeHeader.mNumCustomDataBytes);
            MCore::LogDetailedInfo("  + Num child nodes = %d", nodeHeader.mNumChildNodes);
            MCore::LogDetailedInfo("  + Num attributes  = %d", nodeHeader.mNumAttributes);
            MCore::LogDetailedInfo("  + Visualize Color = %d, %d, %d", MCore::ExtractRed(nodeHeader.mVisualizeColor), MCore::ExtractGreen(nodeHeader.mVisualizeColor), MCore::ExtractBlue(nodeHeader.mVisualizeColor));
            MCore::LogDetailedInfo("  + Visual pos      = (%d, %d)", nodeHeader.mVisualPosX, nodeHeader.mVisualPosY);
            MCore::LogDetailedInfo("  + Collapsed       = %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_COLLAPSED) ? "Yes" : "No");
            MCore::LogDetailedInfo("  + Visualized      = %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_VISUALIZED) ? "Yes" : "No");
            MCore::LogDetailedInfo("  + Disabled        = %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_DISABLED) ? "Yes" : "No");
            MCore::LogDetailedInfo("  + Virtual FinalOut= %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_VIRTUALFINALOUTPUT) ? "Yes" : "No");
        }

        AnimGraphNode* node = nullptr;
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNodeChunk(file
                , importParams
                , nodeName
                , nodeHeader
                , node))
        {
            if (importParams.mAnimGraph->GetRootStateMachine() == node)
            {
                importParams.mAnimGraph->SetRootStateMachine(nullptr);
            }

            if (node)
            {
                AnimGraphNode* parentNode = node->GetParentNode();
                if (parentNode)
                {
                    parentNode->RemoveChildNodeByPointer(node, false);
                }
            }

            delete node;
            node = nullptr;
            return false;
        }

        EMotionFX::GetEventManager().OnCreatedNode(animGraph, node);

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph parameters
    bool ChunkProcessorAnimGraphParameters::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the number of parameters
        uint32 numParams;
        file->Read(&numParams, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numParams, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num parameters = %d", numParams);
        }

        // read all parameters
        for (uint32 p = 0; p < numParams; ++p)
        {
            // read the parameter info header
            FileFormat::AnimGraph_ParameterInfo paramInfo;
            file->Read(&paramInfo, sizeof(FileFormat::AnimGraph_ParameterInfo));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&paramInfo.mNumComboValues,    importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&paramInfo.mInterfaceType,     importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&paramInfo.mAttributeType,     importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt16(&paramInfo.mFlags,             importParams.mEndianType);

            // check the attribute type
            const uint32 attribType = paramInfo.mAttributeType;
            if (attribType == 0)
            {
                MCore::LogError("EMotionFX::ChunkProcessorAnimGraphParameters::Process() - Failed to convert interface type %d to an attribute type.", attribType);
                return false;
            }

            const AZ::TypeId parameterTypeId = EMotionFX::FileFormat::GetParameterTypeIdForInterfaceType(paramInfo.mInterfaceType);
            const AZStd::string name = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            AZStd::unique_ptr<EMotionFX::Parameter> newParam(EMotionFX::ParameterFactory::Create(parameterTypeId));
            AZ_Assert(azrtti_istypeof<EMotionFX::ValueParameter>(newParam.get()), "Expected a value parameter");

            if (!newParam)
            {
                MCore::LogError("EMotionFX::ChunkProcessorAnimGraphParameters::Process() - Failed to create parameter: '%s'.", name.c_str());
                return false;
            }

            // read the strings
            newParam->SetName(name);
            SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType); // We dont use internal name anymore
            newParam->SetDescription(SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType));

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Parameter #%d:", p);
                MCore::LogDetailedInfo("  + Name           = %s", newParam->GetName().c_str());
                MCore::LogDetailedInfo("  + Description    = %s", newParam->GetDescription().c_str());
                MCore::LogDetailedInfo("  + type           = %s", newParam->RTTI_GetTypeName());
                MCore::LogDetailedInfo("  + Attribute type = %d", paramInfo.mAttributeType);
                MCore::LogDetailedInfo("  + Has MinMax     = %d", paramInfo.mHasMinMax);
                MCore::LogDetailedInfo("  + Flags          = %d", paramInfo.mFlags);
            }

            MCore::Attribute* attr(MCore::GetAttributeFactory().CreateAttributeByType(attribType));
            EMotionFX::ValueParameter* valueParameter = static_cast<EMotionFX::ValueParameter*>(newParam.get());

            // create the min, max and default value attributes
            if (paramInfo.mHasMinMax == 1)
            {
                // min value
                attr->Read(file, importParams.mEndianType);
                valueParameter->SetMinValueFromAttribute(attr);

                // max value
                attr->Read(file, importParams.mEndianType);
                valueParameter->SetMaxValueFromAttribute(attr);
            }

            // default value
            attr->Read(file, importParams.mEndianType);
            valueParameter->SetDefaultValueFromAttribute(attr);
            delete attr;

            // Parameters were previously stored in "AttributeSettings". The calss supported
            // multiple values, however, the UI did not, so this ended up not being used.
            // Support for multiple values in parameters is possible, however we dont need it now.
            // Leaving this code as reference
            for (uint32 i = 0; i < paramInfo.mNumComboValues; ++i)
            {
                SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            }

            if (!animGraph->AddParameter(newParam.get()))
            {
                MCore::LogError("EMotionFX::ChunkProcessorAnimGraphParameters::Process() - Failed to add parameter: '%s'.", name.c_str());
                return false;
            }
            newParam.release(); // ownership moved to animGraph
        }

        return true;
    }


    // animGraph node groups
    bool ChunkProcessorAnimGraphNodeGroups::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the number of node groups
        uint32 numNodeGroups;
        file->Read(&numNodeGroups, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numNodeGroups, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num Node Groups = %d", numNodeGroups);
        }

        // read all node groups
        for (uint32 g = 0; g < numNodeGroups; ++g)
        {
            // read the node group header
            FileFormat::AnimGraph_NodeGroup nodeGroupChunk;
            file->Read(&nodeGroupChunk, sizeof(FileFormat::AnimGraph_NodeGroup));

            MCore::RGBAColor emfxColor(nodeGroupChunk.mColor.mR, nodeGroupChunk.mColor.mG, nodeGroupChunk.mColor.mB, nodeGroupChunk.mColor.mA);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&nodeGroupChunk.mNumNodes, importParams.mEndianType);
            MCore::Endian::ConvertRGBAColor(&emfxColor,                    importParams.mEndianType);

            const AZ::Color color128 = MCore::EmfxColorToAzColor(emfxColor);
            const AZ::u32 color32 = color128.ToU32();

            const char* groupName   = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            const uint32    numNodes    = nodeGroupChunk.mNumNodes;

            // create and fill the new node group
            AnimGraphNodeGroup* nodeGroup = aznew AnimGraphNodeGroup(groupName);
            animGraph->AddNodeGroup(nodeGroup);
            nodeGroup->SetIsVisible(nodeGroupChunk.mIsVisible != 0);
            nodeGroup->SetColor(color32);

            // set the nodes of the node group
            MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(importParams.mSharedData);
            nodeGroup->SetNumNodes(numNodes);
            for (uint32 i = 0; i < numNodes; ++i)
            {
                // read the node index of the current node inside the group
                uint32 nodeNr;
                file->Read(&nodeNr, sizeof(uint32));
                MCore::Endian::ConvertUnsignedInt32(&nodeNr, importParams.mEndianType);

                MCORE_ASSERT(nodeNr != MCORE_INVALIDINDEX32);

                // set the id of the given node to the group
                if (nodeNr != MCORE_INVALIDINDEX32 && blendNodes[nodeNr])
                {
                    nodeGroup->SetNode(i, blendNodes[nodeNr]->GetId());
                }
                else
                {
                    nodeGroup->SetNode(i, AnimGraphNodeId::InvalidId);
                }
            }

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Node Group #%d:", g);
                MCore::LogDetailedInfo("  + Name           = %s", nodeGroup->GetName());
                MCore::LogDetailedInfo("  + Color          = (%.2f, %.2f, %.2f, %.2f)", static_cast<float>(color128.GetR()), static_cast<float>(color128.GetG()), static_cast<float>(color128.GetB()), static_cast<float>(color128.GetA()));
                MCore::LogDetailedInfo("  + Num Nodes      = %i", nodeGroup->GetNumNodes());
            }
        }

        return true;
    }


    // animGraph group parameters
    bool ChunkProcessorAnimGraphGroupParameters::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the number of group parameters
        uint32 numGroupParameters;
        file->Read(&numGroupParameters, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numGroupParameters, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num group parameters = %d", numGroupParameters);
        }

        // Group parameters is going to re-shuffle the value parameter indices, therefore we
        // need to update the connections downstream of parameter nodes.
        EMotionFX::ValueParameterVector valueParametersBeforeChange = animGraph->RecursivelyGetValueParameters();

        // Since relocating a parameter to another parent changes its index, we are going to
        // compute all the relationships leaving the value parameters at the root, then relocate
        // them.
        AZStd::vector<AZStd::pair<const EMotionFX::GroupParameter*, EMotionFX::ParameterVector> > parametersByGroup;

        // read all group parameters
        for (uint32 g = 0; g < numGroupParameters; ++g)
        {
            // read the group parameter header
            FileFormat::AnimGraph_GroupParameter groupChunk;
            file->Read(&groupChunk, sizeof(FileFormat::AnimGraph_GroupParameter));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&groupChunk.mNumParameters, importParams.mEndianType);

            const char* groupName = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            const uint32 numParameters = groupChunk.mNumParameters;

            // create and fill the new group parameter
            AZStd::unique_ptr<EMotionFX::Parameter> parameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<EMotionFX::GroupParameter>()));
            parameter->SetName(groupName);

            // Previously collapsed/expanded state in group parameters was stored in the animgraph file. However, that
            // would require to check out the animgraph file if you expand/collapse a group. Because this change was not
            // done through commands, the dirty state was not properly restored.
            // Collapsing state should be more of a setting per-user than something saved in the animgraph
            //groupParameter->SetIsCollapsed(groupChunk.mCollapsed != 0);

            if (!animGraph->AddParameter(parameter.get()))
            {
                continue;
            }
            const EMotionFX::GroupParameter* groupParameter = static_cast<EMotionFX::GroupParameter*>(parameter.release());

            parametersByGroup.emplace_back(groupParameter, EMotionFX::ParameterVector());
            AZStd::vector<EMotionFX::Parameter*>& parametersInGroup = parametersByGroup.back().second;

            // set the parameters of the group parameter
            for (uint32 i = 0; i < numParameters; ++i)
            {
                // read the parameter index
                uint32 parameterIndex;
                file->Read(&parameterIndex, sizeof(uint32));
                MCore::Endian::ConvertUnsignedInt32(&parameterIndex, importParams.mEndianType);

                MCORE_ASSERT(parameterIndex != MCORE_INVALIDINDEX32);
                if (parameterIndex != MCORE_INVALIDINDEX32)
                {
                    const EMotionFX::Parameter* childParameter = animGraph->FindValueParameter(parameterIndex);
                    parametersInGroup.emplace_back(const_cast<EMotionFX::Parameter*>(childParameter));
                }
            }

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Group parameter #%d:", g);
                MCore::LogDetailedInfo("  + Name           = %s", groupParameter->GetName().c_str());
                MCore::LogDetailedInfo("  + Num Parameters = %i", groupParameter->GetNumParameters());
            }
        }

        // Now move the parameters to their groups
        for (const AZStd::pair<const EMotionFX::GroupParameter*, EMotionFX::ParameterVector>& groupAndParameters : parametersByGroup)
        {
            const EMotionFX::GroupParameter* groupParameter = groupAndParameters.first;
            for (EMotionFX::Parameter* parameter : groupAndParameters.second)
            {
                animGraph->TakeParameterFromParent(parameter);
                animGraph->AddParameter(const_cast<EMotionFX::Parameter*>(parameter), groupParameter);
            }
        }

        const EMotionFX::ValueParameterVector valueParametersAfterChange = animGraph->RecursivelyGetValueParameters();

        AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
        animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);

        for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
        {
            EMotionFX::ObjectAffectedByParameterChanges* affectedObjectByParameterChanges = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
            affectedObjectByParameterChanges->ParameterOrderChanged(valueParametersBeforeChange, valueParametersAfterChange);
        }

        return true;
    }


    // animGraph game controller settings
    bool ChunkProcessorAnimGraphGameControllerSettings::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        uint32 i;

        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // get the game controller settings for the anim graph and clear it
        AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();
        gameControllerSettings.Clear();

        // read the number of presets and the active preset index
        uint32 activePresetIndex, numPresets;
        file->Read(&activePresetIndex, sizeof(uint32));
        file->Read(&numPresets, sizeof(uint32));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&activePresetIndex, importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&numPresets, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Game Controller Settings (NumPresets=%d, ActivePreset=%d)", numPresets, activePresetIndex);
        }

        // preallocate memory for the presets
        gameControllerSettings.SetNumPresets(numPresets);

        // read all presets
        for (uint32 p = 0; p < numPresets; ++p)
        {
            // read the preset chunk
            FileFormat::AnimGraph_GameControllerPreset presetChunk;
            file->Read(&presetChunk, sizeof(FileFormat::AnimGraph_GameControllerPreset));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&presetChunk.mNumParameterInfos, importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&presetChunk.mNumButtonInfos, importParams.mEndianType);

            // read the preset name and get the number of parameter and button infos
            const char*     presetName      = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            const uint32    numParamInfos   = presetChunk.mNumParameterInfos;
            const uint32    numButtonInfos  = presetChunk.mNumButtonInfos;

            // create and fill the new preset
            AnimGraphGameControllerSettings::Preset* preset = aznew AnimGraphGameControllerSettings::Preset(presetName);
            gameControllerSettings.SetPreset(p, preset);

            // read the parameter infos
            preset->SetNumParamInfos(numParamInfos);
            for (i = 0; i < numParamInfos; ++i)
            {
                // read the parameter info chunk
                FileFormat::AnimGraph_GameControllerParameterInfo paramInfoChunk;
                file->Read(&paramInfoChunk, sizeof(FileFormat::AnimGraph_GameControllerParameterInfo));

                // read the parameter name
                const char* parameterName = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);

                // construct and fill the parameter info
                AnimGraphGameControllerSettings::ParameterInfo* parameterInfo = aznew AnimGraphGameControllerSettings::ParameterInfo(parameterName);
                parameterInfo->m_axis    = paramInfoChunk.mAxis;
                parameterInfo->m_invert  = (paramInfoChunk.mInvert != 0);
                parameterInfo->m_mode    = (AnimGraphGameControllerSettings::ParameterMode)paramInfoChunk.mMode;

                preset->SetParamInfo(i, parameterInfo);
            }

            // read the button infos
            preset->SetNumButtonInfos(numButtonInfos);
            for (i = 0; i < numButtonInfos; ++i)
            {
                // read the button info chunk
                FileFormat::AnimGraph_GameControllerButtonInfo buttonInfoChunk;
                file->Read(&buttonInfoChunk, sizeof(FileFormat::AnimGraph_GameControllerButtonInfo));

                // read the button string
                const char* buttonString = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);

                // construct and fill the button info
                AnimGraphGameControllerSettings::ButtonInfo* buttonInfo = aznew AnimGraphGameControllerSettings::ButtonInfo(buttonInfoChunk.mButtonIndex);
                buttonInfo->m_mode   = (AnimGraphGameControllerSettings::ButtonMode)buttonInfoChunk.mMode;
                buttonInfo->m_string = buttonString;

                preset->SetButtonInfo(i, buttonInfo);
            }

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Preset '%s':", preset->GetName());
                MCore::LogDetailedInfo("  + Num Param Infos  = %d", preset->GetNumParamInfos());
                MCore::LogDetailedInfo("  + Num Button Infos = %d", preset->GetNumButtonInfos());
            }
        }

        // set the active preset
        if (activePresetIndex != MCORE_INVALIDINDEX32)
        {
            AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetPreset(activePresetIndex);
            gameControllerSettings.SetActivePreset(activePreset);
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------
    // MotionSet
    //----------------------------------------------------------------------------------------------------------

    // all submotions in one chunk
    bool ChunkProcessorMotionSet::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;

        FileFormat::MotionSetsChunk motionSetsChunk;
        file->Read(&motionSetsChunk, sizeof(FileFormat::MotionSetsChunk));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&motionSetsChunk.mNumSets, endianType);

        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = motionSetsChunk.mNumSets;
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            FileFormat::MotionSetChunk motionSetChunk;
            file->Read(&motionSetChunk, sizeof(FileFormat::MotionSetChunk));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&motionSetChunk.mNumChildSets, endianType);
            MCore::Endian::ConvertUnsignedInt32(&motionSetChunk.mNumMotionEntries, endianType);

            // get the parent set
            const char* parentSetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            GetMotionManager().Lock();
            MotionSet* parentSet = GetMotionManager().FindMotionSetByName(parentSetName, importParams.m_isOwnedByRuntime);
            GetMotionManager().Unlock();

            // read the motion set name and create our new motion set
            const char* motionSetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            MotionSet* motionSet = aznew MotionSet(motionSetName, parentSet);
            motionSet->SetIsOwnedByRuntime(importParams.m_isOwnedByRuntime);

            // set the root motion set to the importer params motion set, this will be returned by the Importer::LoadMotionSet() function
            if (parentSet == nullptr)
            {
                assert(importParams.mMotionSet == nullptr);
                importParams.mMotionSet = motionSet;
            }

            // read the filename and set it
            /*const char* motionSetFileName = */ SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            //motionSet->SetFileName( motionSetFileName );

            // in case this is not a root motion set add the new motion set as child set to the parent set
            if (parentSet)
            {
                parentSet->AddChildSet(motionSet);
            }

            // Read all motion entries.
            const uint32 numMotionEntries = motionSetChunk.mNumMotionEntries;
            motionSet->ReserveMotionEntries(numMotionEntries);
            AZStd::string nativeMotionFileName;
            for (uint32 j = 0; j < numMotionEntries; ++j)
            {
                // read the motion entry
                const char* motionFileName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
                nativeMotionFileName = motionFileName;

                // read the string id and set it
                const char* motionStringID = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

                // add the motion entry to the motion set
                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(nativeMotionFileName.c_str(), motionStringID);
                motionSet->AddMotionEntry(motionEntry);
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
