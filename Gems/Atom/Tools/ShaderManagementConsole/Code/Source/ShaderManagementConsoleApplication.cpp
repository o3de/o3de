/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetDatabase/AssetDatabaseConnection.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <ShaderManagementConsoleApplication.h>

#include <Document/ShaderManagementConsoleDocument.h>
#include <Window/ShaderManagementConsoleTableView.h>
#include <Window/ShaderManagementConsoleWindow.h>

#include <QMenu>

void InitShaderManagementConsoleResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(ShaderManagementConsole);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace ShaderManagementConsole
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    ShaderManagementConsoleApplication::ShaderManagementConsoleApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
        InitShaderManagementConsoleResources();

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Shader Management Console");
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        ShaderManagementConsoleRequestBus::Handler::BusConnect();
    }

    ShaderManagementConsoleApplication::~ShaderManagementConsoleApplication()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        ShaderManagementConsoleRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void ShaderManagementConsoleApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        ShaderManagementConsoleDocument::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ShaderManagementConsoleRequestBus>("ShaderManagementConsoleRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("GetSourceAssetInfo", &ShaderManagementConsoleRequestBus::Events::GetSourceAssetInfo)
                ->Event("FindMaterialAssetsUsingShader", &ShaderManagementConsoleRequestBus::Events::FindMaterialAssetsUsingShader)
                ->Event("GetMaterialInstanceShaderItems", &ShaderManagementConsoleRequestBus::Events::GetMaterialInstanceShaderItems)
                ->Event("GetAllMaterialAssetIds", &ShaderManagementConsoleRequestBus::Events::GetAllMaterialAssetIds)
                ->Event("GenerateRelativeSourcePath", &ShaderManagementConsoleRequestBus::Events::GenerateRelativeSourcePath)
                ->Event("MakeShaderOptionValueFromInt", &ShaderManagementConsoleRequestBus::Events::MakeShaderOptionValueFromInt)
                ;
        }
    }

    const char* ShaderManagementConsoleApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseShaderManagementConsole";
#elif defined(_DEBUG)
        return "DebugShaderManagementConsole";
#else
        return "ProfileShaderManagementConsole";
