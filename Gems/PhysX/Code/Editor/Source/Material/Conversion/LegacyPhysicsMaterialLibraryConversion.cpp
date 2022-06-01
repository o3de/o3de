/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Asset/GenericAssetHandler.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>

#include <Editor/Source/Material/PhysXEditorMaterialAsset.h>

#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialLibraryConversion.h>

namespace PhysicsLegacy
{
    // O3DE_DEPRECATION_NOTICE(GHI-9840)
    // Default values used for initializing materials.
    // Use MaterialConfiguration to define properties for materials at the time of creation.
    class MaterialConfiguration
    {
    public:
        AZ_RTTI(PhysicsLegacy::MaterialConfiguration, "{8807CAA1-AD08-4238-8FDB-2154ADD084A1}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PhysicsLegacy::MaterialConfiguration>()
                    ->Version(3)
                    ->Field("SurfaceType", &MaterialConfiguration::m_surfaceType)
                    ->Field("DynamicFriction", &MaterialConfiguration::m_dynamicFriction)
                    ->Field("StaticFriction", &MaterialConfiguration::m_staticFriction)
                    ->Field("Restitution", &MaterialConfiguration::m_restitution)
                    ->Field("FrictionCombine", &MaterialConfiguration::m_frictionCombine)
                    ->Field("RestitutionCombine", &MaterialConfiguration::m_restitutionCombine)
                    ->Field("Density", &MaterialConfiguration::m_density)
                    ->Field("DebugColor", &MaterialConfiguration::m_debugColor);
            }
        }

        MaterialConfiguration() = default;
        virtual ~MaterialConfiguration() = default;

        AZStd::string m_surfaceType{ "Default" };
        float m_dynamicFriction = 0.5f;
        float m_staticFriction = 0.5f;
        float m_restitution = 0.5f;
        float m_density = 1000.0f;

        PhysX::CombineMode m_restitutionCombine = PhysX::CombineMode::Average;
        PhysX::CombineMode m_frictionCombine = PhysX::CombineMode::Average;

        AZ::Color m_debugColor = AZ::Colors::White;
    };

    // O3DE_DEPRECATION_NOTICE(GHI-9840)
    // A single Material entry in the material library
    // MaterialLibraryAsset holds a collection of MaterialFromAssetConfiguration instances.
    class MaterialFromAssetConfiguration
    {
    public:
        AZ_TYPE_INFO(PhysicsLegacy::MaterialFromAssetConfiguration, "{FBD76628-DE57-435E-BE00-6FFAE64DDF1D}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PhysicsLegacy::MaterialFromAssetConfiguration>()
                    ->Version(1)
                    ->Field("Configuration", &MaterialFromAssetConfiguration::m_configuration)
                    ->Field("UID", &MaterialFromAssetConfiguration::m_id);
            }
        }

        MaterialConfiguration m_configuration;
        MaterialId m_id;

        void CopyDataToMaterialAsset(PhysX::EditorMaterialAsset& materialAsset) const
        {
            materialAsset.m_materialConfiguration.m_dynamicFriction = m_configuration.m_dynamicFriction;
            materialAsset.m_materialConfiguration.m_staticFriction = m_configuration.m_staticFriction;
            materialAsset.m_materialConfiguration.m_restitution = m_configuration.m_restitution;
            materialAsset.m_materialConfiguration.m_density = m_configuration.m_density;
            materialAsset.m_materialConfiguration.m_restitutionCombine = m_configuration.m_restitutionCombine;
            materialAsset.m_materialConfiguration.m_frictionCombine = m_configuration.m_frictionCombine;
            materialAsset.m_materialConfiguration.m_debugColor = m_configuration.m_debugColor;
            materialAsset.m_legacyPhysicsMaterialId = m_id;
        }
    };

    // O3DE_DEPRECATION_NOTICE(GHI-9840)
    // An asset that holds a list of materials.
    class MaterialLibraryAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialLibraryAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysicsLegacy::MaterialLibraryAsset, "{9E366D8C-33BB-4825-9A1F-FA3ADBE11D0F}", AZ::Data::AssetData);

        MaterialLibraryAsset() = default;
        virtual ~MaterialLibraryAsset() = default;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PhysicsLegacy::MaterialLibraryAsset, AZ::Data::AssetData>()
                    ->Version(2)
                    ->Field("Properties", &MaterialLibraryAsset::m_materialLibrary);
            }
        }

        AZStd::vector<MaterialFromAssetConfiguration> m_materialLibrary;
    };
} // namespace PhysicsLegacy

