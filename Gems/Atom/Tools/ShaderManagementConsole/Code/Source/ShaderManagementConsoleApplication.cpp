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
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <Document/ShaderManagementConsoleDocument.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <ShaderManagementConsoleApplication.h>
#include <ShaderManagementConsoleRequestBus.h>
#include <ShaderManagementConsole_Traits_Platform.h>

#include <QDesktopServices>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>

void InitShaderManagementConsoleResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(ShaderManagementConsole);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace ShaderManagementConsole
{
    ShaderManagementConsoleApplication::ShaderManagementConsoleApplication(int* argc, char*** argv)
        : Base(argc, argv)
    {
        InitShaderManagementConsoleResources();

        QApplication::setApplicationName("O3DE Shader Management Console");

        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());

        ShaderManagementConsoleRequestBus::Handler::BusConnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusConnect();
    }

    ShaderManagementConsoleApplication::~ShaderManagementConsoleApplication()
    {
        ShaderManagementConsoleRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void ShaderManagementConsoleApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);

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

            behaviorContext->EBus<ShaderManagementConsoleDocumentRequestBus>("ShaderManagementConsoleDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("SetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData)
                ->Event("GetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData)
                ->Event("GetShaderOptionCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionCount)
                ->Event("GetShaderOptionDescriptor", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor)
                ->Event("GetShaderVariantCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantCount)
                ->Event("GetShaderVariantInfo", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantInfo)
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

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []() { return aznew ShaderManagementConsoleDocument(); });
    }

    AZStd::string ShaderManagementConsoleApplication::GetBuildTargetName() const
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        //! Returns the build system target name of "ShaderManagementConsole"
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }

    AZStd::vector<AZStd::string> ShaderManagementConsoleApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/" });
    }

    QWidget* ShaderManagementConsoleApplication::GetAppMainWindow()
    {
        return m_window.get();
    }

    void ShaderManagementConsoleApplication::CreateMainWindow()
    {
        m_window.reset(aznew ShaderManagementConsoleWindow);
        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);
        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            []([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                if (AzFramework::StringFunc::Path::IsExtension(
                        entries.front()->GetFullPath().c_str(), AZ::RPI::ShaderSourceData::Extension))
                {
                    menu->addAction("Generate Shader Variant List", [entries]()
                        {
                            const QString script =
                                "@engroot@/Gems/Atom/Tools/ShaderManagementConsole/Scripts/GenerateShaderVariantListForMaterials.py";
                            AZStd::vector<AZStd::string_view> pythonArgs{ entries.front()->GetFullPath() };
                            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, script.toUtf8().constData(),
                                pythonArgs);
                        });
                }
                else if (AzFramework::StringFunc::Path::IsExtension(
                             entries.front()->GetFullPath().c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument,
                                entries.front()->GetFullPath());
                        });
                }
                else
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            QDesktopServices::openUrl(QUrl::fromLocalFile(entries.front()->GetFullPath().c_str()));
                        });
                }
            });
    }

    void ShaderManagementConsoleApplication::DestroyMainWindow()
    {
        m_window.reset();
    }

    AZ::Data::AssetInfo ShaderManagementConsoleApplication::GetSourceAssetInfo(const AZStd::string& sourceAssetFileName)
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

    AZStd::vector<AZ::Data::AssetId> ShaderManagementConsoleApplication::FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath)
    {
        // Collect the material types referencing the shader
        AZStd::vector<AZStd::string> materialTypeSources;

        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            shaderFilePath.c_str(), nullptr, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
            {
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
                result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, materialTypeSource.c_str(),
                materialTypeSourceAssetInfo, watchFolder);

            assetDatabaseConnection.QueryDirectReverseProductDependenciesBySourceGuidSubId(
                materialTypeSourceAssetInfo.m_assetId.m_guid, materialTypeSourceAssetInfo.m_assetId.m_subId,
                [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                {
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
        const AZ::Data::AssetId& assetId)
    {
        auto materialAsset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::MaterialAsset>(assetId, AZ::RPI::AssetUtils::TraceLevel::Error);

        auto materialInstance = AZ::RPI::Material::Create(materialAsset);
        AZ_Error(
            nullptr, materialAsset, "Failed to get a material instance from product asset id: %s",
            assetId.ToString<AZStd::string>().c_str());

        if (materialInstance != nullptr)
        {
            return AZStd::vector<AZ::RPI::ShaderCollection::Item>(
                materialInstance->GetShaderCollection().begin(), materialInstance->GetShaderCollection().end());
        }
        return AZStd::vector<AZ::RPI::ShaderCollection::Item>();
    }
} // namespace ShaderManagementConsole
