/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorManager.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzToolsFramework/AssetBundle/AssetBundleAPI.h>

const char* appWindowName = "DeltaCataloger";
enum class DeltaCatalogerResult : AZ::u8
{
    Success = 0,

    InvalidArg = 1,
    FailedToCreateDeltaCatalog,
    FailedToInjectFile, 
};

struct DeltaCatalogerParams
{
    AZStd::string sourceCatalogPath;
    AZStd::vector<AZStd::string> sourcePaks;
    AZStd::string workingDirectory;
    bool verbose = false;
    bool regenerateExistingDeltas = false;
};


DeltaCatalogerResult DeltaCataloger(DeltaCatalogerParams& params)
{
    using AssetCatalogRequestBus = AZ::Data::AssetCatalogRequestBus;
    using AssetBundleCommandsBus = AzToolsFramework::AssetBundleCommands::Bus;

    // update all relative paths given to be relative to the working directory
    if (params.workingDirectory.length())
    { 
        AzFramework::StringFunc::Path::Join(params.workingDirectory.c_str(), params.sourceCatalogPath.c_str(), params.sourceCatalogPath);
        for (AZ::u32 index = 0; index < params.sourcePaks.size(); ++index)
        {
            AzFramework::StringFunc::Path::Join(params.workingDirectory.c_str(), params.sourcePaks[index].c_str(), params.sourcePaks[index]);
        }
    }

    // validate params
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (!fileIO->Exists(params.sourceCatalogPath.c_str()))
    {
        AZ_Error(appWindowName, false, "Invalid Arg: Source Asset Catalog does not exist at \"%s\".", params.sourceCatalogPath.c_str());
        return DeltaCatalogerResult::InvalidArg;
    }

    if (params.sourcePaks.empty())
    {
        AZ_Error(appWindowName, false, "Failed to read source pak files arg list. Should start from second argument.");
        return DeltaCatalogerResult::InvalidArg;
    }

    for (const AZStd::string& sourcePakPath : params.sourcePaks)
    {
        if (!fileIO->Exists(sourcePakPath.c_str()))
        {
            AZ_Error(appWindowName, false, "Invalid Arg: Source Pak does not exist at \"%s\".", sourcePakPath.c_str());
            return DeltaCatalogerResult::InvalidArg;
        }
    }
  
    // Load the source catalog
    if (params.verbose)
    {
        AZ_TracePrintf(appWindowName, "Loading source asset catalog \"%s\".\n", params.sourceCatalogPath.c_str())
    }

    AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
    bool result = false;
    AssetCatalogRequestBus::BroadcastResult(result, &AssetCatalogRequestBus::Events::LoadCatalog, params.sourceCatalogPath.c_str());

    if (result)
    {
        for (const AZStd::string& sourcePakPath : params.sourcePaks)
        {
            bool catalogCreated = false;
            AssetBundleCommandsBus::BroadcastResult(catalogCreated, &AssetBundleCommandsBus::Events::CreateDeltaCatalog, sourcePakPath, params.regenerateExistingDeltas);
            if (!catalogCreated)
            {
                AZ_Error(appWindowName, false, "Failed to make or inject delta asset catalog for \"%s\".", sourcePakPath.c_str());
                return DeltaCatalogerResult::FailedToCreateDeltaCatalog;
            }
        }
    }
    else
    {
        AZStd::string error = AZStd::string::format("Failed to load source asset catalog \"%s\".", params.sourceCatalogPath.c_str());
        AZ_Error(appWindowName, false, error.c_str());
        return DeltaCatalogerResult::FailedToCreateDeltaCatalog;
    }
    return DeltaCatalogerResult::Success;
}


DeltaCatalogerResult ParseArgs(const AzFramework::CommandLine* parser, DeltaCatalogerParams& params)
{
    // AzFramework CommandLine consumes the first arg (the executable itself), so positional or switch args start at 0
    const AZ::u8 sourceCatalogPathIndex = 0;
    const AZ::u8 sourceCatalogStartIndex = 1;

    params.sourceCatalogPath = parser->GetMiscValue(sourceCatalogPathIndex);
    AZ::u64 numPositionalArgs = parser->GetNumMiscValues();
    for (AZ::u64 index = sourceCatalogStartIndex; index < numPositionalArgs; ++index)
    {
        params.sourcePaks.push_back(parser->GetMiscValue(index));
    }

    params.verbose = parser->HasSwitch("verbose");
    params.regenerateExistingDeltas = parser->HasSwitch("regenerate");
    params.workingDirectory = parser->GetSwitchValue("working-dir", 0);
    
    return DeltaCatalogerResult::Success;
}

int main(int argc, char** argv)
{
    const AZ::Debug::Trace tracer;
    DeltaCatalogerResult exitCode = DeltaCatalogerResult::Success;
    const AZ::u8 minimumArgCount = 3; // 0 = exe, 1 = source catalog path, 2 = source pak path, from raw commandline input
    if (argc < minimumArgCount)
    {
        AZ_Error(appWindowName, false, "Must specify source catalog, and at least one source pak file.");
        return static_cast<int>(DeltaCatalogerResult::InvalidArg);
    }

    AzToolsFramework::ToolsApplication app(&argc, &argv);

    // can deal with starting the app with a different working directory if needed later (use lmbr cli main.cpp as a reference)
    AzFramework::Application::StartupParameters startParam;
    app.Start(AzFramework::Application::Descriptor());    
    {
        DeltaCatalogerParams params;
        exitCode = ParseArgs(app.GetCommandLine(), params);
        if (exitCode != DeltaCatalogerResult::Success)
        {
            return static_cast<int>(exitCode);
        }

        exitCode = DeltaCataloger(params);

        // Tick until everything is ready for shutdown
        app.Tick();
    }
    app.Stop();    
    return static_cast<int>(exitCode);
}
