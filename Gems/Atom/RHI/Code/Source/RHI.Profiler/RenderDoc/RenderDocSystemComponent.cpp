/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI.Profiler/RenderDoc/RenderDocSystemComponent.h>
#include <RHI.Profiler/Utils.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Atom_RHI_Traits_Platform.h>

namespace AZ::RHI
{
    void RenderDocSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<RenderDocSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void RenderDocSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GraphicsProfilerService"));
    }

    void RenderDocSystemComponent::Activate()
    {
        bool loadRenderDoc = RHI::ShouldLoadProfiler("RenderDoc");
        AZStd::string filePath = ATOM_RENDERDOC_RUNTIME_PATH;
        if (!filePath.empty())
        {
            AzFramework::StringFunc::Path::AppendSeparator(filePath);
        }
        filePath += AZ_TRAIT_RENDERDOC_MODULE;
        m_dynamicModule = DynamicModuleHandle::Create(filePath.c_str());
        AZ_Assert(m_dynamicModule, "Failed to create RenderDoc dynamic module");
        if (m_dynamicModule->Load(loadRenderDoc ? DynamicModuleHandle::LoadFlags::None : DynamicModuleHandle::LoadFlags::NoLoad))
        {
            pRENDERDOC_GetAPI renderDocGetAPIFunc = m_dynamicModule->GetFunction<pRENDERDOC_GetAPI>("RENDERDOC_GetAPI");
            if (renderDocGetAPIFunc)
            {
                if (!renderDocGetAPIFunc(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&m_renderDocApi)))
                {
                    m_renderDocApi = nullptr;
                }
            }

            if (m_renderDocApi)
            {
                GraphicsProfilerBus::Handler::BusConnect();
                // Prevent RenderDoc from handling any exceptions that may interfere with the O3DE exception handler
                m_renderDocApi->UnloadCrashHandler();
                AZ_Printf("RenderDocSystemComponent", "RenderDoc loaded. Capture path is %s.\n", m_renderDocApi->GetCaptureFilePathTemplate());
            }
            else
            {
                AZ_Printf("RenderDocSystemComponent", "RenderDoc module loaded but failed to retrieve API function pointer.\n");
            }
        }
    }

    void RenderDocSystemComponent::Deactivate()
    {
        GraphicsProfilerBus::Handler::BusDisconnect();
        if (m_dynamicModule)
        {
            m_dynamicModule->Unload();
        }
        m_renderDocApi = nullptr;
    }

    void RenderDocSystemComponent::StartCapture(const AzFramework::NativeWindowHandle window)
    {
        AZ_Assert(m_renderDocApi, "Null RenderDoc API");
        m_renderDocApi->StartFrameCapture(nullptr, window); 
    }

    bool RenderDocSystemComponent::EndCapture(const AzFramework::NativeWindowHandle window)
    {
        AZ_Assert(m_renderDocApi, "Null RenderDoc API");
        AZ_Printf("RenderDocSystemComponent", "Saving RenderDoc capture to %s\n", m_renderDocApi->GetCaptureFilePathTemplate());
        return m_renderDocApi->EndFrameCapture(nullptr, window) == 1;
    }

    void RenderDocSystemComponent::TriggerCapture()
    {
        AZ_Assert(m_renderDocApi, "Null RenderDoc API");
        AZ_Printf("RenderDocSystemComponent", "Saving RenderDoc capture to %s\n", m_renderDocApi->GetCaptureFilePathTemplate());
        m_renderDocApi->TriggerCapture();
    }
} // namespace AZ::RHI
