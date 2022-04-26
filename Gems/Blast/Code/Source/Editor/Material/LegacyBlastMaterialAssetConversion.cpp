/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Asset/GenericAssetHandler.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Material/BlastMaterialConfiguration.h>
#include <Material/BlastMaterialAsset.h>

namespace Blast
{
    void ConvertMaterialLibrariesIntoIndividualMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC("ed_blastConvertMaterialLibrariesIntoIndividualMaterials", ConvertMaterialLibrariesIntoIndividualMaterials, AZ::ConsoleFunctorFlags::Null,
        "Finds legacy blast material library assets in the project and generates new individual blast material assets. Original library assets will be deleted.");

    // O3DE_DEPRECATION
    // Default values used for initializing materials.
    // Use BlastMaterialConfiguration to define properties for materials at the time of creation.
    class BlastMaterialConfiguration
    {
    public:
        AZ_TYPE_INFO(BlastMaterialConfiguration, "{BEC875B1-26E4-4A4A-805E-0E880372720D}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlastMaterialConfiguration>()
                    ->Version(1)
                    ->Field("MaterialName", &BlastMaterialConfiguration::m_materialName)
                    ->Field("Health", &BlastMaterialConfiguration::m_health)
                    ->Field("ForceDivider", &BlastMaterialConfiguration::m_forceDivider)
                    ->Field("MinDamageThreshold", &BlastMaterialConfiguration::m_minDamageThreshold)
                    ->Field("MaxDamageThreshold", &BlastMaterialConfiguration::m_maxDamageThreshold)
                    ->Field("StressLinearFactor", &BlastMaterialConfiguration::m_stressLinearFactor)
                    ->Field("StressAngularFactor", &BlastMaterialConfiguration::m_stressAngularFactor);
            }
        }

        float m_health = 1.0f;
        float m_forceDivider = 1.0f;
        float m_minDamageThreshold = 0.0f;
        float m_maxDamageThreshold = 1.0f;
        float m_stressLinearFactor = 1.0f;
        float m_stressAngularFactor = 1.0f;

