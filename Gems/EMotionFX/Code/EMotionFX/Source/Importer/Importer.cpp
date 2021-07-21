/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ_CLASS_ALLOCATOR_IMPL(Importer, ImporterAllocator, 0)


    // constructor
    Importer::Importer()
        : BaseObject()
    {
        // set the memory category
        mChunkProcessors.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);

        // register all standard chunks
        RegisterStandardChunks();

        // init some default values
        mLoggingActive  = true;
        mLogDetails     = false;
    }


    // destructor
    Importer::~Importer()
    {
        // remove all chunk processors
        const uint32 numProcessors = mChunkProcessors.GetLength();
        for (uint32 i = 0; i < numProcessors; ++i)
        {
            mChunkProcessors[i]->Destroy();
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
        if (header.mFourcc[0] != 'A' || header.mFourcc[1] != 'C' || header.mFourcc[2] != 'T' || header.mFourcc[3] != 'R')
        {
            return false;
        }

        // read the chunks
        switch (header.mEndianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.mEndianType);
            return false;
        }
        ;

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
        if ((header.mFourcc[0] != 'M' || header.mFourcc[1] != 'O' || header.mFourcc[2] != 'T' || header.mFourcc[3] != ' '))
        {
            return false;
        }

        // read the chunks
        switch (header.mEndianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.mEndianType);
            return false;
        }
        ;

        // yes, it is a valid motion file!
        return true;
    }


    // check if we can process the given motion set file
    bool Importer::CheckIfIsValidMotionSetFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const
    {
        MCORE_ASSERT(f->GetIsOpen());

        // verify if we actually are dealing with a valid actor file
        FileFormat::MotionSet_Header    header;
        if (f->Read(&header, sizeof(FileFormat::MotionSet_Header)) == 0)
        {
            MCore::LogError("Failed to read the motion set file header!");
            return false;
        }

        // check the FOURCC
        if (header.mFourCC[0] != 'M' || header.mFourCC[1] != 'O' || header.mFourCC[2] != 'S' || header.mFourCC[3] != ' ')
        {
            return false;
        }

        // read the chunks
        switch (header.mEndianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.mEndianType);
            return false;
        }
        ;

        // yes, it is a valid motionset file!
        return true;
    }



    // check if we can process the given anim graph file
    bool Importer::CheckIfIsValidAnimGraphFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const
    {
        MCORE_ASSERT(f->GetIsOpen());

        // verify if we actually are dealing with a valid actor file
        FileFormat::AnimGraph_Header  header;
        if (f->Read(&header, sizeof(FileFormat::AnimGraph_Header)) == 0)
        {
            return false;
        }

        // check the FOURCC
        if (header.mFourCC[0] != 'A' || header.mFourCC[1] != 'N' || header.mFourCC[2] != 'G' || header.mFourCC[3] != 'R')
        {
            return false;
        }

        // read the chunks
        switch (header.mEndianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.mEndianType);
            return false;
        }
        ;

        // yes, it is a valid anim graph file!
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
        if (header.mFourCC[0] != 'N' || header.mFourCC[1] != 'O' || header.mFourCC[2] != 'M' || header.mFourCC[3] != 'P')
        {
            return false;
        }

        // read the chunks
        switch (header.mEndianType)
        {
        case 0:
            *outEndianType  = MCore::Endian::ENDIAN_LITTLE;
            break;
        case 1:
            *outEndianType  = MCore::Endian::ENDIAN_BIG;
            break;
        default:
            MCore::LogError("Unsupported endian type used! (endian type = %d)", header.mEndianType);
            return false;
        }

        // yes, it is a valid node map file!
        return true;
    }


    // try to load an actor from disk
    AZStd::unique_ptr<Actor> Importer::LoadActor(AZStd::string filename, ActorSettings* settings)
    {
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
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
        MCore::Array<SharedData*> sharedData;
        sharedData.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
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

        if (actorSettings.mOptimizeForServer)
        {
            actorSettings.OptimizeForServer();
        }

        // fix any possible conflicting settings
        ValidateActorSettings(&actorSettings);

        // create the actor
        AZStd::unique_ptr<Actor> actor = AZStd::make_unique<Actor>("Unnamed actor");
        MCORE_ASSERT(actor);

        if (actor)
        {
            actor->SetThreadIndex(actorSettings.mThreadIndex);

            // set the scale mode
            // actor->SetScaleMode( scaleMode );

            // init the import parameters
            ImportParameters params;
            params.mSharedData = &sharedData;
            params.mEndianType = endianType;
            params.mActorSettings = &actorSettings;
            params.mActor = actor.get();

            // process all chunks
            while (ProcessChunk(f, params))
            {
            }

            actor->SetFileName(filename);

            // Generate an optimized version of skeleton for server.
            if (actorSettings.mOptimizeForServer && actor->GetOptimizeSkeleton())
            {
                actor->GenerateOptimizedSkeleton();
            }

            // post create init
            actor->PostCreateInit(actorSettings.mMakeGeomLODsCompatibleWithSkeletalLODs, false, actorSettings.mUnitTypeConvert);
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.Clear();

        // return the created actor
        return actor;
    }

    //-------------------------------------------------------------------------------------------------

    // try to load a motion from disk
    Motion* Importer::LoadMotion(AZStd::string filename, MotionSettings* settings)
    {
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // check if we want to load the motion even if a motion with the given filename is already inside the motion manager
        if (settings == nullptr || (settings && settings->mForceLoading == false))
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
        MCore::Array<SharedData*> sharedData;
        sharedData.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
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
        params.mSharedData     = &sharedData;
        params.mEndianType     = endianType;
        params.mMotionSettings = &motionSettings;
        params.mMotion         = motion;

        // read the chunks
        while (ProcessChunk(f, params))
        {
        }

        // make sure there is a sync track
        motion->GetEventTable()->AutoCreateSyncTrack(motion);

        // scale to the EMotion FX unit type
        if (motionSettings.mUnitTypeConvert)
        {
            motion->ScaleToUnitType(GetEMotionFX().GetUnitType());
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.Clear();

        return motion;
    }

    //-------------------------------------------------------------------------------------------------

    // try to load a motion set from disk
    MotionSet* Importer::LoadMotionSet(AZStd::string filename, MotionSetSettings* settings, const AZ::ObjectStream::FilterDescriptor& loadFilter)
    {
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        const bool isLegacyFile = Importer::CheckFileType(filename.c_str()) == Importer::EFileType::FILETYPE_MOTIONSET;
        if (!isLegacyFile)
        {
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
            }
            return motionSet;
        }

        //////////////////////////////////////////
        // Legacy file type loading
        //////////////////////////////////////////

        // check if we want to load the motion set even if a motion set with the given filename is already inside the motion manager
        if (settings == nullptr || settings->mForceLoading == false)
        {
            // search the motion set inside the motion manager and return it if it already got loaded
            MotionSet* motionSet = GetMotionManager().FindMotionSetByFileName(filename.c_str());
            if (motionSet)
            {
                MCore::LogInfo("  + Motion set '%s' already loaded, returning already loaded motion set from the MotionManager.", filename.c_str());
                return motionSet;
            }
        }

        if (GetLogging())
        {
            MCore::LogInfo("- Trying to load motion set from file '%s'...", filename.c_str());
        }

        // try to open the file from disk
        MCore::DiskFile f;
        if (f.Open(filename.c_str(), MCore::DiskFile::READ) == false)
        {
            if (GetLogging())
            {
                MCore::LogError("  + Failed to open the file for motion set.");
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

        // create the motion set reading from memory
        MotionSet* result = LoadMotionSet(fileBuffer, fileSize, settings);
        if (result)
        {
            result->SetFilename(filename.c_str());
        }

        // delete the filebuffer again
        MCore::Free(fileBuffer);

        // check if it worked :)
        if (result == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("  + Failed to load motion set from file '%s'.", filename.c_str());
            }
        }
        else
        {
            if (GetLogging())
            {
                MCore::LogInfo("  + Loading successfully finished.");
            }
        }

        // return the result
        return result;
    }


    MotionSet* Importer::LoadMotionSet(uint8* memoryStart, size_t lengthInBytes, MotionSetSettings* settings)
    {
        // Legacy file type loading.
        MCore::MemoryFile memFile;
        memFile.Open(memoryStart, lengthInBytes);

        const bool isLegacyFile = Importer::CheckFileType(&memFile) == Importer::EFileType::FILETYPE_MOTIONSET;
        if (isLegacyFile)
        {
            // Open the memory file again as CheckFileType() is closing it at the end.
            memFile.Open(memoryStart, lengthInBytes);
            EMotionFX::MotionSet* motionSet = LoadMotionSet(&memFile, settings);
            if (settings)
            {
                motionSet->SetIsOwnedByRuntime(settings->m_isOwnedByRuntime);
            }
            return motionSet;
        }


        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        EMotionFX::MotionSet* motionSet = EMotionFX::MotionSet::LoadFromBuffer(memoryStart, lengthInBytes, context);
        if (settings)
        {
            motionSet->SetIsOwnedByRuntime(settings->m_isOwnedByRuntime);
        }
        return motionSet;
    }


    // try to load a motion set from a file
    MotionSet* Importer::LoadMotionSet(MCore::File* f, MotionSetSettings* settings)
    {
        MCORE_ASSERT(f);
        MCORE_ASSERT(f->GetIsOpen());

        // create the shared data
        MCore::Array<SharedData*> sharedData;
        sharedData.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
        PrepareSharedData(sharedData);

        // load the file header
        FileFormat::MotionSet_Header fileHeader;
        f->Read(&fileHeader, sizeof(FileFormat::MotionSet_Header));
        if (fileHeader.mFourCC[0] != 'M' || fileHeader.mFourCC[1] != 'O' || fileHeader.mFourCC[2] != 'S' || fileHeader.mFourCC[3] != ' ')
        {
            MCore::LogError("The motion set file is not a valid file.");
            f->Close();
            return nullptr;
        }

        // get the endian type
        MCore::Endian::EEndianType endianType = (MCore::Endian::EEndianType)fileHeader.mEndianType;

        // init the import parameters
        ImportParameters params;
        params.mSharedData  = &sharedData;
        params.mEndianType  = endianType;
        params.m_isOwnedByRuntime = settings ? settings->m_isOwnedByRuntime : false;

        // read the chunks
        while (ProcessChunk(f, params))
        {
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.Clear();

        // check if the motion set got set
        if (params.mMotionSet == nullptr)
        {
            return nullptr;
        }

        return params.mMotionSet;
    }

    //-------------------------------------------------------------------------------------------------

    // load a node map by filename
    NodeMap* Importer::LoadNodeMap(AZStd::string filename, NodeMapSettings* settings)
    {
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

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
        MCore::Array<SharedData*> sharedData;
        sharedData.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
        PrepareSharedData(sharedData);

        //-----------------------------------------------

        // load the file header
        FileFormat::NodeMap_Header fileHeader;
        f->Read(&fileHeader, sizeof(FileFormat::NodeMap_Header));
        if (fileHeader.mFourCC[0] != 'N' || fileHeader.mFourCC[1] != 'O' || fileHeader.mFourCC[2] != 'M' || fileHeader.mFourCC[3] != 'P')
        {
            MCore::LogError("The node map file is not a valid node map file.");
            f->Close();
            return nullptr;
        }

        // get the endian type
        MCore::Endian::EEndianType endianType = (MCore::Endian::EEndianType)fileHeader.mEndianType;

        // create the node map
        NodeMap* nodeMap = NodeMap::Create();

        // init the import parameters
        ImportParameters params;
        params.mSharedData          = &sharedData;
        params.mEndianType          = endianType;
        params.mNodeMap             = nodeMap;
        params.mNodeMapSettings     = &nodeMapSettings;

        // process all chunks
        while (ProcessChunk(f, params))
        {
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.Clear();

        // return the created actor
        return nodeMap;
    }

    //-------------------------------------------------------------------------------------------------

    // register a new chunk processor
    void Importer::RegisterChunkProcessor(ChunkProcessor* processorToRegister)
    {
        MCORE_ASSERT(processorToRegister);
        mChunkProcessors.Add(processorToRegister);
    }


    // add shared data object to the importer
    void Importer::AddSharedData(MCore::Array<SharedData*>& sharedData, SharedData* data)
    {
        MCORE_ASSERT(data);
        sharedData.Add(data);
    }


    // search for shared data
    SharedData* Importer::FindSharedData(MCore::Array<SharedData*>* sharedDataArray, uint32 type)
    {
        // for all shared data
        const uint32 numSharedData = sharedDataArray->GetLength();
        for (uint32 i = 0; i < numSharedData; ++i)
        {
            SharedData* sharedData = sharedDataArray->GetItem(i);

            // check if it's the type we are searching for
            if (sharedData->GetType() == type)
            {
                return sharedData;
            }
        }

        // nothing found
        return nullptr;
    }


    void Importer::SetLoggingEnabled(bool enabled)
    {
        mLoggingActive = enabled;
    }


    bool Importer::GetLogging() const
    {
        return mLoggingActive;
    }


    void Importer::SetLogDetails(bool detailLoggingActive)
    {
        mLogDetails = detailLoggingActive;

        // set the processors logging flag
        const int32 numProcessors = mChunkProcessors.GetLength();
        for (int32 i = 0; i < numProcessors; i++)
        {
            ChunkProcessor* processor = mChunkProcessors[i];
            processor->SetLogging((mLoggingActive && detailLoggingActive)); // only enable if logging is also enabled
        }
    }


    bool Importer::GetLogDetails() const
    {
        return mLogDetails;
    }


    void Importer::PrepareSharedData(MCore::Array<SharedData*>& sharedData)
    {
        // create standard shared objects
        AddSharedData(sharedData, SharedHelperData::Create());
    }


    // reset shared objects so that the importer is ready for use again
    void Importer::ResetSharedData(MCore::Array<SharedData*>& sharedData)
    {
        const int32 numSharedData = sharedData.GetLength();
        for (int32 i = 0; i < numSharedData; i++)
        {
            SharedData* data = sharedData[i];
            data->Reset();
            data->Destroy();
        }
        sharedData.Clear();
    }


    // find chunk, return nullptr if it hasn't been found
    ChunkProcessor* Importer::FindChunk(uint32 chunkID, uint32 version) const
    {
        // for all chunk processors
        const uint32 numProcessors = mChunkProcessors.GetLength();
        for (uint32 i = 0; i < numProcessors; ++i)
        {
            ChunkProcessor* processor = mChunkProcessors[i];

            // if this chunk is the type we are searching for AND it can process our chunk version, return it
            if (processor->GetChunkID() == chunkID && processor->GetVersion() == version)
            {
                return processor;
            }
        }

        // nothing found
        return nullptr;
    }


    // register basic chunk processors
    void Importer::RegisterStandardChunks()
    {
        // reserve space for 75 chunk processors
        mChunkProcessors.Reserve(75);

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
        RegisterChunkProcessor(aznew ChunkProcessorActorNodes());
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

        // AnimGraph file format
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphParameters());
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphNodeGroups());
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphNode());
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphStateTransitions());
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphNodeConnections());
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphGroupParameters());
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphGameControllerSettings());
        RegisterChunkProcessor(aznew ChunkProcessorAnimGraphAdditionalInfo());

        // motion set file format
        RegisterChunkProcessor(aznew ChunkProcessorMotionSet());

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
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        MCore::Endian::ConvertUnsignedInt32(&chunk.mChunkID, endianType);
        MCore::Endian::ConvertUnsignedInt32(&chunk.mSizeInBytes, endianType);
        MCore::Endian::ConvertUnsignedInt32(&chunk.mVersion, endianType);

        // try to find the chunk processor which can process this chunk
        ChunkProcessor* processor = FindChunk(chunk.mChunkID, chunk.mVersion);

        // if we cannot find the chunk, skip the chunk
        if (processor == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("Importer::ProcessChunk() - Unknown chunk (ID=%d  Size=%d bytes Version=%d), skipping...", chunk.mChunkID, chunk.mSizeInBytes, chunk.mVersion);
            }
            file->Forward(chunk.mSizeInBytes);

            return true;
        }

        // get some shortcuts
        Importer::ActorSettings* actorSettings = importParams.mActorSettings;
        Importer::MotionSettings* skelMotionSettings = importParams.mMotionSettings;

        // check if we still want to skip the chunk or not
        bool mustSkip = false;

        // check if we specified to ignore this chunk
        if (actorSettings && actorSettings->mChunkIDsToIgnore.Contains(chunk.mChunkID))
        {
            mustSkip = true;
        }

        if (skelMotionSettings && skelMotionSettings->mChunkIDsToIgnore.Contains(chunk.mChunkID))
        {
            mustSkip = true;
        }

        // if we still don't need to skip
        if (mustSkip == false)
        {
            // if we're loading an actor
            if (actorSettings)
            {
                if ((actorSettings->mLoadLimits                 == false && chunk.mChunkID == FileFormat::ACTOR_CHUNK_LIMIT) ||
                    (actorSettings->mLoadMorphTargets           == false && chunk.mChunkID == FileFormat::ACTOR_CHUNK_STDPROGMORPHTARGET) ||
                    (actorSettings->mLoadMorphTargets           == false && chunk.mChunkID == FileFormat::ACTOR_CHUNK_STDPMORPHTARGETS) ||
                    (actorSettings->mLoadSimulatedObjects       == false && chunk.mChunkID == FileFormat::ACTOR_CHUNK_SIMULATEDOBJECTSETUP))
                {
                    mustSkip = true;
                }
            }

            // if we're loading a motion
            if (skelMotionSettings)
            {
                if (skelMotionSettings->mLoadMotionEvents       == false && chunk.mChunkID == FileFormat::MOTION_CHUNK_MOTIONEVENTTABLE)
                {
                    mustSkip = true;
                }
            }
        }

        // if we want to skip this chunk
        if (mustSkip)
        {
            file->Forward(chunk.mSizeInBytes);
            return true;
        }

        // try to process the chunk
        return processor->Process(file, importParams);
    }


    // make sure no settings conflict or can cause crashes
    void Importer::ValidateActorSettings(ActorSettings* settings)
    {
        // After atom: Make sure we are not loading the tangents and bitangents
        if (!settings->mLayerIDsToIgnore.Contains(Mesh::ATTRIB_TANGENTS))
        {
            settings->mLayerIDsToIgnore.Add(Mesh::ATTRIB_TANGENTS);
        }

        if (!settings->mLayerIDsToIgnore.Contains(Mesh::ATTRIB_BITANGENTS))
        {
            settings->mLayerIDsToIgnore.Add(Mesh::ATTRIB_BITANGENTS);
        }

        // make sure we load at least the position and normals and org vertex numbers
        settings->mLayerIDsToIgnore.RemoveByValue(Mesh::ATTRIB_ORGVTXNUMBERS);
        settings->mLayerIDsToIgnore.RemoveByValue(Mesh::ATTRIB_NORMALS);
        settings->mLayerIDsToIgnore.RemoveByValue(Mesh::ATTRIB_POSITIONS);
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

        // check for motion set
        file->Seek(0);
        if (CheckIfIsValidMotionSetFile(file, &endianType))
        {
            file->Close();
            return FILETYPE_MOTIONSET;
        }

        // check for node map
        file->Seek(0);
        if (CheckIfIsValidNodeMapFile(file, &endianType))
        {
            file->Close();
            return FILETYPE_NODEMAP;
        }

        // check for anim graph
        file->Seek(0);
        if (CheckIfIsValidAnimGraphFile(file, &endianType))
        {
            file->Close();
            return FILETYPE_ANIMGRAPH;
        }

        // close the file again
        file->Close();

        return FILETYPE_UNKNOWN;
    }

    //---------------------------------------------------------


    // load anim graph by filename
    AnimGraph* Importer::LoadAnimGraph(AZStd::string filename, AnimGraphSettings* settings, const AZ::ObjectStream::FilterDescriptor& loadFilter)
    {
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        const bool isLegacyFile = Importer::CheckFileType(filename.c_str()) == Importer::EFileType::FILETYPE_ANIMGRAPH;
        if (!isLegacyFile)
        {
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
                animGraph->RemoveInvalidConnections();  // Remove connections that have nullptr source node's, which happens when connections point to unknown nodes.
            }

            return animGraph;
        }

        //////////////////////////////////////////
        // Legacy file type loading
        //////////////////////////////////////////

        // check if we want to load the anim graph even if a anim graph with the given filename is already inside the anim graph manager
        if (settings == nullptr || settings->mForceLoading == false)
        {
            // search the anim graph inside the anim graph manager and return it if it already got loaded
            AnimGraph* animGraph = GetAnimGraphManager().FindAnimGraphByFileName(filename.c_str());
            if (animGraph)
            {
                MCore::LogInfo("  + Anim graph '%s' already loaded, returning already loaded anim graph from the AnimGraphManager.", filename.c_str());
                return animGraph;
            }
        }

        if (GetLogging())
        {
            MCore::LogInfo("- Trying to load anim graph from file '%s'...", filename.c_str());
        }

        // try to open the file from disk
        MCore::DiskFile f;
        if (!f.Open(filename.c_str(), MCore::DiskFile::READ))
        {
            if (GetLogging())
            {
                MCore::LogError("  + Failed to open the file for anim graph '%s', anim graph not loaded!", filename.c_str());
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
        AnimGraph* result = LoadAnimGraph(fileBuffer, fileSize, settings);
        if (result)
        {
            result->SetFileName(filename.c_str());
            result->RemoveInvalidConnections();  // Remove connections that have nullptr source node's, which happens when connections point to unknown nodes.
        }

        // delete the filebuffer again
        MCore::Free(fileBuffer);

        // check if it worked
        if (result == nullptr)
        {
            if (GetLogging())
            {
                MCore::LogError("   + Failed to load anim graph from file '%s'", filename.c_str());
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


    // load the anim graph from memory
    AnimGraph* Importer::LoadAnimGraph(uint8* memoryStart, size_t lengthInBytes, AnimGraphSettings* settings)
    {
        // Legacy file type loading.
        MCore::MemoryFile memFile;
        memFile.Open(memoryStart, lengthInBytes);

        const bool isLegacyFile = Importer::CheckFileType(&memFile) == Importer::EFileType::FILETYPE_ANIMGRAPH;
        if (isLegacyFile)
        {
            // Open the memory file again as CheckFileType() is closing it at the end.
            memFile.Open(memoryStart, lengthInBytes);
            return LoadAnimGraph(&memFile, settings);
        }

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


    // load a anim graph from a file object
    AnimGraph* Importer::LoadAnimGraph(MCore::File* f, AnimGraphSettings* settings)
    {
        MCORE_ASSERT(f);
        MCORE_ASSERT(f->GetIsOpen());

        // execute the pre-passes
        if (f->GetType() != MCore::MemoryFile::TYPE_ID)
        {
            MCore::LogError("Given file is not a memory file. Cannot process pre-passes.");
            return nullptr;
        }

        // copy over the actor settings, or use defaults
        AnimGraphSettings animGraphSettings;
        if (settings)
        {
            animGraphSettings = *settings;
        }

        // create the shared data
        MCore::Array<SharedData*> sharedData;
        sharedData.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
        PrepareSharedData(sharedData);

        //-----------------------------------------------

        // load the file header
        FileFormat::AnimGraph_Header fileHeader;
        f->Read(&fileHeader, sizeof(FileFormat::AnimGraph_Header));
        if (fileHeader.mFourCC[0] != 'A' || fileHeader.mFourCC[1] != 'N' || fileHeader.mFourCC[2] != 'G' || fileHeader.mFourCC[3] != 'R')
        {
            MCore::LogError("The anim graph file is not a valid anim graph file.");
            f->Close();
            return nullptr;
        }

        // get the endian type
        MCore::Endian::EEndianType endianType = (MCore::Endian::EEndianType)fileHeader.mEndianType;

        // convert endian of the integer values
        MCore::Endian::ConvertUnsignedInt32(&fileHeader.mFileVersion,          endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileHeader.mNumNodes,             endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileHeader.mNumStateTransitions,  endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileHeader.mNumNodeConnections,   endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileHeader.mNumParameters,        endianType);

        // read the anim graph name, create it, and read the other remaining info strings
        SharedHelperData::ReadString(f, &sharedData, endianType);
        AnimGraph* animGraph = aznew AnimGraph();

        if (GetLogDetails())
        {
            MCore::LogDetailedInfo("Anim Graph:");

            SharedHelperData::ReadString(f, &sharedData, endianType); // copyright
            SharedHelperData::ReadString(f, &sharedData, endianType); // description
            MCore::LogDetailedInfo("   + Company           = %s", SharedHelperData::ReadString(f, &sharedData, endianType)); // company
            MCore::LogDetailedInfo("   + EMotion FX Version= %s", SharedHelperData::ReadString(f, &sharedData, endianType)); // emfx version
            MCore::LogDetailedInfo("   + EMStudio Build    = %s", SharedHelperData::ReadString(f, &sharedData, endianType)); // emstudio build
            MCore::LogDetailedInfo("   + Num nodes         = %d", fileHeader.mNumNodes);
            MCore::LogDetailedInfo("   + Num transitions   = %d", fileHeader.mNumStateTransitions);
            MCore::LogDetailedInfo("   + Num connections   = %d", fileHeader.mNumNodeConnections);
            MCore::LogDetailedInfo("   + Num parameters    = %d", fileHeader.mNumParameters);
            MCore::LogDetailedInfo("   + File version      = %d", fileHeader.mFileVersion);
            MCore::LogDetailedInfo("   + Endian type       = %d", fileHeader.mEndianType);
        }
        else
        {
            SharedHelperData::ReadString(f, &sharedData, endianType); // copyright
            SharedHelperData::ReadString(f, &sharedData, endianType); // description
            SharedHelperData::ReadString(f, &sharedData, endianType); // company
            SharedHelperData::ReadString(f, &sharedData, endianType); // emfx version
            SharedHelperData::ReadString(f, &sharedData, endianType); // emstudio build
        }

        // init the import parameters
        ImportParameters params;
        params.mSharedData          = &sharedData;
        params.mEndianType          = endianType;
        params.mAnimGraph          = animGraph;
        params.mAnimGraphSettings  = &animGraphSettings;

        // pre-allocate the blend nodes array to prevent reallocs
        MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(params.mSharedData);
        blendNodes.Reserve(fileHeader.mNumNodes);

        // process all chunks
        while (ProcessChunk(f, params))
        {
        }

        // close the file and return a pointer to the actor we loaded
        f->Close();

        // get rid of shared data
        ResetSharedData(sharedData);
        sharedData.Clear();

        // recursively update attributes of all state machines and blend tree nodes
        if (animGraph->GetRootStateMachine())
        {
            animGraph->InitAfterLoading();
            animGraph->RemoveInvalidConnections(true);  // Remove connections that have nullptr source node's, which happens when connections point to unknown nodes.
        }
        else
        {
            delete animGraph;
            animGraph = nullptr;
        }

        // return the created actor
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
        outInfo->mEndianType = endianType;

        // as we seeked to the end of the header and we know the second chunk always is the time stamp, we can read this now
        FileFormat::FileChunk fileChunk;
        file.Read(&fileChunk, sizeof(FileFormat::FileChunk));
        FileFormat::FileTime timeChunk;
        file.Read(&timeChunk, sizeof(FileFormat::FileTime));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileChunk.mChunkID, endianType);
        MCore::Endian::ConvertUnsignedInt16(&timeChunk.mYear, endianType);

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
        outInfo->mEndianType = endianType;

        // as we seeked to the end of the header and we know the second chunk always is the time stamp, we can read this now
        FileFormat::FileChunk fileChunk;
        file.Read(&fileChunk, sizeof(FileFormat::FileChunk));
        FileFormat::FileTime timeChunk;
        file.Read(&timeChunk, sizeof(FileFormat::FileTime));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileChunk.mChunkID, endianType);
        MCore::Endian::ConvertUnsignedInt16(&timeChunk.mYear, endianType);

        return true;
    }
} // namespace EMotionFX
