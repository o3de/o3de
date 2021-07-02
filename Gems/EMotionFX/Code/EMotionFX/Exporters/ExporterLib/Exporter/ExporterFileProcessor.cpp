/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "ExporterFileProcessor.h"
#include "Exporter.h"
#include <AzCore/Debug/Timer.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace ExporterLib
{
#define PREPARE_DISKFILE_SAVE(EXTENSION)                      \
    AZ::Debug::Timer saveTimer;                               \
    saveTimer.Stamp();                                        \
    if (filenameWithoutExtension.empty())                     \
    {                                                         \
        MCore::LogError("Cannot save file. Empty FileName."); \
        return false;                                         \
    }                                                         \
    AZStd::string extension;                                  \
    AzFramework::StringFunc::Path::GetExtension(filenameWithoutExtension.c_str(), extension); \
    if (!extension.empty())                                   \
    {                                                         \
        AzFramework::StringFunc::Replace(filenameWithoutExtension, extension.c_str(), EXTENSION(), true, false, true); \
    }                                                         \
    else                                                      \
    {                                                         \
        filenameWithoutExtension += EXTENSION();              \
    }                                                         \
                                                              \
    MCore::MemoryFile memoryFile;                             \
    memoryFile.Open();                                        \


#define FINISH_DISKFILE_SAVE                                                                                                    \
    bool result = memoryFile.SaveToDiskFile(filenameWithoutExtension.c_str());                                                  \
    const float saveTime = saveTimer.GetDeltaTimeInSeconds() * 1000.0f;                                                         \
    MCore::LogInfo("Saved file '%s' in %.2f ms.", filenameWithoutExtension.c_str(), saveTime);                                  \
    return result;


    // constructor
    Exporter::Exporter()
        : EMotionFX::BaseObject()
    {
    }


    // destructor
    Exporter::~Exporter()
    {
    }


    // create the exporter
    Exporter* Exporter::Create()
    {
        return new Exporter();
    }


    void Exporter::Delete()
    {
        delete this;
    }


    // make the memory file ready for saving
    void Exporter::ResetMemoryFile(MCore::MemoryFile* file)
    {
        // make sure the file is valid
        MCORE_ASSERT(file);

        // reset the incoming memory file
        file->Close();
        file->Open();
        file->SetPreAllocSize(262144); // 256kB
        file->Seek(0);
    }


    // actor
    bool Exporter::SaveActor(MCore::MemoryFile* file, const EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        ResetMemoryFile(file);
        ExporterLib::SaveActor(file, actor, targetEndianType);
        return true;
    }


    bool Exporter::SaveActor(AZStd::string filenameWithoutExtension, const EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        PREPARE_DISKFILE_SAVE(GetActorExtension);
        if (SaveActor(&memoryFile, actor, targetEndianType) == false)
        {
            return false;
        }
        FINISH_DISKFILE_SAVE
    }


    // skeletal motion
    bool Exporter::SaveMotion(MCore::MemoryFile* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        ResetMemoryFile(file);
        ExporterLib::SaveMotion(file, motion, targetEndianType);
        return true;
    }


    bool Exporter::SaveMotion(AZStd::string filenameWithoutExtension, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        PREPARE_DISKFILE_SAVE(GetMotionExtension);
        if (SaveMotion(&memoryFile, motion, targetEndianType) == false)
        {
            return false;
        }
        FINISH_DISKFILE_SAVE
    }
} // namespace ExporterLib