#endif
    }

    void ShaderManagementConsoleApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        // Overriding default document type info to provide a custom view
        auto documentTypeInfo = ShaderManagementConsoleDocument::BuildDocumentTypeInfo();
        documentTypeInfo.m_documentViewFactoryCallback = [this](const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            // Generic widget here serves to adapt the expected pointer type that AddDocumenTab takes.
            // ShaderManagementConsoleContainer derives from Layout* so it wouldn't be compatible without using this dummy intermediary.
            auto* container = new QWidget;
            new ShaderManagementConsoleContainer(container, toolId, documentId, m_window.get());
            return m_window->AddDocumentTab(documentId, container);
        };
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);

        m_window.reset(aznew ShaderManagementConsoleWindow(m_toolId));
        m_window->show();
    }

    void ShaderManagementConsoleApplication::Destroy()
    {
        m_window.reset();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> ShaderManagementConsoleApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/" });
    }

    QWidget* ShaderManagementConsoleApplication::GetAppMainWindow()
    {
        return m_window.get();
    }

    AZ::Data::AssetInfo ShaderManagementConsoleApplication::GetSourceAssetInfo(const AZStd::string& sourceAssetFileName)
    {
        bool result = false;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
            sourceAssetFileName.c_str(),
            assetInfo,
            watchFolder);
        AZ_Error(AZ::Debug::Trace::GetDefaultSystemWindow(), result, "Failed to get the asset info for the file: %s.", sourceAssetFileName.c_str());

        return assetInfo;
    }

    AZStd::vector<AZ::Data::AssetId> ShaderManagementConsoleApplication::FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath)
    {
        // Find all material types that depend on the input shader file path
        AZStd::vector<AZStd::string> materialTypeSourcePaths = AtomToolsFramework::GetPathsForAssetSourceDependentsByPath(shaderFilePath);
        AZStd::erase_if(
            materialTypeSourcePaths,
            [](const AZStd::string& path)
            {
                return !path.ends_with(AZ::RPI::MaterialTypeSourceData::Extension);
            });

        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        // Find all materials that reference any of the material types using this shader
        AZStd::vector<AzToolsFramework::AssetDatabase::ProductDatabaseEntry> productDependencies;
        for (const auto& materialTypeSourcePath : materialTypeSourcePaths)
        {
            bool result = false;
            AZStd::string watchFolder;
            AZ::Data::AssetInfo materialTypeSourceAssetInfo;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, materialTypeSourcePath.c_str(),
                materialTypeSourceAssetInfo, watchFolder);
            if (result)
            {
                assetDatabaseConnection.QueryDirectReverseProductDependenciesBySourceGuidSubId(
                    materialTypeSourceAssetInfo.m_assetId.m_guid, materialTypeSourceAssetInfo.m_assetId.m_subId,
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                    {
                        if (AzFramework::StringFunc::Path::IsExtension(entry.m_productName.c_str(), AZ::RPI::MaterialAsset::Extension))
                        {
                            productDependencies.push_back(entry);
                        }
                        return true;
                    });
            }
        }

        AZStd::vector<AZ::Data::AssetId> results;
        results.reserve(productDependencies.size());
        for (const auto& product : productDependencies)
        {
            assetDatabaseConnection.QueryCombinedByProductID(
                product.m_productID,
                [&](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined)
                {
                    results.push_back({ combined.m_sourceGuid, combined.m_subID });
                    return false;
                },
                {});
        }

        return results;
    }

    AZStd::vector<AZ::RPI::ShaderCollection::Item> ShaderManagementConsoleApplication::GetMaterialInstanceShaderItems(
        const AZ::Data::AssetId& materialAssetId)
    {
        const AZ::Data::AssetLoadParameters dontLoadImageAssets{
            [](const AZ::Data::AssetFilterInfo& filterInfo)
            {
                return
                    filterInfo.m_assetType != AZ::AzTypeInfo<AZ::RPI::StreamingImageAsset>::Uuid() &&
                    filterInfo.m_assetType != AZ::AzTypeInfo<AZ::RPI::AttachmentImageAsset>::Uuid() &&
                    filterInfo.m_assetType != AZ::AzTypeInfo<AZ::RPI::ImageAsset>::Uuid();
            }
        };

        const auto materialAssetOutcome = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(
            materialAssetId, AZ::RPI::AssetUtils::TraceLevel::Error, dontLoadImageAssets);
        if (!materialAssetOutcome)
        {
            AZ_Error(
                "ShaderManagementConsole", false, "Failed to load material asset from asset id: %s",
                materialAssetId.ToFixedString().c_str());
            return {};
        }

        const auto materialAsset = materialAssetOutcome.GetValue();
        const auto materialInstance = AZ::RPI::Material::FindOrCreate(materialAsset);
        if (!materialInstance)
        {
            AZ_Error(
                "ShaderManagementConsole", false, "Failed to create material instance from asset: %s",
                materialAsset.ToString<AZStd::string>().c_str());
            return {};
        }

        AZStd::vector<AZ::RPI::ShaderCollection::Item> shaderItems;

        materialInstance->ForAllShaderItems(
            [&](const AZ::Name&, const AZ::RPI::ShaderCollection::Item& shaderItem)
            {
                shaderItems.push_back(shaderItem);
                return true;
            });

        return shaderItems;
    }

    AZStd::vector<AZ::Data::AssetId> ShaderManagementConsoleApplication::GetAllMaterialAssetIds()
    {
        AZStd::vector<AZ::Data::AssetId> assetIds;

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB collectAssetsCb =
            [&]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (info.m_assetType == AZ::RPI::MaterialAsset::RTTI_Type())
            {
                assetIds.push_back(id);
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, collectAssetsCb, nullptr);

        return assetIds;
    }

    AZStd::string ShaderManagementConsoleApplication::GenerateRelativeSourcePath(const AZStd::string& fullShaderPath)
    {
        bool pathFound = false;
        AZStd::string relativePath, rootFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            pathFound,
            &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
            fullShaderPath,
            relativePath,
            rootFolder);

        if (pathFound)
        {
            return relativePath;
        }
        else
        {
            AZ_Error("GenerateRelativeSourcePath", false, "Can not find a relative path from the shader: '%s'.", fullShaderPath.c_str());
            return "";
        }
    }

    AZ::RPI::ShaderOptionValue ShaderManagementConsoleApplication::MakeShaderOptionValueFromInt(int value)
    {
        return AZ::RPI::ShaderOptionValue(value);
    }
} // namespace ShaderManagementConsole