        AZStd::string m_materialName{ "Default" };
    };

    // O3DE_DEPRECATION
    // A single BlastMaterial entry in the material library
    // BlastMaterialLibraryAsset holds a collection of BlastMaterialFromAssetConfiguration instances.
    class BlastMaterialFromAssetConfiguration
    {
    public:
        AZ_TYPE_INFO(BlastMaterialFromAssetConfiguration, "{E380E174-BCA3-4BBB-AA39-8FAD39005B12}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlastMaterialFromAssetConfiguration>()
                    ->Version(1)
                    ->Field("Configuration", &BlastMaterialFromAssetConfiguration::m_configuration)
                    ->Field("UID", &BlastMaterialFromAssetConfiguration::m_id);
            }
        }

        BlastMaterialConfiguration m_configuration;
        BlastMaterialId m_id;

        void CopyDataToMaterialAsset(MaterialAsset& materialAsset) const
        {
            materialAsset.m_materialConfiguration.m_health = m_configuration.m_health;
            materialAsset.m_materialConfiguration.m_forceDivider = m_configuration.m_forceDivider;
            materialAsset.m_materialConfiguration.m_minDamageThreshold = m_configuration.m_minDamageThreshold;
            materialAsset.m_materialConfiguration.m_maxDamageThreshold = m_configuration.m_maxDamageThreshold;
            materialAsset.m_materialConfiguration.m_stressLinearFactor = m_configuration.m_stressLinearFactor;
            materialAsset.m_materialConfiguration.m_stressAngularFactor = m_configuration.m_stressAngularFactor;
            materialAsset.m_legacyBlastMaterialId = m_id;
        }
    };

    // O3DE_DEPRECATION
    // An asset that holds a list of materials.
    class BlastMaterialLibraryAsset : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastMaterialLibraryAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(BlastMaterialLibraryAsset, "{55F38C86-0767-4E7F-830A-A4BF624BE4DA}", AZ::Data::AssetData);

        BlastMaterialLibraryAsset() = default;
        virtual ~BlastMaterialLibraryAsset() = default;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlastMaterialLibraryAsset>()
                    ->Version(1)
                    ->Field("Properties", &BlastMaterialLibraryAsset::m_materialLibrary);
            }
        }

        AZStd::vector<BlastMaterialFromAssetConfiguration> m_materialLibrary;
    };

    void ReflectLegacyMaterialClasses(AZ::ReflectContext* context)
    {
        BlastMaterialConfiguration::Reflect(context);
        BlastMaterialFromAssetConfiguration::Reflect(context);
        BlastMaterialLibraryAsset::Reflect(context);
    }

    AZStd::optional<AZStd::string> GetFullSourceAssetPathById(AZ::Data::AssetId assetId)
    {
        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);
        AZ_Assert(!assetPath.empty(), "Asset Catalog returned an invalid path from an enumerated asset.");
        if (assetPath.empty())
        {
            AZ_Warning("BlastMaterialConversion", false, "Not able get asset path for asset with id %s.",
                assetId.ToString<AZStd::string>().c_str());
            return AZStd::nullopt;
        }

        AZStd::string assetFullPath;
        bool assetFullPathFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            assetFullPathFound,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath,
            assetPath, assetFullPath);
        if (!assetFullPathFound)
        {
            AZ_Warning("BlastMaterialConversion", false, "Source file of asset '%s' could not be found.", assetPath.c_str());
            return AZStd::nullopt;
        }

        return { AZStd::move(assetFullPath) };
    }

    struct BlastMaterialLibrary
    {
        AZStd::vector<BlastMaterialFromAssetConfiguration> m_materialAssetConfigurations;
        AZStd::string m_sourceFile; // Path to material library source file
    };

    // Collects all legacy material libraries to convert to new material assets
    AZStd::vector<BlastMaterialLibrary> CollectMaterialLibraries(AZ::Data::AssetHandler* materialAssetHandler)
    {
        AZStd::vector<BlastMaterialLibrary> materialLibraries;

        // Unregister the MaterialAsset handler new for .blastmaterial files
        AZ::Data::AssetManager::Instance().UnregisterHandler(materialAssetHandler);

        // Create and register the asset handler for legacy BlastMaterialLibraryAsset to handle old .blastmaterial files
        auto materialLibraryAssetHandler = AZStd::make_unique<AzFramework::GenericAssetHandler<BlastMaterialLibraryAsset>>(
            "Blast Material", "Blast Material", "blastmaterial");
        AZ::Data::AssetManager::Instance().RegisterHandler(materialLibraryAssetHandler.get(), BlastMaterialLibraryAsset::RTTI_Type());

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&materialLibraryAssetHandler, &materialLibraries](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            // In the catalog all .blastmaterial files have rtti type of new MaterialAsset class.
            if (assetInfo.m_assetType != MaterialAsset::RTTI_Type())
            {
                return;
            }

            AZStd::optional<AZStd::string> assetFullPath = GetFullSourceAssetPathById(assetId);
            if (!assetFullPath.has_value())
            {
                return;
            }

            auto assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();
            // Read in the data from a file to a buffer, then hand ownership of the buffer over to the assetDataStream
            {
                AZ::IO::FileIOStream stream(assetFullPath->c_str(), AZ::IO::OpenMode::ModeRead);
                if (!AZ::IO::RetryOpenStream(stream))
                {
                    AZ_Warning("BlastMaterialConversion", false, "Source file '%s' could not be opened.", assetFullPath->c_str());
                    return;
                }
                AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
                size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
                if (bytesRead != stream.GetLength())
                {
                    AZ_Warning("BlastMaterialConversion", false, "Source file '%s' could not be read.", assetFullPath->c_str());
                    return;
                }

                // Only consider old .blastmaterial assets by checking if the legacy material library asset type id is part of the content.
                AZStd::string_view fileBufferString(reinterpret_cast<const char*>(fileBuffer.data()), bytesRead);
                if (!fileBufferString.contains(BlastMaterialLibraryAsset::RTTI_Type().ToString<AZStd::string>().c_str()))
                {
                    return;
                }

                assetDataStream->Open(AZStd::move(fileBuffer));
            }

            AZ::Data::Asset<BlastMaterialLibraryAsset> materialLibraryAsset;
            materialLibraryAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

            if (materialLibraryAssetHandler->LoadAssetDataFromStream(materialLibraryAsset, assetDataStream, nullptr) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
            {
                AZ_Warning("BlastMaterialConversion", false, "Failed to load BlastMaterialLibraryAsset asset: '%s'", assetFullPath->c_str());
                return;
            }

            BlastMaterialLibrary blastMaterialLibrary;
            blastMaterialLibrary.m_materialAssetConfigurations = materialLibraryAsset->m_materialLibrary;
            blastMaterialLibrary.m_sourceFile = AZStd::move(*assetFullPath);

            materialLibraries.push_back(AZStd::move(blastMaterialLibrary));
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr,
            assetEnumerationCB,
            nullptr);

        // Unregister and destroy the asset handler for legacy BlastMaterialLibraryAsset
        AZ::Data::AssetManager::Instance().UnregisterHandler(materialLibraryAssetHandler.get());
        materialLibraryAssetHandler.reset();

        // Register back the new MaterialAsset handler for .blastmaterial files
        AZ::Data::AssetManager::Instance().RegisterHandler(materialAssetHandler, MaterialAsset::RTTI_Type());

        return materialLibraries;
    }

    void ConvertMaterialLibrary(const BlastMaterialLibrary& materialLibrary, AZ::Data::AssetHandler* materialAssetHandler)
    {
        AZ_TracePrintf("BlastMaterialConversion", "Converting blast material library '%s' (%zu materials).\n",
            materialLibrary.m_sourceFile.c_str(),
            materialLibrary.m_materialAssetConfigurations.size());

        for (const auto& materialAssetConfiguration : materialLibrary.m_materialAssetConfigurations)
        {
            AZStd::string targetSourceFile = materialLibrary.m_sourceFile;
            AZ::StringFunc::Path::ReplaceFullName(targetSourceFile, materialAssetConfiguration.m_configuration.m_materialName.c_str(), ".blastmaterial");

            AZ_TracePrintf("BlastMaterialConversion", "Material '%s' found. Generating '%s'.\n",
                materialAssetConfiguration.m_configuration.m_materialName.c_str(),
                targetSourceFile.c_str());

            // If there is a source file with the same name already then generate a unique target source name
            int suffixNumber = 1;
            while (AZ::IO::FileIOBase::GetInstance()->Exists(targetSourceFile.c_str()))
            {
                const AZStd::string materialNameWithSuffix = AZStd::string::format("%s_%d", materialAssetConfiguration.m_configuration.m_materialName.c_str(), suffixNumber++);
                AZ_Warning("BlastMaterialConversion", false, "Source file '%s' already exists, using %s filename.", targetSourceFile.c_str(), materialNameWithSuffix.c_str());
                AZ::StringFunc::Path::ReplaceFullName(targetSourceFile, materialNameWithSuffix.c_str(), ".blastmaterial");
            }

            AZ::Data::Asset<MaterialAsset> newMaterialAsset;
            newMaterialAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            materialAssetConfiguration.CopyDataToMaterialAsset(*newMaterialAsset);

            AZStd::vector<AZ::u8> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

            if (!materialAssetHandler->SaveAssetData(newMaterialAsset, &byteStream))
            {
                AZ_Warning("BlastMaterialConversion", false, "Failed to save runtime Blast Material to object stream");
                continue; // next material
            }

            AZ::IO::FileIOStream outFileStream(targetSourceFile.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (!outFileStream.IsOpen())
            {
                AZ_Warning("BlastMaterialConversion", false, "Failed to open output file '%s'", targetSourceFile.c_str());
                continue; // next material
            }

            size_t bytesWritten = outFileStream.Write(byteBuffer.size(), byteBuffer.data());
            if (bytesWritten != byteBuffer.size())
            {
                AZ_Warning("BlastMaterialConversion", false, "Unable to save Blast Material Asset file '%s'", targetSourceFile.c_str());
                continue; // next material
            }

            // Add new file to source control (which is done by calling RequestEdit)
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                targetSourceFile.c_str(), true,
                [targetSourceFile]([[maybe_unused]] bool success, [[maybe_unused]] const AzToolsFramework::SourceControlFileInfo& info)
                {
                    AZ_Warning("BlastMaterialConversion", success, "Unable to mark for add '%s' in source control.", targetSourceFile.c_str());
                }
            );
        }

        // Delete old material library assets from source
        AZ_TracePrintf("BlastMaterialConversion", "Deleting blast material library '%s'.\n", materialLibrary.m_sourceFile.c_str());
        if (AZ::IO::FileIOBase::GetInstance()->Exists(materialLibrary.m_sourceFile.c_str()))
        {
            // Mark for deletion in source control (it will also delete the file)
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestDelete,
                materialLibrary.m_sourceFile.c_str(),
                [sourceFile = materialLibrary.m_sourceFile](bool success, [[maybe_unused]] const AzToolsFramework::SourceControlFileInfo& info)
            {
                AZ_Warning("BlastMaterialConversion", success, "Unable to mark for deletion '%s' in source control.", sourceFile.c_str());

                // If source control didn't delete it, then delete the file ourselves.
                if (!success)
                {
                    AZ::IO::FileIOBase::GetInstance()->Remove(sourceFile.c_str());
                }
            }
            );
        }

        AZ_TracePrintf("BlastMaterialConversion", "\n");
    }

    void ConvertMaterialLibrariesIntoIndividualMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        auto* materialAssetHandler = AZ::Data::AssetManager::Instance().GetHandler(MaterialAsset::RTTI_Type());
        if (!materialAssetHandler)
        {
            AZ_Error("BlastMaterialConversion", false, "Unable to find blast MaterialAsset handler.");
            return;
        }

        AZ_TracePrintf("BlastMaterialConversion", "Searching for blast material library assets to convert...\n");
        AZStd::vector<BlastMaterialLibrary> materialLibrariesToConvert = CollectMaterialLibraries(materialAssetHandler);
        if (materialLibrariesToConvert.empty())
        {
            AZ_TracePrintf("BlastMaterialConversion", "No blast material library assets found to convert.\n");
            return;
        }
        AZ_TracePrintf("BlastMaterialConversion", "Found %zu blast material libraries.\n", materialLibrariesToConvert.size());
        AZ_TracePrintf("BlastMaterialConversion", "\n");

        for (const auto& materialLibrary : materialLibrariesToConvert)
        {
            ConvertMaterialLibrary(materialLibrary, materialAssetHandler);
        }
    }
} // namespace Blast
