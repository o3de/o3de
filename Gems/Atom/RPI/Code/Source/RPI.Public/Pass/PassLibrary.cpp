/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <Atom/RHI/RHIUtils.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassLibrary.h>
#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace RPI
    {
        // Initialization & Shutdown...

        void PassLibrary::Init()
        {
            AddCoreTemplates();
        }

        void PassLibrary::Shutdown()
        {
            m_isShuttingDown = true;
            m_passNameMapping.clear();
            m_templateEntries.clear();
            m_templateMappingAssets.clear();
            Data::AssetBus::MultiHandler::BusDisconnect();
        }


        // Getters...

        PassLibrary::TemplateEntry* PassLibrary::GetEntry(const Name& templateName)
        {
            auto itr = m_templateEntries.find(templateName);
            if (itr != m_templateEntries.end())
            {
                return &(itr->second);
            }
            return nullptr;
        }

        const PassLibrary::TemplateEntry* PassLibrary::GetEntry(const Name& templateName) const
        {
            auto itr = m_templateEntries.find(templateName);
            if (itr != m_templateEntries.end())
            {
                return &(itr->second);
            }
            return nullptr;
        }

        const AZStd::shared_ptr<PassTemplate> PassLibrary::GetPassTemplate(const Name& templateName) const
        {
            const TemplateEntry* entry = GetEntry(templateName);
            return entry ? entry->m_template : nullptr;
        }

        const AZStd::vector<Pass*>& PassLibrary::GetPassesForTemplate(const Name& templateName) const
        {
            static AZStd::vector<Pass*> emptyPassList;
            const TemplateEntry* entry = GetEntry(templateName);
            return entry ? entry->m_passes : emptyPassList;
        }

        bool PassLibrary::HasTemplate(const Name& templateName) const
        {
            return m_templateEntries.find(templateName) != m_templateEntries.end();
        }

        bool PassLibrary::HasPassesForTemplate(const Name& templateName) const
        {
            return (GetPassesForTemplate(templateName).size() > 0);
        }

        void PassLibrary::ForEachPass(const PassFilter& passFilter, AZStd::function<PassFilterExecutionFlow(Pass*)> passFunction)
        {
            uint32_t filterOptions = passFilter.GetEnabledFilterOptions();

            // A lambda function which visits each pass in a pass list, if the pass matches the pass filter, then call the pass function
            auto visitList = [passFilter, passFunction](const AZStd::vector<Pass*>& passList, uint32_t options) -> PassFilterExecutionFlow
            {
                if (passList.size() == 0)
                {
                    return PassFilterExecutionFlow::ContinueVisitingPasses;
                }
                // if there is not other filter options enabled, skip the filter and call pass functions directly
                if (options == PassFilter::FilterOptions::Empty)
                {
                    for (Pass* pass : passList)
                    {
                        // If user want to skip processing, return directly.
                        if (passFunction(pass) == PassFilterExecutionFlow::StopVisitingPasses)
                        {
                            return PassFilterExecutionFlow::StopVisitingPasses;
                        }
                    }
                    return PassFilterExecutionFlow::ContinueVisitingPasses;
                }

                // Check with the pass filter and call pass functions
                for (Pass* pass : passList)
                {
                    if (passFilter.Matches(pass, options))
                    {
                        if (passFunction(pass) == PassFilterExecutionFlow::StopVisitingPasses)
                        {
                            return PassFilterExecutionFlow::StopVisitingPasses;
                        }
                    }
                }
                 return PassFilterExecutionFlow::ContinueVisitingPasses;
            };

            // Check pass template name first
            if (filterOptions & PassFilter::FilterOptions::PassTemplateName)
            {
                auto entry = GetEntry(passFilter.GetPassTemplateName());
                if (!entry)
                {
                    return;
                }

                filterOptions &= ~(PassFilter::FilterOptions::PassTemplateName);
                visitList(entry->m_passes, filterOptions);
                return;
            }
            else if (filterOptions & PassFilter::FilterOptions::PassName)
            {
                const auto constItr = m_passNameMapping.find(passFilter.GetPassName());
                if (constItr == m_passNameMapping.end())
                {
                    return;
                }

                filterOptions &= ~(PassFilter::FilterOptions::PassName);
                visitList(constItr->second, filterOptions);
                return;
            }

            // check againest every passes. This might be slow 
            AZ_PROFILE_SCOPE(RPI, "PassLibrary::ForEachPass");
            for (auto& namePasses : m_passNameMapping)
            {
                if (visitList(namePasses.second, filterOptions) == PassFilterExecutionFlow::StopVisitingPasses)
                {
                    return;
                }
            }
        }

        // Add Functions...

        void PassLibrary::AddPass(Pass* pass)
        {
            if (pass->m_template)
            {
                TemplateEntry* entry = GetEntry(pass->m_template->m_name);
                if (entry)
                {
                    entry->m_passes.push_back(pass);
                }
            }

            m_passNameMapping[pass->m_name].push_back(pass);
        }

        void PassLibrary::AddCoreTemplates()
        {
            // Put calls to pass template creation functions here...
            AddCopyPassTemplate();
        }

        void PassLibrary::AddCopyPassTemplate()
        {
            AZStd::shared_ptr<PassTemplate> passTemplate = AZStd::make_shared<PassTemplate>();
            passTemplate->m_passClass = "CopyPass";
            passTemplate->m_name = "CopyPassTemplate";

            PassSlot inputSlot;
            inputSlot.m_name = "Input";
            inputSlot.m_slotType = PassSlotType::Input;
            inputSlot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Copy;
            inputSlot.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
            passTemplate->m_slots.emplace_back(inputSlot);

            PassSlot outputSlot;
            outputSlot.m_name = "Output";
            outputSlot.m_slotType = PassSlotType::Output;
            outputSlot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Copy;
            outputSlot.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;
            passTemplate->m_slots.emplace_back(outputSlot);

            AddPassTemplate(passTemplate->m_name, std::move(passTemplate));
        }

        bool PassLibrary::AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate, bool hotReloading)
        {
            // Check if template already exists (unless we're hot reloading)
            if (!hotReloading && GetPassTemplate(name) != nullptr)
            {
                AZ_Warning("PassLibrary", false,
                    "Trying to add a PassTemplate that already exists in PassLibrary. Template name: %s", name.GetCStr());
                return false;
            }

            if (!passTemplate)
            {
                AZ_Warning("PassLibrary", false,
                    "Trying to add a null PassTemplate. Template name: %s", name.GetCStr());
                return false;
            }

            if (passTemplate->m_name != name)
            {
                AZ_Warning("PassLibrary", false,
                    "Pass template alias [%s] is different than its name [%s]", name.GetCStr(), passTemplate->m_name.GetCStr());
                passTemplate->m_name = name;
            }

            ValidateDeviceFormats(passTemplate);

            m_templateEntries[name].m_template = std::move(passTemplate);
            return true;
        }

        void PassLibrary::RemovePassFromLibrary(Pass* pass)
        {
            if (m_isShuttingDown)
            {
                return;
            }

            // Remove from associated template
            if (pass->m_template)
            {
                TemplateEntry* entry = GetEntry(pass->m_template->m_name);
                if (entry)
                {
                    [[maybe_unused]] auto iter = AZStd::remove(entry->m_passes.begin(), entry->m_passes.end(), pass);

                    AZ_Assert((iter + 1) == entry->m_passes.end(),
                        "Pass [%s] is being deleted but was not registered with it's PassTemlate [%s] in the PassLibrary.",
                        pass->m_name.GetCStr(), pass->m_template->m_name.GetCStr());

                    // Delete the pass that is now at the end of the list
                    entry->m_passes.pop_back();
                }
            }

            // Remove pass from pass name
            AZ_Assert(m_passNameMapping.find(pass->GetName()) != m_passNameMapping.end(),
                "Pass [%s] is trying to be removed from PassLibrary but was not found in library",
                pass->GetName().GetCStr());

            AZStd::vector<Pass*>& passes = m_passNameMapping[pass->GetName()];
            for (auto itr = passes.begin(); itr != passes.end(); itr++)
            {
                if (*itr == pass)
                {
                    passes.erase(itr);
                    return;
                }
            }
        }

        // Pass Asset Functions...

        void PassLibrary::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            // Handle pass asset reload
            Data::Asset<PassAsset> passAsset = { asset.GetAs<PassAsset>(), AZ::Data::AssetLoadBehavior::PreLoad };
            if (passAsset && passAsset->GetPassTemplate())
            {
                LoadPassAsset(passAsset->GetPassTemplate()->m_name, passAsset, true);
                return;
            }

            // Handle template mapping reload
            // Note: it's a known issue that when mapping asset got reloaded, we only handle the new entries
            Data::Asset<AnyAsset> templateMappings = { asset.GetAs<AnyAsset>(), AZ::Data::AssetLoadBehavior::PreLoad };
            if (templateMappings)
            {
                auto itr = m_templateMappingAssets.find(asset->GetId());
                if (itr != m_templateMappingAssets.end())
                {
                    LoadPassTemplateMappings(templateMappings);
                }
            }
        }

        bool PassLibrary::LoadPassAsset(const Name& name, const Data::Asset<PassAsset>& passAsset, bool hotReloading)
        {
            if (!passAsset.IsReady())
            {
                AZ_Error("PassAsset", false, "Failed to get pass asset. %s", passAsset.ToString<AZStd::string>().c_str());
                return false;
            }

            if (!passAsset->GetPassTemplate())
            {
                AZ_Error("PassAsset", false, "Pass asset does not contain a pass template. %s", passAsset.ToString<AZStd::string>().c_str());
                return false;
            }

            AZStd::shared_ptr<PassTemplate> passTemplate = passAsset->GetPassTemplate()->Clone();

            bool success = AddPassTemplate(name, std::move(passTemplate), hotReloading);
            if (success)
            {
                TemplateEntry& entry = m_templateEntries[name];
                entry.m_asset = passAsset;

                if (hotReloading)
                {
                    for (Pass* pass : entry.m_passes)
                    {
                        if (pass->m_pipeline)
                        {
                            pass->m_pipeline->SetPassNeedsRecreate();
                        }
                    }
                }
            }

            return success;
        }

        bool PassLibrary::LoadPassAsset(const Name& name, const Data::AssetId& passAssetId)
        {
            Data::Asset<PassAsset> passAsset;
            if (passAssetId.IsValid())
            {
                passAsset = Data::AssetManager::Instance().GetAsset<RPI::PassAsset>(passAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                passAsset.BlockUntilLoadComplete();
            }

            bool loadSuccess = LoadPassAsset(name, passAsset);

            if (loadSuccess)
            {
                Data::AssetBus::MultiHandler::BusConnect(passAssetId);
            }

            return loadSuccess;
        }

        bool PassLibrary::LoadPassTemplateMappings(const AZStd::string& templateMappingPath)
        {
            Data::Asset<AnyAsset> mappingAsset = AssetUtils::LoadCriticalAsset<AnyAsset>(templateMappingPath.c_str(), AssetUtils::TraceLevel::Error);

            if (m_templateMappingAssets.find(mappingAsset.GetId()) != m_templateMappingAssets.end())
            {
                AZ_Warning("PassLibrary", false, "Pass template mapping [%s] was already loaded", mappingAsset.GetHint().c_str());
                return true;
            }

            bool success = LoadPassTemplateMappings(mappingAsset);
            if (success)
            {
                Data::AssetBus::MultiHandler::BusConnect(mappingAsset->GetId());
            }
            return success;
        }

        bool PassLibrary::LoadPassTemplateMappings(Data::Asset<AnyAsset> mappingAsset)
        {
            if (mappingAsset.IsReady())
            {
                const AssetAliases* mappings = GetDataFromAnyAsset<AssetAliases>(mappingAsset);
                if (mappings == nullptr)
                {
                    AZ_Error("PassLibrary", false, "Asset [%s] doesn't have assetAliases data", mappingAsset.GetHint().c_str());
                    return false;
                }

                const AZStd::unordered_map<AZStd::string, Data::AssetId>& assetMapping = mappings->GetAssetMapping();
                Data::AssetId mappingAssetId = mappingAsset.GetId();
                m_templateEntries.reserve(m_templateEntries.size() + assetMapping.size());
                for (const auto& assetInfo : assetMapping)
                {
                    Name templateName = AZ::Name(assetInfo.first);
                    if (!HasTemplate(templateName))
                    {
                        bool loaded = LoadPassAsset(templateName, assetInfo.second);
                        if (loaded)
                        {
                            auto& entry = m_templateEntries[templateName];
                            entry.m_mappingAssetId = mappingAssetId;
                        }
                    }
                    else
                    {
                        // Report a warning if the template was setup in another mappping asset.
                        // We won't report a warning if the template was loaded from same asset. This only happens when the asset got reloaded.
                        if (m_templateEntries[templateName].m_mappingAssetId != mappingAssetId)
                        {
                            AZ_Warning("PassLibrary", false, "Template [%s] was aleady added to the library. Duplicated template from [%s]",
                                templateName.GetCStr(), mappingAsset.ToString<AZStd::string>().c_str());
                        }
                    }
                }

                m_templateMappingAssets[mappingAsset->GetId()] = mappingAsset;
                return true;
            }
            return false;
        }

        void PassLibrary::ValidateDeviceFormats(const AZStd::shared_ptr<PassTemplate>& passTemplate)
        {
            // Validate image attachments
            for (PassImageAttachmentDesc& imageAttachment : passTemplate->m_imageAttachments)
            {
                RHI::Format format = imageAttachment.m_imageDescriptor.m_format;
                AZStd::string formatLocation = AZStd::string::format("PassAttachmentDesc [%s] on PassTemplate [%s]", imageAttachment.m_name.GetCStr(), passTemplate->m_name.GetCStr());
                imageAttachment.m_imageDescriptor.m_format = RHI::ValidateFormat(format, formatLocation.c_str(), imageAttachment.m_formatFallbacks);
            }

            // Validate slot views
            for (PassSlot& slot : passTemplate->m_slots)
            {
                if (slot.m_imageViewDesc)
                {
                    RHI::Format format = slot.m_imageViewDesc->m_overrideFormat;
                    AZStd::string formatLocation = AZStd::string::format("ImageViewDescriptor on Slot [%s] in PassTemplate [%s]", slot.m_name.GetCStr(), passTemplate->m_name.GetCStr());
                    RHI::FormatCapabilities capabilities = RHI::GetCapabilities(slot.m_scopeAttachmentUsage, slot.GetAttachmentAccess(), RHI::AttachmentType::Image);
                    slot.m_imageViewDesc->m_overrideFormat = RHI::ValidateFormat(format, formatLocation.c_str(), slot.m_formatFallbacks, capabilities);
                }
                if (slot.m_bufferViewDesc)
                {
                    RHI::Format format = slot.m_bufferViewDesc->m_elementFormat;
                    AZStd::string formatLocation = AZStd::string::format("BufferViewDescriptor on Slot [%s] in PassTemplate [%s]", slot.m_name.GetCStr(), passTemplate->m_name.GetCStr());
                    RHI::FormatCapabilities capabilities = RHI::GetCapabilities(slot.m_scopeAttachmentUsage, slot.GetAttachmentAccess(), RHI::AttachmentType::Buffer);
                    slot.m_bufferViewDesc->m_elementFormat = RHI::ValidateFormat(format, formatLocation.c_str(), slot.m_formatFallbacks, capabilities);
                }
            }

        }

    }   // namespace RPI
}   // namespace AZ

