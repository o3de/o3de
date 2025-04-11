/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImGui/ImGuiSystemComponent.h>
#include <ImGui/ImGuiPass.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace AZ
{
    namespace Render
    {
        void ImGuiSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImGuiSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        void ImGuiSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImGuiSystemComponent"));
        }

        void ImGuiSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void ImGuiSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile)
        {
            incompatbile.push_back(AZ_CRC_CE("ImGuiSystemComponent"));
        }

        ImGuiSystemComponent::ImGuiSystemComponent()
        {
        }

        void ImGuiSystemComponent::Activate()
        {
            ImGuiSystemRequestBus::Handler::BusConnect();
        }

        void ImGuiSystemComponent::Deactivate()
        {
            ImGuiSystemRequestBus::Handler::BusDisconnect();
        }

        void ImGuiSystemComponent::SetGlobalSizeScale(float scale)
        {
            if (m_sizeScale != scale)
            {
                m_sizeScale = scale;
                ForAllImGuiPasses(
                    [&]([[maybe_unused]] ImGuiPass* pass)
                    {
                        ImGui::GetStyle().ScaleAllSizes(m_sizeScale);
                    }
                );
            }
        }

        void ImGuiSystemComponent::SetGlobalFontScale(float scale)
        {
            if (m_fontScale != scale)
            {
                m_fontScale = scale;
                ForAllImGuiPasses(
                    [&]([[maybe_unused]] ImGuiPass* pass)
                    {
                        ImGui::GetIO().FontGlobalScale = scale;
                    }
                );
            }
        }

        void ImGuiSystemComponent::HideAllImGuiPasses()
        {
            ForAllImGuiPasses(
                [&](ImGuiPass* pass)
                {
                    pass->SetEnabled(false);
                }
            );
        }

        void ImGuiSystemComponent::ShowAllImGuiPasses()
        {
            ForAllImGuiPasses(
                [&](ImGuiPass* pass)
                {
                    pass->SetEnabled(true);
                }
            );
        }

        void ImGuiSystemComponent::ForAllImGuiPasses(PassFunction func)
        {
            ImGuiContext* contextToRestore = ImGui::GetCurrentContext();
            
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassClass<ImGuiPass>();
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [func](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    ImGuiPass* imguiPass = azrtti_cast<ImGuiPass*>(pass);
                    ImGui::SetCurrentContext(imguiPass->GetContext());
                    func(imguiPass);
                    return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });

            ImGui::SetCurrentContext(contextToRestore);
        }

        void ImGuiSystemComponent::PushDefaultImGuiPass(ImGuiPass* imguiPass)
        {
            ImGuiPass** existingPass = AZStd::find(m_defaultImguiPassStack.begin(), m_defaultImguiPassStack.end(), imguiPass);
            if (existingPass != m_defaultImguiPassStack.end())
            {
                AZ_Assert(false, "This Imgui pass is already registered as a default pass.");
                return;
            }
            m_defaultImguiPassStack.push_back(imguiPass);
        }

        void ImGuiSystemComponent::RemoveDefaultImGuiPass(ImGuiPass* imguiPass)
        {
            for (size_t i = 0; i < m_defaultImguiPassStack.size(); ++i)
            {
                if (imguiPass == m_defaultImguiPassStack.at(i))
                {
                    m_defaultImguiPassStack.erase(m_defaultImguiPassStack.begin() + i);

                    // If the pass being removed as default is at the top of the active stack, replace it
                    // with whatever's now on the top of the default pass stack.
                    if (GetActiveContext() == imguiPass->GetContext())
                    {
                        PushActiveContextFromDefaultPass();
                    }
                    break;
                }
            }

            // The ImGuiPass will delete its context so we need to remove it from the active list
            for (size_t i = 0; i < m_activeContextStack.size(); ++i)
            {
                if (imguiPass->GetContext() == m_activeContextStack.at(i))
                {
                    m_activeContextStack.erase(m_activeContextStack.begin() + i);
                    break;
                }
            }
        }

        ImGuiPass* ImGuiSystemComponent::GetDefaultImGuiPass()
        {
            return m_defaultImguiPassStack.size() > 0 ? m_defaultImguiPassStack.back() : nullptr;
        }

        bool ImGuiSystemComponent::PushActiveContextFromDefaultPass()
        {
            if (m_defaultImguiPassStack.size() > 0)
            {
                ImGuiContext* context = m_defaultImguiPassStack.back()->GetContext();
                m_activeContextStack.push_back(context);
                ImGuiSystemNotificationBus::Broadcast(&ImGuiSystemNotifications::ActiveImGuiContextChanged, context);
                return true;
            }
            return false;
        }

        bool ImGuiSystemComponent::PushActiveContextFromPass(const AZStd::vector<AZStd::string>& passHierarchyFilter)
        {
            if (passHierarchyFilter.size() == 0)
            {
                AZ_Warning("ImGuiSystemComponent", false, "passHierarchyFilter is empty");
                return false;
            }

            AZStd::vector<ImGuiPass*> foundImGuiPasses;

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassHierarchy(passHierarchyFilter);
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [&foundImGuiPasses](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    ImGuiPass* imGuiPass = azrtti_cast<ImGuiPass*>(pass);
                    if (imGuiPass)
                    {
                        foundImGuiPasses.push_back(imGuiPass);
                    }

                     return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });

            if (foundImGuiPasses.size() == 0)
            {
                AZ_Warning("ImGuiSystemComponent", false, "Failed to find ImGui pass to activate from %s", passHierarchyFilter[0].c_str());
                return false;
            }

            if (foundImGuiPasses.size() > 1)
            {
                AZ_Warning("ImGuiSystemComponent", false, "Found more than one ImGui pass to activate from %s, only activating first one.", passHierarchyFilter[0].c_str());
            }

            ImGuiContext* context = foundImGuiPasses.at(0)->GetContext();
            m_activeContextStack.push_back(context);
            ImGuiSystemNotificationBus::Broadcast(&ImGuiSystemNotifications::ActiveImGuiContextChanged, context);
            return true;
        }

        bool ImGuiSystemComponent::PopActiveContext()
        {
            if (m_activeContextStack.size() > 0)
            {
                m_activeContextStack.pop_back();

                ImGuiContext* newContext = GetActiveContext();
                ImGuiSystemNotificationBus::Broadcast(&ImGuiSystemNotifications::ActiveImGuiContextChanged, newContext);
                return true;
            }
            AZ_Error("ImGuiSystemComponent", false, "Attempting to pop active ImGui context when there are none on the stack. There must be a Push/Pop mismatch.")
            return false;
        }

        ImGuiContext* ImGuiSystemComponent::GetActiveContext()
        {
            if (m_activeContextStack.size() > 0)
            {
                return m_activeContextStack.back();
            }
            return nullptr;
        }

        bool ImGuiSystemComponent::RenderImGuiBuffersToCurrentViewport(const ImDrawData& drawData)
        {
            auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();

            RPI::ViewportContextPtr viewportContext = atomViewportRequests->GetDefaultViewportContext();
            if (viewportContext == nullptr)
            {
                return false;
            }

            auto renderPipeline = viewportContext->GetCurrentPipeline();
            if (renderPipeline == nullptr)
            {
                return false;
            }

            for (auto imGuiPass : m_defaultImguiPassStack)
            {
                if (imGuiPass->GetRenderPipeline() == renderPipeline.get())
                {
                    imGuiPass->RenderImguiDrawData(drawData);
                    return true;
                }
            }
            return false;
        }

    } // namespace RPI
} // namespace AZ