namespace PhysX
{
    void ConvertMaterialLibrariesIntoIndividualMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC("ed_physxConvertMaterialLibrariesIntoIndividualMaterials", ConvertMaterialLibrariesIntoIndividualMaterials, AZ::ConsoleFunctorFlags::Null,
        "Finds legacy physics material library assets in the project and generates new individual PhysX material assets. Original library assets will be deleted.");

    void ReflectLegacyMaterialClasses(AZ::ReflectContext* context)
    {
        PhysicsLegacy::MaterialConfiguration::Reflect(context);
        PhysicsLegacy::MaterialFromAssetConfiguration::Reflect(context);
        PhysicsLegacy::MaterialLibraryAsset::Reflect(context);
    }

    struct PhysicsMaterialLibrary
    {
        AZStd::vector<PhysicsLegacy::MaterialFromAssetConfiguration> m_materialAssetConfigurations;
        AZStd::string m_sourceFile; // Path to material library source file
    };

    // Collects all legacy material libraries to convert to new material assets
    AZStd::vector<PhysicsMaterialLibrary> CollectMaterialLibraries()
    {
        AZStd::vector<PhysicsMaterialLibrary> materialLibraries;

        // Create and register the asset handler for legacy MaterialLibraryAsset to handle old .physmaterial files
        auto materialLibraryAssetHandler = AZStd::make_unique<AzFramework::GenericAssetHandler<PhysicsLegacy::MaterialLibraryAsset>>(
            "Physics Material", "Physics Material", "physmaterial");
        AZ::Data::AssetManager::Instance().RegisterHandler(materialLibraryAssetHandler.get(), PhysicsLegacy::MaterialLibraryAsset::RTTI_Type());

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&materialLibraryAssetHandler, &materialLibraries](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != PhysicsLegacy::MaterialLibraryAsset::RTTI_Type())
            {
                return;
            }

