/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/MultisampleState.h>

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Pass/Specific/ImageAttachmentPreviewPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/Utils/DdsFile.h>


#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>

#ifndef SCRIPTABLE_IMGUI
#define Scriptable_ImGui ImGui
#endif

namespace AZ::Render
{
    inline AZ::RPI::PassAttachment* FindPassAttachment(AZ::RPI::Pass* pass, AZ::RHI::AttachmentId attachmentId)
    {
        for (auto& binding : pass->GetAttachmentBindings())
        {
            if (binding.GetAttachment() && binding.GetAttachment()->GetAttachmentId() == attachmentId)
            {
                return binding.GetAttachment().get();
            }
        }
        return nullptr;
    }

    inline void ImGuiPassTree::Draw(bool& draw, AZ::RPI::Pass* rootPass)
    {
        using namespace AZ;

        // always set m_selectedPass to empty and use m_selectedPassPath to find it when render the pass tree
        m_selectedPass = nullptr;
        bool needSaveAttachment = false;

        ImGui::SetNextWindowSize(ImVec2(200.f, 200.f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("PassTree View", &draw, ImGuiWindowFlags_None))
        {
            // Draw the header
            // some options for pass attachments
            if (Scriptable_ImGui::Checkbox("Preview Attachment", &m_shouldPreviewAttachment))
            {
                m_selectedChanged = true;
                if (!m_previewPass)
                {
                    RPI::PassDescriptor descriptor(Name("ImageAttachmentsPreviewPass"));
                    m_previewPass = RPI::ImageAttachmentPreviewPass::Create(descriptor);
                }

                if (!m_shouldPreviewAttachment)
                {
                    m_previewPass->ClearPreviewAttachment();
                    if (m_previewPass->GetParent())
                    {
                        m_previewPass->QueueForRemoval();
                    }
                }
            }

            if (Scriptable_ImGui::Checkbox("Show Pass Attachments", &m_showAttachments))
            {
                if (!m_showAttachments)
                {
                    m_selectedChanged = true;
                    m_attachmentId = AZ::RHI::AttachmentId{};
                    m_slotName = AZ::Name{};
                }
            }

            Scriptable_ImGui::Checkbox("Expand All Passes", &m_expandAllPasses);

            if (m_showAttachments)
            {
                Scriptable_ImGui::SliderFloat2("Color Range", m_attachmentColorTranformRange, 0.0f, 1.0f);
            }

            if (Scriptable_ImGui::Button("Save Attachment"))
            {
                needSaveAttachment = true;
            }

            ImGui::TextWrapped("%s", m_attachmentReadbackInfo.c_str());
        }

        ImGui::End();

        // Draw the hierarchical view
        // It will assign m_seletedPass if there is a pass matches m_seletedPassPath
        ImGui::SetNextWindowPos(ImVec2(300, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("PassTree", nullptr, ImGuiWindowFlags_None))
        {
            m_passFilter.Draw("Pass Name Filter");
            AZStd::unordered_set<Name> filteredPassNames;
            if (GetFilteredPassNames(rootPass, filteredPassNames))
            {
                if (ImGui::BeginChild("Passes"))
                {
                    DrawTreeView(rootPass, filteredPassNames);
                }
                ImGui::EndChild();
            }
            else
            {
                ImGui::Text("No matching pass name found");
            }
        }
        ImGui::End();

        // It's possible that the pass pointer changed but selected pass path wasn't changed
        if (m_selectedPass != m_lastSelectedPass)
        {
            m_selectedChanged = true;
            if (m_selectedPass == nullptr)
            {
                m_selectedPassPath = AZ::Name{};
            }
        }
        m_lastSelectedPass = m_selectedPass;

        if (m_shouldPreviewAttachment && m_selectedChanged)
        {
            m_selectedChanged = false;
            if (!m_attachmentId.IsEmpty() && m_selectedPass)
            {
                if (!m_previewPass->GetParent())
                {
                    RPI::PassSystemInterface::Get()->AddPassWithoutPipeline(m_previewPass);
                }
                AZ::RPI::PassAttachment* attachment = FindPassAttachment(m_selectedPass, m_attachmentId);
                if (attachment)
                {
                    // Reset output attachment to empty so the preview will use pass's owner render pipeline's output
                    m_previewPass->SetOutputColorAttachment(nullptr);
                    m_previewPass->PreviewImageAttachmentForPass(m_selectedPass, attachment);
                }
            }
            else
            {
                m_previewPass->ClearPreviewAttachment();
                if (m_previewPass->GetParent())
                {
                    m_previewPass->QueueForRemoval();
                }
            }
        }

        if (m_shouldPreviewAttachment)
        {
            m_previewPass->SetColorTransformRange(m_attachmentColorTranformRange);
        }

        if (needSaveAttachment)
        {            
            m_attachmentReadbackInfo = "";
            if (!m_readback)
            {
                m_readback = AZStd::make_shared<AZ::RPI::AttachmentReadback>(AZ::RHI::ScopeId{ "AttachmentReadback" });
                m_readback->SetCallback(AZStd::bind(&ImGuiPassTree::ReadbackCallback, this, AZStd::placeholders::_1));
            }

            if (m_selectedPass && !m_slotName.IsEmpty())
            {
                bool readbackResult = m_selectedPass->ReadbackAttachment(m_readback, 0, m_slotName);
                if (!readbackResult)
                {
                    AZ_Error("ImGuiPassTree", false, "Failed to readback attachment from pass [%s] slot [%s]", m_selectedPass->GetName().GetCStr(), m_slotName.GetCStr());
                }
            }
        }
    }
        
    inline void ImGuiPassTree::DrawPassAttachments(AZ::RPI::Pass* pass)
    {
        for (const auto& binding : pass->GetAttachmentBindings())
        {
            // Binding info: [slot type] [slot name]
            AZStd::string label = AZStd::string::format("[%s] [%s]", AZ::RPI::ToString(binding.m_slotType),
                binding.m_name.GetCStr());

            // Append attachment info if the attachment exists
            if (binding.GetAttachment())
            {
                AZ::RHI::AttachmentType type = binding.GetAttachment()->GetAttachmentType();

                // Append attachment info: [attachment type] attachment name
                label += AZStd::string::format(" [%s] %s",
                    AZ::RHI::ToString(type),
                    binding.GetAttachment()->m_name.GetCStr());

                if (type == AZ::RHI::AttachmentType::Image)
                {
                    // Append image info: [format] [size] [msaa]
                    AZ::RHI::ImageDescriptor descriptor;
                    if (binding.GetAttachment()->m_importedResource)
                    {
                        AZ::RPI::Image* image = static_cast<AZ::RPI::Image*>(binding.GetAttachment()->m_importedResource.get());
                        descriptor = image->GetRHIImage()->GetDescriptor();
                    }
                    else
                    {
                        descriptor = binding.GetAttachment()->m_descriptor.m_image;
                    }
                    auto format = descriptor.m_format;
                    auto size = descriptor.m_size;
                    label += AZStd::string::format(" [%s] [%dx%d]", AZ::RHI::ToString(format), size.m_width, size.m_height);

                    if (descriptor.m_multisampleState.m_samples > 1)
                    {
                        if (descriptor.m_multisampleState.m_customPositionsCount > 0)
                        {
                            label += AZStd::string::format(" [MSAA_Custom_%dx]", descriptor.m_multisampleState.m_samples);
                        }
                        else
                        {
                            label += AZStd::string::format(" [MSAA_%dx]", descriptor.m_multisampleState.m_samples);
                        }
                    }
                }
                else if (type == AZ::RHI::AttachmentType::Buffer)
                {
                    // Append buffer info: [size]
                    auto size = binding.GetAttachment()->m_descriptor.m_buffer.m_byteCount;
                    label += AZStd::string::format(" [%llu]", size);
                }

                if (Scriptable_ImGui::Selectable(label.c_str(), m_attachmentId == binding.GetAttachment()->GetAttachmentId()))
                {
                    m_selectedPassPath = pass->GetPathName();
                    m_selectedPass = pass;
                    m_attachmentId = binding.GetAttachment()->GetAttachmentId();
                    m_slotName = binding.m_name;
                    m_selectedChanged = true;
                }
            }
            else
            {
                // Only draw text (not selectable) if there is no attachment binded to the slot.
                ImGui::Text("%s", label.c_str());
            }
        }

    }

    inline void ImGuiPassTree::DrawTreeView(AZ::RPI::Pass* pass, const AZStd::unordered_set<Name>& filteredPassNames)
    {
        if (!filteredPassNames.contains(pass->GetPathName()))
        {
            return;
        }

        AZ::RPI::ParentPass* asParent = pass->AsParent();

        bool enabled = pass->IsEnabled();
        if (!enabled)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        }
        if (!asParent)
        {
            // For leaf passes
            if (!m_showAttachments)
            {
                // Only draw the leaf pass as selectable if we are not showing attachments as its children
                if (Scriptable_ImGui::Selectable(pass->GetName().GetCStr(), m_selectedPassPath == pass->GetPathName()))
                {
                    m_selectedPassPath = pass->GetPathName();
                    m_attachmentId = AZ::RHI::AttachmentId{};
                    m_slotName = AZ::Name{};
                    m_selectedChanged = true;
                }
            }
            else
            {
                // Draw the pass as a tree node which has attachments as its children
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick 
                    | (m_expandAllPasses ? ImGuiTreeNodeFlags_DefaultOpen : 0)
                    | ((m_selectedPassPath == pass->GetPathName()) ? ImGuiTreeNodeFlags_Selected : 0);

                bool nodeOpen = Scriptable_ImGui::TreeNodeEx(pass->GetName().GetCStr(), flags);


                AZ::RPI::RasterPass* asRasterPass = azrtti_cast<AZ::RPI::RasterPass*>(pass);
                if (asRasterPass)
                {
                    ImGui::Text("Raster pass with %d draw items", asRasterPass->GetDrawItemCount());
                }

                AZ::RPI::RenderPass* asRenderPass = azrtti_cast<AZ::RPI::RenderPass*>(pass);
                if (AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() > 1 && asRenderPass && asRenderPass->IsEnabled())
                {
                    ImGui::Text("Pass runs on device %d", AZStd::max(asRenderPass->ScopeProducer::GetDeviceIndex(), 0));
                }

                if (ImGui::IsItemClicked())
                {
                    m_selectedPassPath = pass->GetPathName();
                    m_attachmentId = AZ::RHI::AttachmentId{};
                    m_slotName = AZ::Name{};
                    m_selectedChanged = true;
                }

                if (nodeOpen)
                {
                    DrawPassAttachments(pass);
                    Scriptable_ImGui::TreePop();
                }
            }
        }
        else
        {
            // For a ParentPasse, draw it as a tree node 
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
                | (m_expandAllPasses ? ImGuiTreeNodeFlags_DefaultOpen : 0)
                | ((m_selectedPassPath == pass->GetPathName()) ? ImGuiTreeNodeFlags_Selected : 0);

            bool nodeOpen = ImGui::TreeNodeEx(pass->GetName().GetCStr(), flags);

            if (ImGui::IsItemClicked())
            {
                m_selectedPassPath = pass->GetPathName();
                m_attachmentId = AZ::RHI::AttachmentId{};
                m_slotName = AZ::Name{};
                m_selectedChanged = true;
            }

            if (nodeOpen)
            {
                if (m_showAttachments)
                {
                    DrawPassAttachments(pass);
                }
                for (const auto& child : asParent->GetChildren())
                {
                    DrawTreeView(child.get(), filteredPassNames);
                }

                ImGui::TreePop();
            }
        }

        if (!enabled)
        {
            ImGui::PopStyleColor();
        }

        // set m_selectedPass if pass path matches
        if (pass->GetPathName() == m_selectedPassPath)
        {
            m_selectedPass = pass;
        }
    }

    inline bool ImGuiPassTree::GetFilteredPassNames(AZ::RPI::Pass* pass, AZStd::unordered_set<Name>& filteredPassNames) const
    {
        bool anyChildMatch = m_passFilter.PassFilter(pass->GetName().GetCStr());

        if (RPI::ParentPass* asParent = pass->AsParent())
        {
            for (const auto& child : asParent->GetChildren())
            {
                anyChildMatch |= GetFilteredPassNames(child.get(), filteredPassNames);
            }
        }

        if (anyChildMatch)
        {
            filteredPassNames.insert(pass->GetPathName());
        }

        return anyChildMatch;
    }

    inline void ImGuiPassTree::ReadbackCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
    {
        if (readbackResult.m_state == AZ::RPI::AttachmentReadback::ReadbackState::Failed)
        {
            m_attachmentReadbackInfo = AZStd::string::format("Failed to readback attachment [%s]", readbackResult.m_name.GetCStr());
            return;
        }

        if (m_engineRoot.empty())
        {
            AZ::IO::FixedMaxPathString engineRoot = AZ::Utils::GetEnginePath();
            if (!engineRoot.empty())
            {
                m_engineRoot = AZStd::string_view(engineRoot);
            }
        }

        AZStd::string filePath;
        const char* localFolder = "FrameCapture";
        if (readbackResult.m_attachmentType == AZ::RHI::AttachmentType::Buffer)
        {
            // write buffer data to the data file
            AZStd::string localPath, fileName;
            fileName = AZStd::string::format("%s.buffer", readbackResult.m_name.GetCStr());
            AzFramework::StringFunc::Path::Join(localFolder, fileName.c_str(), localPath, true, false);
            AzFramework::StringFunc::Path::Join(m_engineRoot.c_str(), localPath.c_str(), filePath, true, false);

            AZ::IO::FileIOStream fileStream(filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
            if (!fileStream.IsOpen())
            {
                m_attachmentReadbackInfo = AZStd::string::format("Failed to open file %s for writing", filePath.c_str());
                return;
            }

            fileStream.Write(readbackResult.m_dataBuffer->size(), readbackResult.m_dataBuffer->data());
        }
        else if (readbackResult.m_attachmentType == AZ::RHI::AttachmentType::Image)
        {
            // write the readback result of the image attachment to a dds file
            AZStd::string localPath, fileName;
            fileName = AZStd::string::format("%s.dds", readbackResult.m_name.GetCStr());
            AzFramework::StringFunc::Path::Join(localFolder, fileName.c_str(), localPath, true, false);
            AzFramework::StringFunc::Path::Join(m_engineRoot.c_str(), localPath.c_str(), filePath, true, false);

            auto outcome = AZ::DdsFile::WriteFile(filePath, { readbackResult.m_imageDescriptor.m_size,
                readbackResult.m_imageDescriptor.m_format, readbackResult.m_dataBuffer.get() });
            if (!outcome)
            {
                m_attachmentReadbackInfo = AZStd::string::format("Fail to save attachment: %s", outcome.GetError().m_message.c_str());
                return;
            }
        }
        else
        {
            return;
        }

        m_attachmentReadbackInfo = AZStd::string::format("Attachment was saved to %s", filePath.c_str());
    }

    inline void ImGuiPassTree::Reset()
    {
        m_shouldPreviewAttachment = false;
        m_showAttachments = false;

        m_selectedPassPath = AZ::Name{};
        m_selectedPass = nullptr;
        m_lastSelectedPass = nullptr;
        m_attachmentId = AZ::RHI::AttachmentId{};
         m_slotName = AZ::Name{};
        m_selectedChanged = false;
        m_readback = nullptr;
        m_previewPass = nullptr;
        m_attachmentReadbackInfo = "";
    }
}
