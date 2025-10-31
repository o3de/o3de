/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/File.h>
#include <MCore/Source/DiskFile.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/MemoryFile.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Importer/ChunkProcessors.h>
#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <EMotionFX/Source/Importer/MotionFileFormat.h>
#include <EMotionFX/Source/Importer/NodeMapFileFormat.h>

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/VertexAttributeLayerAbstractData.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/NodeMap.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Importer, ImporterAllocator)


    // constructor
    Importer::Importer()
        : MCore::RefCounted()
    {
        // register all standard chunks
        RegisterStandardChunks();

        // init some default values
        m_loggingActive  = true;
        m_logDetails     = false;
    }


    // destructor
    Importer::~Importer()
    {
        // remove all chunk processors
        for (ChunkProcessor* chunkProcessor : m_chunkProcessors)
        {
            chunkProcessor->Destroy();
        }
    }


    Importer* Importer::Create()
    {
        return aznew Importer();
    }

    //-------------------------------------------------------------------------------------------------

    // check if we can process the given actor file
    bool Importer::CheckIfIsValidActorFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const
    {
        MCORE_ASSERT(f->GetIsOpen());

        // verify if we actually are dealing with a valid actor file
        FileFormat::Actor_Header  header;
        if (f->Read(&header, sizeof(FileFormat::Actor_Header)) == 0)
        {
            MCore::LogError("Failed to read the actor file header!");
            return false;
        }

        // check the FOURCC
        if (header.m_fourcc[0] != 'A' || header.m_fourcc[1] != 'C' || header.m_fourcc[2] != 'T' || header.m_fourcc[3] != 'R')
        {
            return false;
        }

        // read the chunks
        switch (header.m_endianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.m_endianType);
            return false;
        }

        // yes, it is a valid actor file!
        return true;
    }



    // check if we can process the given motion file
    bool Importer::CheckIfIsValidMotionFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const
    {
        MCORE_ASSERT(f->GetIsOpen());

        // verify if we actually are dealing with a valid actor file
        FileFormat::Motion_Header  header;
        if (f->Read(&header, sizeof(FileFormat::Motion_Header)) == 0)
        {
            MCore::LogError("Failed to read the motion file header!");
            return false;
        }

        // check the FOURCC
        if ((header.m_fourcc[0] != 'M' || header.m_fourcc[1] != 'O' || header.m_fourcc[2] != 'T' || header.m_fourcc[3] != ' '))
        {
            return false;
        }

        // read the chunks
        switch (header.m_endianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.m_endianType);
            return false;
        }

        // yes, it is a valid motion file!
        return true;
    }

    // check if we can process the given node map file
    bool Importer::CheckIfIsValidNodeMapFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const
    {
        MCORE_ASSERT(f->GetIsOpen());

        // verify if we actually are dealing with a valid actor file
        FileFormat::NodeMap_Header  header;
        if (f->Read(&header, sizeof(FileFormat::NodeMap_Header)) == 0)
        {
            MCore::LogError("Failed to read the node map file header!");
            return false;
        }

        // check the FOURCC
        if (header.m_fourCc[0] != 'N' || header.m_fourCc[1] != 'O' || header.m_fourCc[2] != 'M' || header.m_fourCc[3] != 'P')
        {
            return false;
        }

        // read the chunks
        switch (header.m_endianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.m_endianType);
            return false;
        }

        // yes, it is a valid node map file!
        return true;
    }


    // try to load an actor from disk
    AZStd::unique_ptr<Actor> Importer::LoadActor(AZStd::string filename, ActorSettings* settings)
    {
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        if (GetLogging())
        {
            MCore::LogInfo("- Trying to load actor from file '%s'...", filename.c_str());
        }

        // try to open the file from disk
        MCore::DiskFile f;
        if (!f.Open(filename.c_str(), MCore::DiskFile::READ))
        {
            if (GetLogging())
            {
                MCore::LogError("  + Failed to open the file for actor '%s', actor not loaded!", filename.c_str());
            }
            return nullptr;
        }

        // retrieve the filesize
        const size_t fileSize = f.GetFileSize();

        // create a temporary buffer for the file
        uint8* fileBuffer = (uint8*)MCore::Allocate(fileSize, EMFX_MEMCATEGORY_IMPORTER);

        // read in the complete file
        f.Read(fileBuffer, fileSize);

        // close the file again
        f.Close();

        // create the actor reading from memory
        AZStd::unique_ptr<Actor> result = LoadActor(fileBuffer, fileSize, settings, filename.c_str());

        // delete the file buffer again
        MCore::Free(fileBuffer);

        // check if it worked :)
        if (result == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("   + Failed to load actor from file '%s'", filename.c_str());
            }
        }
        else
        {
            if (GetLogging())
            {
                MCore::LogInfo("  + Loading successfully finished");
            }
        }

        // return the result
        return result;
    }


    // load an actor from memory
    AZStd::unique_ptr<Actor> Importer::LoadActor(uint8* memoryStart, size_t lengthInBytes, ActorSettings* settings, const char* filename)
    {
        // create the memory file
        MCore::MemoryFile memFile;
        memFile.Open(memoryStart, lengthInBytes);

        // try to load the file
        AZStd::unique_ptr<Actor> result = LoadActor(&memFile, settings, filename);
        if (result == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("Failed to load actor from memory location 0x%x", memoryStart);
            }
        }

        // close the memory file again
        memFile.Close();

        // return the result
        return result;
    }


    // try to load a actor file
    AZStd::unique_ptr<Actor> Importer::LoadActor(MCore::File* f, ActorSettings* settings, const char* filename)
    {
        MCORE_ASSERT(f);
        MCORE_ASSERT(f->GetIsOpen());

        // create the shared data
        AZStd::vector<SharedData*> sharedData;
        PrepareSharedData(sharedData);

        // verify if this is a valid actor file or not
        MCore::Endian::EEndianType endianType = MCore::Endian::ENDIAN_LITTLE;
        if (CheckIfIsValidActorFile(f, &endianType) == false)
        {
            MCore::LogError("The specified file is not a valid EMotion FX actor file!");
            ResetSharedData(sharedData);
            f->Close();
            return nullptr;
        }

        // copy over the actor settings, or use defaults
        ActorSettings actorSettings;
        if (settings)
        {
            actorSettings = *settings;
        }

        if (actorSettings.m_optimizeForServer)
        {
            actorSettings.OptimizeForServer();
        }

        // create the actor
        AZStd::unique_ptr<Actor> actor = AZStd::make_unique<Actor>("Unnamed actor");
        MCORE_ASSERT(actor);

        if (actor)
        {
            actor->SetThreadIndex(actorSettings.m_threadIndex);

            // set the scale mode
            // actor->SetScaleMode( scaleMode );

            // init the import parameters
            ImportParameters params;
            params.m_sharedData = &sharedData;
            params.m_endianType = endianType;
            params.m_actorSettings = &actorSettings;
            params.m_actor = actor.get();

            // process all chunks
            while (ProcessChunk(f, params))
            {
            }

            actor->SetFileName(filename);

            // Generate an optimized version of skeleton for server.
            if (actorSettings.m_optimizeForServer && actor->GetOptimizeSkeleton())
            {
                actor->GenerateOptimizedSkeleton();
            }

            // post create init
            actor->PostCreateInit(actorSettings.m_makeGeomLoDsCompatibleWithSkeletalLoDs, actorSettings.m_unitTypeConvert);
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.clear();

        // return the created actor
        return actor;
    }

    //-------------------------------------------------------------------------------------------------

    // try to load a motion from disk
    Motion* Importer::LoadMotion(AZStd::string filename, MotionSettings* settings)
    {
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);

        // check if we want to load the motion even if a motion with the given filename is already inside the motion manager
        if (settings == nullptr || (settings && settings->m_forceLoading == false))
        {
            // search the motion inside the motion manager and return it if it already got loaded
            Motion* motion = GetMotionManager().FindMotionByFileName(filename.c_str());
            if (motion)
            {
                motion->IncreaseReferenceCount();
                MCore::LogInfo("  + Motion '%s' already loaded, returning already loaded motion from the MotionManager.", filename.c_str());
                return motion;
            }
        }

        if (GetLogging())
        {
            MCore::LogInfo("- Trying to load motion from file '%s'...", filename.c_str());
        }

        // try to open the file from disk
        MCore::DiskFile f;
        if (f.Open(filename.c_str(), MCore::DiskFile::READ) == false)
        {
            if (GetLogging())
            {
                MCore::LogError("  + Failed to open the file for motion '%s'!", filename.c_str());
            }
            return nullptr;
        }

        // retrieve the filesize
        const size_t fileSize = f.GetFileSize();

        // create a temporary buffer for the file
        uint8* fileBuffer = (uint8*)MCore::Allocate(fileSize, EMFX_MEMCATEGORY_IMPORTER);

        // read in the complete file
        f.Read(fileBuffer, fileSize);

        // close the file again
        f.Close();

        // create the motion reading from memory
        Motion* result = LoadMotion(fileBuffer, fileSize, settings);
        if (result)
        {
            result->SetFileName(filename.c_str());
        }

        // delete the filebuffer again
        MCore::Free(fileBuffer);

        // check if it worked :)
        if (result == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("  + Failed to load motion from file '%s'", filename.c_str());
            }
        }
        else
        {
            if (GetLogging())
            {
                MCore::LogInfo("  + Loading successfully finished");
            }
        }

        // return the result
        return result;
    }


    Motion* Importer::LoadMotion(uint8* memoryStart, size_t lengthInBytes, MotionSettings* settings)
    {
        MCore::MemoryFile memFile;
        memFile.Open(memoryStart, lengthInBytes);
        Motion* result = LoadMotion(&memFile, settings);
        return result;
    }


    // try to load a motion from an MCore file
    Motion* Importer::LoadMotion(MCore::File* f, MotionSettings* settings)
    {
        MCORE_ASSERT(f);
        MCORE_ASSERT(f->GetIsOpen());

        // create the shared data
        AZStd::vector<SharedData*> sharedData;
        PrepareSharedData(sharedData);

        // verify if this is a valid actor file or not
        MCore::Endian::EEndianType endianType = MCore::Endian::ENDIAN_LITTLE;
        if (!CheckIfIsValidMotionFile(f, &endianType))
        {
            ResetSharedData(sharedData);
            f->Close();
            return nullptr;
        }

        // create our motion
        Motion* motion = aznew Motion("<Unknown>");

        // copy over the actor settings, or use defaults
        MotionSettings motionSettings;
        if (settings)
        {
            motionSettings = *settings;
        }

        // fix any possible conflicting settings
        ValidateMotionSettings(&motionSettings);

        // init the import parameters
        ImportParameters params;
        params.m_sharedData     = &sharedData;
        params.m_endianType     = endianType;
        params.m_motionSettings = &motionSettings;
        params.m_motion         = motion;

        // read the chunks
        while (ProcessChunk(f, params))
        {
        }

        // make sure there is a sync track
        motion->GetEventTable()->AutoCreateSyncTrack(motion);

        // scale to the EMotion FX unit type
        if (motionSettings.m_unitTypeConvert)
        {
            motion->ScaleToUnitType(GetEMotionFX().GetUnitType());
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.clear();

        return motion;
    }

    //-------------------------------------------------------------------------------------------------

    MotionSet* Importer::LoadMotionSet(AZStd::string filename, MotionSetSettings* settings, const AZ::ObjectStream::FilterDescriptor& loadFilter)
    {
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        EMotionFX::MotionSet* motionSet = EMotionFX::MotionSet::LoadFromFile(filename, context, loadFilter);
        if (motionSet)
        {
            motionSet->SetFilename(filename.c_str());
            if (settings)
            {
                motionSet->SetIsOwnedByRuntime(settings->m_isOwnedByRuntime);
            }
        }
        return motionSet;
    }

    MotionSet* Importer::LoadMotionSet(uint8* memoryStart, size_t lengthInBytes, MotionSetSettings* settings)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        EMotionFX::MotionSet* motionSet = EMotionFX::MotionSet::LoadFromBuffer(memoryStart, lengthInBytes, context);
        if (motionSet)
        {
            if (settings)
            {
                motionSet->SetIsOwnedByRuntime(settings->m_isOwnedByRuntime);
            }
        }
        return motionSet;
    }

    //-------------------------------------------------------------------------------------------------

    // load a node map by filename
    NodeMap* Importer::LoadNodeMap(AZStd::string filename, NodeMapSettings* settings)
    {
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);

        if (GetLogging())
        {
            MCore::LogInfo("- Trying to load node map from file '%s'...", filename.c_str());
        }

        // try to open the file from disk
        MCore::DiskFile f;
        if (!f.Open(filename.c_str(), MCore::DiskFile::READ))
        {
            if (GetLogging())
            {
                MCore::LogError("  + Failed to open the file for node map '%s', file not loaded!", filename.c_str());
            }
            return nullptr;
        }

        // retrieve the filesize
        const size_t fileSize = f.GetFileSize();

        // create a temporary buffer for the file
        uint8* fileBuffer = (uint8*)MCore::Allocate(fileSize, EMFX_MEMCATEGORY_IMPORTER);

        // read in the complete file
        f.Read(fileBuffer, fileSize);

        // close the file again
        f.Close();

        // create the actor reading from memory
        NodeMap* result = LoadNodeMap(fileBuffer, fileSize, settings);
        if (result)
        {
            result->SetFileName(filename.c_str());
        }

        // delete the filebuffer again
        MCore::Free(fileBuffer);

        // check if it worked :)
        if (GetLogging())
        {
            if (result == nullptr)
            {
                MCore::LogError("  + Failed to load node map from file '%s'", filename.c_str());
            }
            else
            {
                MCore::LogInfo("  + Loading successfully finished");
            }
        }

        // return the result
        return result;
    }


    // load the node map from memory
    NodeMap* Importer::LoadNodeMap(uint8* memoryStart, size_t lengthInBytes, NodeMapSettings* settings)
    {
        // create the memory file
        MCore::MemoryFile memFile;
        memFile.Open(memoryStart, lengthInBytes);

        // try to load the file
        NodeMap* result = LoadNodeMap(&memFile, settings);
        if (result == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("Failed to load node map from memory location 0x%x", memoryStart);
            }
        }

        // close the memory file again
        memFile.Close();

        // return the result
        return result;
    }


    // load a node map from a file object
    NodeMap* Importer::LoadNodeMap(MCore::File* f, NodeMapSettings* settings)
    {
        MCORE_ASSERT(f);
        MCORE_ASSERT(f->GetIsOpen());

        // execute the pre-passes
        if (f->GetType() != MCore::MemoryFile::TYPE_ID)
        {
            MCore::LogError("Given file is not a memory file. Cannot process pre-passes.");
            return nullptr;
        }

        // copy over the settings, or use defaults
        NodeMapSettings nodeMapSettings;
        if (settings)
        {
            nodeMapSettings = *settings;
        }

        // create the shared data
        AZStd::vector<SharedData*> sharedData;
        PrepareSharedData(sharedData);

        //-----------------------------------------------

        // load the file header
        FileFormat::NodeMap_Header fileHeader;
        f->Read(&fileHeader, sizeof(FileFormat::NodeMap_Header));
        if (fileHeader.m_fourCc[0] != 'N' || fileHeader.m_fourCc[1] != 'O' || fileHeader.m_fourCc[2] != 'M' || fileHeader.m_fourCc[3] != 'P')
        {
            MCore::LogError("The node map file is not a valid node map file.");
            f->Close();
            return nullptr;
        }

        // get the endian type
        MCore::Endian::EEndianType endianType = (MCore::Endian::EEndianType)fileHeader.m_endianType;

        // create the node map
        NodeMap* nodeMap = NodeMap::Create();

        // init the import parameters
        ImportParameters params;
        params.m_sharedData          = &sharedData;
        params.m_endianType          = endianType;
        params.m_nodeMap             = nodeMap;
        params.m_nodeMapSettings     = &nodeMapSettings;

        // process all chunks
        while (ProcessChunk(f, params))
        {
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.clear();

        // return the created actor
        return nodeMap;
    }

    //-------------------------------------------------------------------------------------------------

    // register a new chunk processor
    void Importer::RegisterChunkProcessor(ChunkProcessor* processorToRegister)
    {
        MCORE_ASSERT(processorToRegister);
        m_chunkProcessors.emplace_back(processorToRegister);
    }


    // add shared data object to the importer
    void Importer::AddSharedData(AZStd::vector<SharedData*>& sharedData, SharedData* data)
    {
        MCORE_ASSERT(data);
        sharedData.emplace_back(data);
    }


    // search for shared data
    SharedData* Importer::FindSharedData(AZStd::vector<SharedData*>* sharedDataArray, uint32 type)
    {
        // for all shared data
        const auto foundSharedData = AZStd::find_if(begin(*sharedDataArray), end(*sharedDataArray), [type](const SharedData* sharedData)
        {
            return sharedData->GetType() == type;
        });
        return foundSharedData != end(*sharedDataArray) ? *foundSharedData : nullptr;
    }


    void Importer::SetLoggingEnabled(bool enabled)
    {
        m_loggingActive = enabled;
    }


    bool Importer::GetLogging() const
    {
        return m_loggingActive;
    }


    void Importer::SetLogDetails(bool detailLoggingActive)
    {
        m_logDetails = detailLoggingActive;

        // set the processors logging flag
        for (ChunkProcessor* processor : m_chunkProcessors)
        {
             processor->SetLogging(m_loggingActive && detailLoggingActive); // only enable if logging is also enabled
        }
    }


    bool Importer::GetLogDetails() const
    {
        return m_logDetails;
    }


    void Importer::PrepareSharedData(AZStd::vector<SharedData*>& sharedData)
    {
        // create standard shared objects
        AddSharedData(sharedData, SharedHelperData::Create());
    }


    // reset shared objects so that the importer is ready for use again
    void Importer::ResetSharedData(AZStd::vector<SharedData*>& sharedData)
    {
        for (SharedData* data : sharedData)
        {
            data->Reset();
            data->Destroy();
        }
        sharedData.clear();
    }


    // find chunk, return nullptr if it hasn't been found
    ChunkProcessor* Importer::FindChunk(uint32 chunkID, uint32 version) const
    {
        // for all chunk processors
        const auto foundProcessor = AZStd::find_if(begin(m_chunkProcessors), end(m_chunkProcessors), [chunkID, version](const ChunkProcessor* processor)
        {
            return processor->GetChunkID() == chunkID && processor->GetVersion() == version;
        });
        return foundProcessor != end(m_chunkProcessors) ? *foundProcessor : nullptr;
    }


    // register basic chunk processors
    void Importer::RegisterStandardChunks()
    {
        // reserve space for 75 chunk processors
        m_chunkProcessors.reserve(75);

        // shared processors
        RegisterChunkProcessor(aznew ChunkProcessorMotionEventTrackTable());
        RegisterChunkProcessor(aznew ChunkProcessorMotionEventTrackTable2());
        RegisterChunkProcessor(aznew ChunkProcessorMotionEventTrackTable3());

        // Actor file format
        RegisterChunkProcessor(aznew ChunkProcessorActorInfo());
        RegisterChunkProcessor(aznew ChunkProcessorActorInfo2());
        RegisterChunkProcessor(aznew ChunkProcessorActorInfo3());
        RegisterChunkProcessor(aznew ChunkProcessorActorProgMorphTarget());
        RegisterChunkProcessor(aznew ChunkProcessorActorNodeGroups());
        RegisterChunkProcessor(aznew ChunkProcessorActorNodes2());
        RegisterChunkProcessor(aznew ChunkProcessorActorProgMorphTargets());
        RegisterChunkProcessor(aznew ChunkProcessorActorProgMorphTargets2());
        RegisterChunkProcessor(aznew ChunkProcessorActorNodeMotionSources());
        RegisterChunkProcessor(aznew ChunkProcessorActorAttachmentNodes());
        RegisterChunkProcessor(aznew ChunkProcessorActorPhysicsSetup());
        RegisterChunkProcessor(aznew ChunkProcessorActorSimulatedObjectSetup());
        RegisterChunkProcessor(aznew ChunkProcessorMeshAsset());

        // Motion file format
        RegisterChunkProcessor(aznew ChunkProcessorMotionInfo());
        RegisterChunkProcessor(aznew ChunkProcessorMotionInfo2());
        RegisterChunkProcessor(aznew ChunkProcessorMotionInfo3());
        RegisterChunkProcessor(aznew ChunkProcessorMotionSubMotions());
        RegisterChunkProcessor(aznew ChunkProcessorMotionMorphSubMotions());
        RegisterChunkProcessor(aznew ChunkProcessorMotionData());
        RegisterChunkProcessor(aznew ChunkProcessorRootMotionExtraction());

        // node map
        RegisterChunkProcessor(aznew ChunkProcessorNodeMap());
    }


    // find the chunk processor and read in the chunk
    bool Importer::ProcessChunk(MCore::File* file, ImportParameters& importParams)
    {
        // if we have reached the end of the file, exit
        if (file->GetIsEOF())
        {
            return false;
        }

        // try to read the chunk header
        FileFormat::FileChunk chunk;
        if (!file->Read(&chunk, sizeof(FileFormat::FileChunk)))
        {
            return false; // failed reading chunk
        }
        // convert endian
        const MCore::Endian::EEndianType endianType = importParams.m_endianType;
        MCore::Endian::ConvertUnsignedInt32(&chunk.m_chunkId, endianType);
        MCore::Endian::ConvertUnsignedInt32(&chunk.m_sizeInBytes, endianType);
        MCore::Endian::ConvertUnsignedInt32(&chunk.m_version, endianType);

        // try to find the chunk processor which can process this chunk
        ChunkProcessor* processor = FindChunk(chunk.m_chunkId, chunk.m_version);

        // if we cannot find the chunk, skip the chunk
        if (processor == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("Importer::ProcessChunk() - Unknown chunk (ID=%d  Size=%d bytes Version=%d), skipping...", chunk.m_chunkId, chunk.m_sizeInBytes, chunk.m_version);
            }
            file->Forward(chunk.m_sizeInBytes);

            return true;
        }

        // get some shortcuts
        Importer::ActorSettings* actorSettings = importParams.m_actorSettings;
        Importer::MotionSettings* skelMotionSettings = importParams.m_motionSettings;

        // check if we still want to skip the chunk or not
        bool mustSkip = false;

        // check if we specified to ignore this chunk
        if (actorSettings && AZStd::find(begin(actorSettings->m_chunkIDsToIgnore), end(actorSettings->m_chunkIDsToIgnore), chunk.m_chunkId) != end(actorSettings->m_chunkIDsToIgnore))
        {
            mustSkip = true;
        }

        if (skelMotionSettings && AZStd::find(begin(skelMotionSettings->m_chunkIDsToIgnore), end(skelMotionSettings->m_chunkIDsToIgnore), chunk.m_chunkId) != end(skelMotionSettings->m_chunkIDsToIgnore))
        {
            mustSkip = true;
        }

        // if we still don't need to skip
        if (mustSkip == false)
        {
            // if we're loading an actor
            if (actorSettings)
            {
                if ((actorSettings->m_loadLimits                 == false && chunk.m_chunkId == FileFormat::ACTOR_CHUNK_LIMIT) ||
                    (actorSettings->m_loadMorphTargets           == false && chunk.m_chunkId == FileFormat::ACTOR_CHUNK_STDPROGMORPHTARGET) ||
                    (actorSettings->m_loadMorphTargets           == false && chunk.m_chunkId == FileFormat::ACTOR_CHUNK_STDPMORPHTARGETS) ||
                    (actorSettings->m_loadSimulatedObjects       == false && chunk.m_chunkId == FileFormat::ACTOR_CHUNK_SIMULATEDOBJECTSETUP))
                {
                    mustSkip = true;
                }
            }

            // if we're loading a motion
            if (skelMotionSettings)
            {
                if (skelMotionSettings->m_loadMotionEvents       == false && chunk.m_chunkId == FileFormat::MOTION_CHUNK_MOTIONEVENTTABLE)
                {
                    mustSkip = true;
                }
            }
        }

        // if we want to skip this chunk
        if (mustSkip)
        {
            file->Forward(chunk.m_sizeInBytes);
            return true;
        }

        // try to process the chunk
        return processor->Process(file, importParams);
    }

    // make sure no settings conflict or can cause crashes
    void Importer::ValidateMotionSettings(MotionSettings* settings)
    {
        MCORE_UNUSED(settings);
    }

    // loads the given file and checks if it is an actor or motion
    Importer::EFileType Importer::CheckFileType(const char* filename)
    {
        // check for a valid name
        if (AZStd::string(filename).empty())
        {
            return FILETYPE_UNKNOWN;
        }

        AZStd::string fileExtension;
        AZ::StringFunc::Path::GetExtension(filename, fileExtension);

        if (fileExtension == ".animgraph")
        {
            return FILETYPE_ANIMGRAPH;
        }
        if (fileExtension == ".motionset")
        {
            return FILETYPE_MOTIONSET;
        }

        // try to open the file from disk
        MCore::MemoryFile memoryFile;
        memoryFile.Open();
        memoryFile.SetPreAllocSize(262144); // 256kb
        if (memoryFile.LoadFromDiskFile(filename) == false)
        {
            return FILETYPE_UNKNOWN;
        }

        if (memoryFile.GetFileSize() == 0)
        {
            return FILETYPE_UNKNOWN;
        }

        // check the file type
        return CheckFileType(&memoryFile);
    }


    // loads the given file and checks if it is an actor or a motion
    Importer::EFileType Importer::CheckFileType(MCore::File* file)
    {
        MCore::Endian::EEndianType  endianType = MCore::Endian::ENDIAN_LITTLE;

        // verify if this is a valid actor file or not
        file->Seek(0);
        if (CheckIfIsValidActorFile(file, &endianType))
        {
            file->Close();
            return FILETYPE_ACTOR;
        }

        // verify if this is a valid skeletal motion file or not
        file->Seek(0);
        if (CheckIfIsValidMotionFile(file, &endianType))
        {
            file->Close();
            return FILETYPE_MOTION;
        }

        // check for node map
        file->Seek(0);
        if (CheckIfIsValidNodeMapFile(file, &endianType))
        {
            file->Close();
            return FILETYPE_NODEMAP;
        }

        // close the file again
        file->Close();

        return FILETYPE_UNKNOWN;
    }

    //---------------------------------------------------------

    AnimGraph* Importer::LoadAnimGraph(AZStd::string filename, const AZ::ObjectStream::FilterDescriptor& loadFilter)
    {
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::NormalizePathKeepCase, filename);

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        EMotionFX::AnimGraph* animGraph = EMotionFX::AnimGraph::LoadFromFile(filename, context, loadFilter);
        if (animGraph)
        {
            animGraph->SetFileName(filename.c_str());
            animGraph->RemoveInvalidConnections(); // Remove connections that have nullptr source node's, which happens when connections point to unknown nodes.
        }

        return animGraph;
    }

    AnimGraph* Importer::LoadAnimGraph(uint8* memoryStart, size_t lengthInBytes)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        EMotionFX::AnimGraph* animGraph = EMotionFX::AnimGraph::LoadFromBuffer(memoryStart, lengthInBytes, context);
        return animGraph;
    }

    // extract the file information from an actor file
    bool Importer::ExtractActorFileInfo(FileInfo* outInfo, const char* filename) const
    {
        // open the file
        MCore::DiskFile file;
        if (file.Open(filename, MCore::DiskFile::READ) == false)
        {
            return false;
        }

        // read out the endian type
        MCore::Endian::EEndianType endianType = MCore::Endian::ENDIAN_LITTLE;
        if (CheckIfIsValidActorFile(&file, &endianType) == false)
        {
            MCore::LogError("The specified file is not a valid EMotion FX actor file!");
            file.Close();
            return false;
        }
        outInfo->m_endianType = endianType;

        // as we seeked to the end of the header and we know the second chunk always is the time stamp, we can read this now
        FileFormat::FileChunk fileChunk;
        file.Read(&fileChunk, sizeof(FileFormat::FileChunk));
        FileFormat::FileTime timeChunk;
        file.Read(&timeChunk, sizeof(FileFormat::FileTime));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileChunk.m_chunkId, endianType);
        MCore::Endian::ConvertUnsignedInt16(&timeChunk.m_year, endianType);

        return true;
    }


    // extract the file information from an motion file
    bool Importer::ExtractMotionFileInfo(FileInfo* outInfo, const char* filename) const
    {
        // open the file
        MCore::DiskFile file;
        if (file.Open(filename, MCore::DiskFile::READ) == false)
        {
            return false;
        }

        // read out the endian type
        MCore::Endian::EEndianType endianType = MCore::Endian::ENDIAN_LITTLE;
        if (!CheckIfIsValidMotionFile(&file, &endianType))
        {
            file.Close();
            return false;
        }
        outInfo->m_endianType = endianType;

        // as we seeked to the end of the header and we know the second chunk always is the time stamp, we can read this now
        FileFormat::FileChunk fileChunk;
        file.Read(&fileChunk, sizeof(FileFormat::FileChunk));
        FileFormat::FileTime timeChunk;
        file.Read(&timeChunk, sizeof(FileFormat::FileTime));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileChunk.m_chunkId, endianType);
        MCore::Endian::ConvertUnsignedInt16(&timeChunk.m_year, endianType);

        return true;
    }
} // namespace EMotionFX