            AZStd::optional<AZStd::string> assetFullPath = Physics::Utils::GetFullSourceAssetPathById(assetId);
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
                    AZ_Warning("PhysXMaterialConversion", false, "Source file '%s' could not be opened.", assetFullPath->c_str());
                    return;
                }
                AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
                size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
                if (bytesRead != stream.GetLength())
                {
                    AZ_Warning("PhysXMaterialConversion", false, "Source file '%s' could not be read.", assetFullPath->c_str());
                    return;
                }

                assetDataStream->Open(AZStd::move(fileBuffer));
            }

            AZ::Data::Asset<PhysicsLegacy::MaterialLibraryAsset> materialLibraryAsset;
            materialLibraryAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

            if (materialLibraryAssetHandler->LoadAssetDataFromStream(materialLibraryAsset, assetDataStream, nullptr) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
            {
                AZ_Warning("PhysXMaterialConversion", false, "Failed to load legacy MaterialLibraryAsset asset: '%s'", assetFullPath->c_str());
                return;
            }

            PhysicsMaterialLibrary physicsMaterialLibrary;
            physicsMaterialLibrary.m_materialAssetConfigurations = materialLibraryAsset->m_materialLibrary;
            physicsMaterialLibrary.m_sourceFile = AZStd::move(*assetFullPath);

            materialLibraries.push_back(AZStd::move(physicsMaterialLibrary));
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr,
            assetEnumerationCB,
            nullptr);

        // Unregister and destroy the asset handler for legacy MaterialLibraryAsset
        AZ::Data::AssetManager::Instance().UnregisterHandler(materialLibraryAssetHandler.get());
        materialLibraryAssetHandler.reset();

        return materialLibraries;
    }

    void ConvertMaterialLibrary(const PhysicsMaterialLibrary& materialLibrary, AZ::Data::AssetHandler* materialAssetHandler)
    {
        AZ_TracePrintf("PhysXMaterialConversion", "Converting physics material library '%s' (%zu materials).\n",
            materialLibrary.m_sourceFile.c_str(),
            materialLibrary.m_materialAssetConfigurations.size());

        for (const auto& materialAssetConfiguration : materialLibrary.m_materialAssetConfigurations)
        {
            AZStd::string targetSourceFile = materialLibrary.m_sourceFile;
            AZ::StringFunc::Path::ReplaceFullName(
                targetSourceFile,
                materialAssetConfiguration.m_configuration.m_surfaceType.c_str(),
                AZStd::string::format(".%s", EditorMaterialAsset::FileExtension).c_str());

            AZ_TracePrintf("PhysXMaterialConversion", "Material '%s' found. Generating '%s'.\n",
                materialAssetConfiguration.m_configuration.m_surfaceType.c_str(),
                targetSourceFile.c_str());

            // If there is a source file with the same name already then generate a unique target source name
            int suffixNumber = 1;
            while (AZ::IO::FileIOBase::GetInstance()->Exists(targetSourceFile.c_str()))
            {
                const AZStd::string materialNameWithSuffix = AZStd::string::format("%s_%d", materialAssetConfiguration.m_configuration.m_surfaceType.c_str(), suffixNumber++);
                AZ_Warning("PhysXMaterialConversion", false, "Source file '%s' already exists, using %s filename.", targetSourceFile.c_str(), materialNameWithSuffix.c_str());
                AZ::StringFunc::Path::ReplaceFullName(
                    targetSourceFile,
                    materialNameWithSuffix.c_str(),
                    AZStd::string::format(".%s", EditorMaterialAsset::FileExtension).c_str());
            }

            AZ::Data::Asset<EditorMaterialAsset> newMaterialAsset;
            newMaterialAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            materialAssetConfiguration.CopyDataToMaterialAsset(*newMaterialAsset);

            AZStd::vector<AZ::u8> byteBuffer;
            AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

            if (!materialAssetHandler->SaveAssetData(newMaterialAsset, &byteStream))
            {
                AZ_Warning("PhysXMaterialConversion", false, "Failed to save runtime PhysX Material to object stream");
                continue; // next material
            }

            AZ::IO::FileIOStream outFileStream(targetSourceFile.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (!outFileStream.IsOpen())
            {
                AZ_Warning("PhysXMaterialConversion", false, "Failed to open output file '%s'", targetSourceFile.c_str());
                continue; // next material
            }

            size_t bytesWritten = outFileStream.Write(byteBuffer.size(), byteBuffer.data());
            if (bytesWritten != byteBuffer.size())
            {
                AZ_Warning("PhysXMaterialConversion", false, "Unable to save PhysX Material Asset file '%s'", targetSourceFile.c_str());
                continue; // next material
            }

            // Add new file to source control (which is done by calling RequestEdit)
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                targetSourceFile.c_str(), true,
                [targetSourceFile]([[maybe_unused]] bool success, [[maybe_unused]] const AzToolsFramework::SourceControlFileInfo& info)
                {
                    AZ_UNUSED(targetSourceFile);
                    AZ_Warning("PhysXMaterialConversion", success, "Unable to mark for add '%s' in source control.", targetSourceFile.c_str());
                }
            );
        }

        // Delete old material library assets from source
        AZ_TracePrintf("PhysXMaterialConversion", "Deleting legacy physics material library '%s'.\n", materialLibrary.m_sourceFile.c_str());
        if (AZ::IO::FileIOBase::GetInstance()->Exists(materialLibrary.m_sourceFile.c_str()))
        {
            // Mark for deletion in source control (it will also delete the file)
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestDelete,
                materialLibrary.m_sourceFile.c_str(),
                [sourceFile = materialLibrary.m_sourceFile](bool success, [[maybe_unused]] const AzToolsFramework::SourceControlFileInfo& info)
            {
                AZ_Warning("PhysXMaterialConversion", success, "Unable to mark for deletion '%s' in source control.", sourceFile.c_str());

                // If source control didn't delete it, then delete the file ourselves.
                if (!success)
                {
                    AZ::IO::FileIOBase::GetInstance()->Remove(sourceFile.c_str());
                }
            }
            );
        }

        AZ_TracePrintf("PhysXMaterialConversion", "\n");
    }

    void ConvertMaterialLibrariesIntoIndividualMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        auto* materialAssetHandler = AZ::Data::AssetManager::Instance().GetHandler(EditorMaterialAsset::RTTI_Type());
        if (!materialAssetHandler)
        {
            AZ_Error("PhysXMaterialConversion", false, "Unable to find PhysX EditorMaterialAsset handler.");
            return;
        }

        AZ_TracePrintf("PhysXMaterialConversion", "Searching for physics material library assets to convert...\n");
        AZStd::vector<PhysicsMaterialLibrary> materialLibrariesToConvert = CollectMaterialLibraries();
        if (materialLibrariesToConvert.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No physics material library assets found to convert.\n");
            return;
        }
        AZ_TracePrintf("PhysXMaterialConversion", "Found %zu physics material libraries.\n", materialLibrariesToConvert.size());
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        for (const auto& materialLibrary : materialLibrariesToConvert)
        {
            ConvertMaterialLibrary(materialLibrary, materialAssetHandler);
        }
    }
} // namespace PhysX
