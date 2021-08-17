/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetDatabase/AssetDatabaseConnection.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <Window/ShaderManagementConsoleWindow.h>
#include <Window/ShaderManagementConsoleWindowComponent.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    void ShaderManagementConsoleWindowComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShaderManagementConsoleWindowComponent, AZ::Component>()
                ->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ShaderManagementConsoleRequestBus>("ShaderManagementConsoleRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("GetSourceAssetInfo", &ShaderManagementConsoleRequestBus::Events::GetSourceAssetInfo)
                ->Event("FindMaterialAssetsUsingShader", &ShaderManagementConsoleRequestBus::Events::FindMaterialAssetsUsingShader )
                ->Event("GetMaterialInstanceShaderItems", &ShaderManagementConsoleRequestBus::Events::GetMaterialInstanceShaderItems)
                ;
        }
    }

    void ShaderManagementConsoleWindowComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AssetBrowserService"));
        required.push_back(AZ_CRC_CE("PropertyManagerService"));
        required.push_back(AZ_CRC_CE("SourceControlService"));
        required.push_back(AZ_CRC_CE("AtomToolsMainWindowSystemService"));
    }

    void ShaderManagementConsoleWindowComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShaderManagementConsoleWindowService"));
    }

    void ShaderManagementConsoleWindowComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShaderManagementConsoleWindowService"));
    }

    void ShaderManagementConsoleWindowComponent::Init()
    {
    }

    void ShaderManagementConsoleWindowComponent::Activate()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusConnect();
        ShaderManagementConsoleRequestBus::Handler::BusConnect();
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequests::EnableSourceControl, true);
    }

    void ShaderManagementConsoleWindowComponent::Deactivate()
    {
        ShaderManagementConsoleRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();

        m_window.reset();
    }

    QWidget* ShaderManagementConsoleWindowComponent::GetAppMainWindow()
    {
        return m_window.get();
    }

    void ShaderManagementConsoleWindowComponent::CreateMainWindow()
    {
        m_assetBrowserInteractions.reset(aznew ShaderManagementConsoleBrowserInteractions);

        m_window.reset(aznew ShaderManagementConsoleWindow);
        m_window->show();
    }

    void ShaderManagementConsoleWindowComponent::DestroyMainWindow()
    {
        m_window.reset();
    }

    AZ::Data::AssetInfo ShaderManagementConsoleWindowComponent::GetSourceAssetInfo(const AZStd::string& sourceAssetFileName)
    {
        bool result = false;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, sourceAssetFileName.c_str(), assetInfo,
            watchFolder);
        AZ_Error(nullptr, result, "Failed to get the asset info for the file: %s.", sourceAssetFileName.c_str());

        return assetInfo;
    }

    AZStd::vector<AZ::Data::AssetId> ShaderManagementConsoleWindowComponent::FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath)
    {
        // Collect the material types referencing the shader
        AZStd::vector<AZStd::string> materialTypeSources;

        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            shaderFilePath.c_str(),
            nullptr,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry) {
                AZStd::string assetExtension;
                if (AzFramework::StringFunc::Path::GetExtension(sourceFileDependencyEntry.m_source.c_str(), assetExtension, false))
                {
                    if (assetExtension == "materialtype")
                    {
                        materialTypeSources.push_back(sourceFileDependencyEntry.m_source);
                    }
                }
                return true;
            });

        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productDependencies;
        for (const auto& materialTypeSource : materialTypeSources)
        {
            bool result = false;
            AZ::Data::AssetInfo materialTypeSourceAssetInfo;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                materialTypeSource.c_str(),
                materialTypeSourceAssetInfo,
                watchFolder
            );

            assetDatabaseConnection.QueryDirectReverseProductDependenciesBySourceGuidSubId(
                materialTypeSourceAssetInfo.m_assetId.m_guid,
                materialTypeSourceAssetInfo.m_assetId.m_subId,
                [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry) {
                    AZStd::string assetExtension;
                    if (AzFramework::StringFunc::Path::GetExtension(entry.m_productName.c_str(), assetExtension, false))
                    {
                        if (assetExtension == "azmaterial")
                        {
                            productDependencies.push_back(entry);
                        }
                    }
                    return true;
                });
        }

        AZStd::vector<AZ::Data::AssetId> results;
        results.reserve(productDependencies.size());
        for (auto product : productDependencies)
        {
            assetDatabaseConnection.QueryCombinedByProductID(
                product.m_productID,
                [&](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined) {
                    results.push_back({combined.m_sourceGuid, combined.m_subID});
                    return false;
                },
                nullptr
            );
        }
        return results;
    }

    AZStd::vector<AZ::RPI::ShaderCollection::Item> ShaderManagementConsoleWindowComponent::GetMaterialInstanceShaderItems(const AZ::Data::AssetId& assetId)
    {
        auto materialAsset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::MaterialAsset>(assetId, AZ::RPI::AssetUtils::TraceLevel::Error);

        auto materialInstance = AZ::RPI::Material::Create(materialAsset);
        AZ_Error(nullptr, materialAsset, "Failed to get a material instance from product asset id: %s", assetId.ToString<AZStd::string>().c_str());

        if (materialInstance != nullptr)
        {
            return AZStd::vector<AZ::RPI::ShaderCollection::Item>(materialInstance->GetShaderCollection().begin(), materialInstance->GetShaderCollection().end());
        }
        return AZStd::vector<AZ::RPI::ShaderCollection::Item>();
    }
}
