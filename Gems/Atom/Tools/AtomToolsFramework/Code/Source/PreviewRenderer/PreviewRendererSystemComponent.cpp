/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <PreviewRenderer/PreviewRendererSystemComponent.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

namespace AtomToolsFramework
{
    void PreviewRendererSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PreviewRendererSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PreviewRendererSystemComponent>("PreviewRendererSystemComponent", "System component that manages a global PreviewRenderer.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void PreviewRendererSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PreviewRendererSystem"));
    }
    
    void PreviewRendererSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("RPISystem"));
    }

    void PreviewRendererSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PreviewRendererSystem"));
    }

    void PreviewRendererSystemComponent::Init()
    {
    }

    void PreviewRendererSystemComponent::Activate()
    {
        AZ::SystemTickBus::Handler::BusConnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        PreviewRendererSystemRequestBus::Handler::BusConnect();
    }

    void PreviewRendererSystemComponent::Deactivate()
    {
        PreviewRendererSystemRequestBus::Handler::BusDisconnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
        m_previewRenderer.reset();
    }

    void PreviewRendererSystemComponent::OnApplicationAboutToStop()
    {
        m_previewRenderer.reset();
    }

    void PreviewRendererSystemComponent::OnSystemTick()
    {
        // Do not create the preview reader until the RPI has been initialized
        if (AZ::RPI::RPISystemInterface::Get() && AZ::RPI::RPISystemInterface::Get()->IsInitialized())
        {
            if (!m_previewRenderer)
            {
                m_previewRenderer.reset(aznew AtomToolsFramework::PreviewRenderer(
                    "PreviewRendererSystemComponent Preview Scene", "PreviewRendererSystemComponent Preview Pipeline"));
            }
            AZ::SystemTickBus::Handler::BusDisconnect();
        }
    }
} // namespace AtomToolsFramework
