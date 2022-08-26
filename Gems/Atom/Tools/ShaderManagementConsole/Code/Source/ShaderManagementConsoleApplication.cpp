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
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <ShaderManagementConsoleApplication.h>

#include <Document/ShaderManagementConsoleDocument.h>
#include <Window/ShaderManagementConsoleTableView.h>
#include <Window/ShaderManagementConsoleWindow.h>

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
                ->Event("GetMaterialInstanceShaderItems", &ShaderManagementConsoleRequestBus::Events::GetMaterialInstanceShaderItems);
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
        documentTypeInfo.m_documentViewFactoryCallback = [this](const AZ::Crc32& toolId, const AZ::Uuid& documentId) {
            return m_window->AddDocumentTab(documentId, new ShaderManagementConsoleTableView(toolId, documentId, m_window.get()));
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
        AZ_Error(nullptr, result, "Failed to get the asset info for the file: %s.", sourceAssetFileName.c_str());

        return assetInfo;
    }

    AZStd::vector<AZ::Data::AssetId> ShaderManagementConsoleApplication::FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath)
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        // Find all material types that reference shaderFilePath
        AZStd::list<AZStd::string> materialTypeSources;

        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            shaderFilePath.c_str(), nullptr, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
            {
                if (AzFramework::StringFunc::Path::IsExtension(
                        sourceFileDependencyEntry.m_source.c_str(), AZ::RPI::MaterialTypeSourceData::Extension))
                {
                    materialTypeSources.push_back(sourceFileDependencyEntry.m_source);
                }
                return true;
            });

        // Find all materials that reference any of the material types using this shader
        AZStd::string watchFolder;
        AZ::Data::AssetInfo materialTypeSourceAssetInfo;
        AZStd::list<AzToolsFramework::AssetDatabase::ProductDatabaseEntry> productDependencies;
        for (const auto& materialTypeSource : materialTypeSources)
        {
            bool result = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, materialTypeSource.c_str(),
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
                nullptr);
        }

        return results;
    }

    AZStd::vector<AZ::RPI::ShaderCollection::Item> ShaderManagementConsoleApplication::GetMaterialInstanceShaderItems(
        const AZ::Data::AssetId& materialAssetId)
    {
        auto materialAsset =
            AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::MaterialAsset>(materialAssetId, AZ::RPI::AssetUtils::TraceLevel::Error);
        if (!materialAsset.IsReady())
        {
            AZ_Error(
                "ShaderManagementConsole", false, "Failed to load material asset from asset id: %s",
                materialAssetId.ToFixedString().c_str());
            return AZStd::vector<AZ::RPI::ShaderCollection::Item>();
        }

        auto materialInstance = AZ::RPI::Material::Create(materialAsset);
        if (!materialInstance)
        {
            AZ_Error(
                "ShaderManagementConsole", false, "Failed to create material instance from asset: %s",
                materialAsset.ToString<AZStd::string>().c_str());
            return AZStd::vector<AZ::RPI::ShaderCollection::Item>();
        }

        return AZStd::vector<AZ::RPI::ShaderCollection::Item>(
            materialInstance->GetShaderCollection().begin(), materialInstance->GetShaderCollection().end());
    }
} // namespace ShaderManagementConsole
