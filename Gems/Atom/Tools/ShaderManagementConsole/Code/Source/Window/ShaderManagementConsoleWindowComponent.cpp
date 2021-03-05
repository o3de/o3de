/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RPI.Edit/Common/JsonUtils.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include <AssetDatabase/AssetDatabaseConnection.h>

#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Source/Window/ShaderManagementConsoleWindowComponent.h>
#include <Source/Window/ShaderManagementConsoleWindow.h>

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
            behaviorContext->EBus<ShaderManagementConsoleWindowRequestBus>("ShaderManagementConsoleWindowRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("CreateShaderManagementConsoleWindow", &ShaderManagementConsoleWindowRequestBus::Events::CreateShaderManagementConsoleWindow)
                ->Event("DestroyShaderManagementConsoleWindow", &ShaderManagementConsoleWindowRequestBus::Events::DestroyShaderManagementConsoleWindow)
                ->Event("GenerateShaderVariantListForShaderMaterials", &ShaderManagementConsoleWindowRequestBus::Events::GenerateShaderVariantListForShaderMaterials)
                ;
        }
    }

    void ShaderManagementConsoleWindowComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetBrowserService", 0x1e54fffb));
        required.push_back(AZ_CRC("PropertyManagerService", 0x63a3d7ad));
        required.push_back(AZ_CRC("SourceControlService", 0x67f338fd));
    }

    void ShaderManagementConsoleWindowComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ShaderManagementConsoleWindowService", 0xb6e7d922));
    }

    void ShaderManagementConsoleWindowComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ShaderManagementConsoleWindowService", 0xb6e7d922));
    }

    void ShaderManagementConsoleWindowComponent::Init()
    {
    }

    void ShaderManagementConsoleWindowComponent::Activate()
    {
        ShaderManagementConsoleWindowRequestBus::Handler::BusConnect();
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequests::EnableSourceControl, true);
    }

    void ShaderManagementConsoleWindowComponent::Deactivate()
    {
        ShaderManagementConsoleWindowRequestBus::Handler::BusDisconnect();

        m_window.reset();
    }

    void ShaderManagementConsoleWindowComponent::CreateShaderManagementConsoleWindow()
    {
        m_assetBrowserInteractions.reset(aznew ShaderManagementConsoleBrowserInteractions);

        m_window.reset(aznew ShaderManagementConsoleWindow);
        m_window->show();
    }

    void ShaderManagementConsoleWindowComponent::DestroyShaderManagementConsoleWindow()
    {
        m_window.reset();
    }

    void ShaderManagementConsoleWindowComponent::GenerateShaderVariantForMaterials(AZStd::string_view destFilePath, AZStd::string_view shaderFilePath, const AZStd::vector<ShaderVariantListInfo>& shaderVariantListInfoList)
    {
        AZ::RPI::ShaderVariantListSourceData shaderVariantList;
        shaderVariantList.m_shaderFilePath = shaderFilePath;
        uint32_t stableId = 1;

        for (const auto& shaderVariantListInfo : shaderVariantListInfoList)
        {
            for (const auto& shaderItem : shaderVariantListInfo.m_shaderItems)
            {
                AZ::RPI::ShaderVariantListSourceData::VariantInfo variantInfo;
                variantInfo.m_stableId = stableId;

                const AZ::RPI::ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                auto& shaderOptionDescriptors = layout->GetShaderOptions();

                AZ::RPI::ShaderOptionGroup shaderVariantOptions{ layout, shaderItem.GetShaderVariantId() };

                for (size_t i = 0; i < shaderOptionDescriptors.size(); ++i)
                {
                    const AZ::RPI::ShaderOptionDescriptor& shaderOptionDesc = shaderOptionDescriptors[i];
                    auto optionName = shaderOptionDesc.GetName();

                    AZ::RPI::ShaderOptionValue optionValue = shaderVariantOptions.GetValue(AZ::RPI::ShaderOptionIndex(i));
                    if (!optionValue.IsValid())
                    {
                        optionValue = shaderOptionDesc.FindValue(shaderOptionDesc.GetDefaultValue());
                    }

                    auto valueName = shaderOptionDesc.GetValueName(optionValue);

                    variantInfo.m_options.insert(AZStd::make_pair(optionName.GetCStr(), valueName.GetCStr()));
                }

                if (!variantInfo.m_options.empty())
                {
                    shaderVariantList.m_shaderVariants.push_back(variantInfo);
                    stableId++;
                }
            }
        }

        bool result = AZ::RPI::JsonUtils::SaveObjectToFile(destFilePath, shaderVariantList);
        AZ_Error(nullptr, result, "Failed to save the file: %s.", destFilePath);

        ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::OpenDocument, destFilePath);
    }

    void ShaderManagementConsoleWindowComponent::GenerateShaderVariantListForShaderMaterials(const char* shaderFileName)
    {
        // A set of unique variants
        AZStd::set<AZ::RPI::ShaderVariantId> shaderVariantIds;

        bool result = false;
        AZ::Data::AssetInfo shaderAssetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
            shaderFileName, shaderAssetInfo, watchFolder);
        AZ_Error(nullptr, result, "Failed to get the asset info for the file: %s.", shaderFileName);

        AZStd::string shaderFilePath;
        AzFramework::StringFunc::Path::GetFullFileName(shaderAssetInfo.m_relativePath.c_str(), shaderFilePath);

        // Connect to the asset database to find asset dependenies
        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        // Collect the material types referencing the shader
        AZStd::vector<AZStd::string> materialTypeSources;

        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            shaderAssetInfo.m_relativePath.c_str(),
            nullptr,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
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

        // Collect the unique materials referencing the dependent material types
        AZStd::set<AZStd::string> materialSources;

        for (const auto& materialTypeSource : materialTypeSources)
        {
            //[ATOM-13342] Handle materials that inherit from another material

            assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
                materialTypeSource.c_str(),
                nullptr,
                AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
                [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
                {
                    AZStd::string assetExtension;
                    if (AzFramework::StringFunc::Path::GetExtension(sourceFileDependencyEntry.m_source.c_str(), assetExtension, false))
                    {
                        // [ATOM-13341] Handle materials from FBX files as well

                        if (assetExtension == "material")
                        {
                            materialSources.insert(sourceFileDependencyEntry.m_source);
                        }
                    }
                    return true;
                });
        }

        // Load each material to obtain the shader items
        AZStd::vector<ShaderVariantListInfo> shaderVariantListInfoList;

        for (const auto& materialSource : materialSources)
        {
            AZStd::string materialAssetPath = materialSource;
            AzFramework::StringFunc::Path::ReplaceExtension(materialAssetPath, "azmaterial");

            auto materialAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(materialAssetPath.c_str());
            AZ_Error(nullptr, materialAsset, "Failed to load the material asset: %s.", materialAssetPath.c_str());

            auto materialInstance = AZ::RPI::Material::FindOrCreate(materialAsset);
            AZ_Error(nullptr, materialAsset, "Failed to get the material: %s.", materialAssetPath.c_str());

            AZStd::vector<AZ::RPI::ShaderCollection::Item> shaderItems;

            for (auto& shaderItem : materialInstance->GetShaderCollection())
            {
                AZ::Data::AssetId shaderAssetId = shaderItem.GetShaderAsset().GetId();

                // Check that this shader item matches the requested shader
                if (shaderAssetInfo.m_assetId == shaderAssetId)
                {
                    if (shaderItem.GetShaderVariantId().m_key != 0)
                    {
                        auto it = shaderVariantIds.insert(shaderItem.GetShaderVariantId());
                        if (it.second)
                        {
                            shaderItems.push_back(shaderItem);
                        }
                    }
                }
            }
             
            if (!shaderItems.empty())
            {
                shaderVariantListInfoList.push_back({
                    materialSource,
                    shaderItems });
            }
        }

        if (shaderVariantListInfoList.empty())
        {
            AZ_TracePrintf("Shader Management Console", "There are no .shadervariantlist assets to generate from the shader %s.", shaderAssetInfo.m_relativePath.c_str());
            return;
        }

        // Ask the user if they want to proceed
        QStringList materialNames;
        std::transform(shaderVariantListInfoList.begin(), shaderVariantListInfoList.end(), std::back_inserter(materialNames),
            [](const auto& entry) { return entry.m_materialFileName.c_str(); });
        materialNames.sort();

        QString ask = QString("Generate the .shadervariantlist asset?\n\nSource:\n%1\n\nMaterials:\n%2")
            .arg(shaderAssetInfo.m_relativePath.c_str()).arg(materialNames.join("\n"));
        if (QMessageBox::question(m_window.get(), "ShaderManagementConsole", ask) != QMessageBox::Yes)
        {
            return;
        }

        // Compute the file name for the Shader Variant List based on the shader file name.
        AZStd::string variantListFilePath = shaderFileName;
        AzFramework::StringFunc::Path::ReplaceExtension(variantListFilePath, "shadervariantlist");

        GenerateShaderVariantForMaterials(variantListFilePath, shaderFilePath, shaderVariantListInfoList);
    }
}
