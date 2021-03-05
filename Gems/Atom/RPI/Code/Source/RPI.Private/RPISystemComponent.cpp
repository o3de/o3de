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

/**
* @file RPISystemComponent.cpp
* @brief Contains the definition of the RPISystemComponent methods that aren't defined as inline
*/

#include <RPI.Private/RPISystemComponent.h>

#include <Atom/RHI/Factory.h>

#include <AzCore/IO/IOUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>
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
            // [GFX TODO] [ATOM-1436] this can be removed when setup and save system component's configure from projectConfigure.exe is fixed.
            m_rpiDescriptor.m_rhiSystemDescriptor.m_drawListTags.push_back(AZ::Name("forward"));

            m_rpiSystem.Initialize(m_rpiDescriptor);
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void RPISystemComponent::Deactivate()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            m_rpiSystem.Shutdown();
        }

        void RPISystemComponent::OnSystemTick()
        {
            m_rpiSystem.SimulationTick();
            m_rpiSystem.RenderTick();
        }

    } // namespace RPI
} // namespace AZ
