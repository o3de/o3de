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
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderOptionValuesSourceData.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Document/ShaderManagementConsoleDocument.h>

namespace ShaderManagementConsole
{
    void ShaderManagementConsoleDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShaderManagementConsoleDocument, AtomToolsFramework::AtomToolsDocument>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ShaderManagementConsoleDocumentRequestBus>("ShaderManagementConsoleDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("SetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData)
                ->Event("GetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData)
                ->Event("GetShaderOptionDescriptorCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptorCount)
                ->Event("GetShaderOptionDescriptor", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor)
                ;
        }
    }

    ShaderManagementConsoleDocument::ShaderManagementConsoleDocument(
        const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo)
        : AtomToolsFramework::AtomToolsDocument(toolId, documentTypeInfo)
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusConnect(m_id);
    }

    ShaderManagementConsoleDocument::~ShaderManagementConsoleDocument()
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleDocument::SetShaderVariantListSourceData(
        const AZ::RPI::ShaderVariantListSourceData& shaderVariantListSourceData)
    {
        m_shaderVariantListSourceData = shaderVariantListSourceData;
        AZStd::string shaderPath = m_shaderVariantListSourceData.m_shaderFilePath;

        auto shaderAssetResult = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::ShaderAsset>(m_absolutePath, shaderPath);
        if (shaderAssetResult)
        {
            m_shaderAsset = shaderAssetResult.GetValue();

            // No material is using this shader, check for system option settings
            if (m_shaderVariantListSourceData.m_shaderVariants.empty())
            {
                // Read system option file
                AZ::IO::Path fullPath = AZ::IO::Path(AZ::RPI::AssetUtils::ResolvePathReference(m_absolutePath, shaderPath));
                fullPath.ReplaceExtension("systemoptions");

                AZ::RPI::ShaderOptionValuesSourceData systemOptionSetting;
                if (!AZ::RPI::JsonUtils::LoadObjectFromFile(fullPath.String(), systemOptionSetting))
                {
                    AZ_Warning("ShaderManagementConsoleDocument", false, "System option setting not found : '%s.'", fullPath.c_str());
                    return;
                }

                if (systemOptionSetting.size() > 0)
                {
                    AZ::u32 stableId = 1;
                    AZStd::vector<AZ::RPI::ShaderOptionDescriptor> unsetOption;
                    const auto& shaderOptionDescriptors = m_shaderAsset->GetShaderOptionGroupLayout()->GetShaderOptions();

                    // Check user input with descriptor from shader asset
                    for (auto& shaderOptionDescriptor : shaderOptionDescriptors)
                    {
                        AZ::Name optionName = shaderOptionDescriptor.GetName();
                        const auto optionIt = systemOptionSetting.find(optionName);
                        if (optionIt != systemOptionSetting.end())
                        {
                            AZ::Name valueName = AZ::Name(optionIt->second);
                            if (strcmp(valueName.GetCStr(), "") == 0)
                            {
                                // Option with unset value, expend later
                                unsetOption.push_back(shaderOptionDescriptor);
                                systemOptionSetting[optionName] = shaderOptionDescriptor.GetDefaultValue();
                            }
                        }
                    }

                    // Get total number of variants
                    size_t totalVariantSize = 1;
                    for (auto& shaderOptionDescriptor : unsetOption)
                    {
                        uint32_t minValue = shaderOptionDescriptor.GetMinValue().GetIndex();
                        uint32_t maxValue = shaderOptionDescriptor.GetMaxValue().GetIndex();
                        totalVariantSize = totalVariantSize * (maxValue - minValue + 1);
                    }
                    m_shaderVariantListSourceData.m_shaderVariants.reserve(totalVariantSize);
                    m_shaderVariantListSourceData.m_shaderVariants.emplace_back(stableId, systemOptionSetting);
                    stableId++;

                    // Expand unset option
                    for (auto& shaderOptionDescriptor : unsetOption)
                    {
                        AZStd::vector<AZ::RPI::ShaderVariantListSourceData::VariantInfo> shaderVariants;
                        AZStd::vector<AZ::RPI::ShaderVariantListSourceData::VariantInfo> expandShaderVariants;

                        uint32_t minValue = shaderOptionDescriptor.GetMinValue().GetIndex();
                        uint32_t maxValue = shaderOptionDescriptor.GetMaxValue().GetIndex();
                        size_t listSize = m_shaderVariantListSourceData.m_shaderVariants.size();
                        size_t expandSize = listSize * (maxValue - minValue);
                        shaderVariants.reserve(listSize);
                        expandShaderVariants.reserve(expandSize);

                        for (uint32_t index = minValue; index <= maxValue; ++index)
                        {
                            AZ::Name optionValue = shaderOptionDescriptor.GetValueName(index);
                            if (optionValue != shaderOptionDescriptor.GetDefaultValue())
                            {
                                stableId = UpdateOptionValue(
                                    m_shaderVariantListSourceData.m_shaderVariants,
                                    shaderVariants,
                                    shaderOptionDescriptor.GetName(),
                                    optionValue,
                                    stableId);

                                expandShaderVariants.insert(
                                    expandShaderVariants.end(),
                                    AZStd::make_move_iterator(shaderVariants.begin()),
                                    AZStd::make_move_iterator(shaderVariants.end()));
                            }
                        }

                        m_shaderVariantListSourceData.m_shaderVariants.insert(
                            m_shaderVariantListSourceData.m_shaderVariants.end(),
                            AZStd::make_move_iterator(expandShaderVariants.begin()),
                            AZStd::make_move_iterator(expandShaderVariants.end()));
                    }
                }
            }
            

            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
            m_modified = true;
        }
        else
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Could not load shader asset: %s.", shaderPath.c_str());
        }
    }

    const AZ::RPI::ShaderVariantListSourceData& ShaderManagementConsoleDocument::GetShaderVariantListSourceData() const
    {
        return m_shaderVariantListSourceData;
    }

    size_t ShaderManagementConsoleDocument::GetShaderOptionDescriptorCount() const
    {
        if (m_shaderAsset.IsReady())
        {
            const auto& layout = m_shaderAsset->GetShaderOptionGroupLayout();
            const auto& shaderOptionDescriptors = layout->GetShaderOptions();
            return shaderOptionDescriptors.size();
        }
        return 0;
    }

    const AZ::RPI::ShaderOptionDescriptor& ShaderManagementConsoleDocument::GetShaderOptionDescriptor(size_t index) const
    {
        if (m_shaderAsset.IsReady())
        {
            const auto& layout = m_shaderAsset->GetShaderOptionGroupLayout();
            const auto& shaderOptionDescriptors = layout->GetShaderOptions();
            return shaderOptionDescriptors.at(index);
        }
        return m_invalidDescriptor;
    }

    AtomToolsFramework::DocumentTypeInfo ShaderManagementConsoleDocument::BuildDocumentTypeInfo()
    {
        AtomToolsFramework::DocumentTypeInfo documentType;
        documentType.m_documentTypeName = "Shader Variant List";
        documentType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo) {
            return aznew ShaderManagementConsoleDocument(toolId, documentTypeInfo); };
        documentType.m_supportedExtensionsToOpen.push_back({ "Shader Variant List", AZ::RPI::ShaderVariantListSourceData::Extension });
        documentType.m_supportedExtensionsToSave.push_back({ "Shader Variant List", AZ::RPI::ShaderVariantListSourceData::Extension });
        return documentType;
    }

    AtomToolsFramework::DocumentObjectInfoVector ShaderManagementConsoleDocument::GetObjectInfo() const
    {
        AtomToolsFramework::DocumentObjectInfoVector objects = AtomToolsDocument::GetObjectInfo();

        AtomToolsFramework::DocumentObjectInfo objectInfo;
        objectInfo.m_visible = true;
        objectInfo.m_name = "Shader Variant List";
        objectInfo.m_displayName = "Shader Variant List";
        objectInfo.m_description = "Shader Variant List";
        objectInfo.m_objectType = azrtti_typeid<AZ::RPI::ShaderVariantListSourceData>();
        objectInfo.m_objectPtr = const_cast<AZ::RPI::ShaderVariantListSourceData*>(&m_shaderVariantListSourceData);
        objects.push_back(AZStd::move(objectInfo));

        return objects;
    }

    bool ShaderManagementConsoleDocument::Open(const AZStd::string& loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderSourceData::Extension))
        {
            return LoadShaderSourceData();
        }

        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
        {
            return LoadShaderVariantListSourceData();
        }

        AZ_Error("ShaderManagementConsoleDocument", false, "Document extension is not supported: '%s.'", m_absolutePath.c_str());
        return OpenFailed();
    }

    bool ShaderManagementConsoleDocument::Save()
    {
        if (!AtomToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveSourceData();
    }

    bool ShaderManagementConsoleDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveSourceData();
    }

    bool ShaderManagementConsoleDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveSourceData();
    }

    bool ShaderManagementConsoleDocument::IsModified() const
    {
        return m_modified;
    }

    bool ShaderManagementConsoleDocument::BeginEdit()
    {
        // Save the current properties as a momento for undo before any changes are applied
        m_shaderVariantListSourceDataBeforeEdit = m_shaderVariantListSourceData;
        return true;
    }

    bool ShaderManagementConsoleDocument::EndEdit()
    {
        bool modified = false;

        // Lazy evaluation, comparing the current and previous shader variant list source data state to determine if we need to record undo/redo history.
        // TODO Refine this so that only the deltas are stored.
        const auto& undoState = m_shaderVariantListSourceDataBeforeEdit;
        const auto& redoState = m_shaderVariantListSourceData;
        if (undoState.m_shaderFilePath != redoState.m_shaderFilePath ||
            undoState.m_shaderVariants.size() != redoState.m_shaderVariants.size())
        {
            modified = true;
        }
        else
        {
            for (size_t i = 0; i < redoState.m_shaderVariants.size(); ++i)
            {
                if (undoState.m_shaderVariants[i].m_stableId != redoState.m_shaderVariants[i].m_stableId ||
                    undoState.m_shaderVariants[i].m_options != redoState.m_shaderVariants[i].m_options)
                {
                    modified = true;
                    break;
                }
            }
        }

        if (modified)
        {
            AddUndoRedoHistory(
                [this, undoState]() { SetShaderVariantListSourceData(undoState); },
                [this, redoState]() { SetShaderVariantListSourceData(redoState); });
        }

        m_shaderVariantListSourceDataBeforeEdit = {};
        return true;
    }

    void ShaderManagementConsoleDocument::Clear()
    {
        AtomToolsFramework::AtomToolsDocument::Clear();

        m_shaderVariantListSourceData = {};
        m_shaderVariantListSourceDataBeforeEdit = {};
        m_shaderAsset = {};
        m_modified = {};
    }

    bool ShaderManagementConsoleDocument::SaveSourceData()
    {
        if (!AZ::RPI::JsonUtils::SaveObjectToFile(m_savePathNormalized, m_shaderVariantListSourceData))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document could not be saved: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        m_absolutePath = m_savePathNormalized;
        m_modified = {};
        return SaveSucceeded();
    }

    bool ShaderManagementConsoleDocument::LoadShaderSourceData()
    {
        return OpenFailed();
    }

    bool ShaderManagementConsoleDocument::LoadShaderVariantListSourceData()
    {
        // Load previously generated shader variant list source data
        AZ::RPI::ShaderVariantListSourceData shaderVariantListSourceData;
        if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_absolutePath, shaderVariantListSourceData))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Failed loading shader variant list data: '%s.'", m_absolutePath.c_str());
            return OpenFailed();
        }

        SetShaderVariantListSourceData(shaderVariantListSourceData);
        m_modified = {};

        return OpenSucceeded();
    }

    AZ::u32 ShaderManagementConsoleDocument::UpdateOptionValue(
        AZStd::vector<AZ::RPI::ShaderVariantListSourceData::VariantInfo>& shaderVariantIN,
        AZStd::vector<AZ::RPI::ShaderVariantListSourceData::VariantInfo>& shaderVariantOUT,
        AZ::Name targetOption,
        AZ::Name targetValue,
        AZ::u32 stableId)
    {
        shaderVariantOUT.clear();

        for (auto& variantInfo : shaderVariantIN)
        {
            AZ::RPI::ShaderOptionValuesSourceData optionList;
            optionList = variantInfo.m_options;
            if (optionList.count(targetOption) > 0)
            {
                optionList[targetOption] = targetValue;
            }
            shaderVariantOUT.emplace_back(stableId, optionList);
            stableId += 1;
        }
        return stableId;
    }
} // namespace ShaderManagementConsole
