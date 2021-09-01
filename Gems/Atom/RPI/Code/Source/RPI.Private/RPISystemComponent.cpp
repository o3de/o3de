/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
* @file RPISystemComponent.cpp
* @brief Contains the definition of the RPISystemComponent methods that aren't defined as inline
*/

#include <RPI.Private/RPISystemComponent.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>

#ifdef RPI_EDITOR
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#endif

namespace AZ
{
    namespace RPI
    {
        void RPISystemComponent::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<RPISystemComponent, Component>()
                    ->Version(0)
                    ->Field("RpiDescriptor", &RPISystemComponent::m_rpiDescriptor)
                    ;


                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<RPISystemComponent>("Atom RPI", "Atom Renderer")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RPISystemComponent::m_rpiDescriptor, "RPI System Settings", "Settings for create RPI system")
                        ;
                }
            }

            RPISystem::Reflect(context);
        }

        void RPISystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(RHI::Factory::GetComponentService());
        }

        void RPISystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("RPISystem", 0xf2add773));
        }

        RPISystemComponent::RPISystemComponent()
        {
        #ifdef RPI_EDITOR
            AZ_Assert(m_materialFunctorRegistration == nullptr, "Material functor registration should be initialized with nullptr. "
                "And allocated depending on the component is in editors or not.");
            m_materialFunctorRegistration = aznew MaterialFunctorSourceDataRegistration;
            m_materialFunctorRegistration->Init();
        #endif
        }

        RPISystemComponent::~RPISystemComponent()
        {
        #ifdef RPI_EDITOR
            if (m_materialFunctorRegistration)
            {
                m_materialFunctorRegistration->Shutdown();
                delete m_materialFunctorRegistration;
            }
        #endif
        }

        void RPISystemComponent::Activate()
        {
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->GetObject(m_rpiDescriptor, "/O3DE/Atom/RPI/Initialization");
            }

            m_rpiSystem.Initialize(m_rpiDescriptor);
            AZ::TickBus::Handler::BusConnect();
        }

        void RPISystemComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            m_rpiSystem.Shutdown();
        }

        void RPISystemComponent::OnTick(float deltaTime, ScriptTimePoint time)
        {
            if (deltaTime == 0.f)
            {
                return;
            }


            m_rpiSystem.SimulationTick(deltaTime, static_cast<float>(time.GetMilliseconds()));
            m_rpiSystem.RenderTick();
        }

        int RPISystemComponent::GetTickOrder()
        {
            return AZ::ComponentTickBus::TICK_RENDER;
        }
    } // namespace RPI
} // namespace AZ
