/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/ComponentAdapterHelpers.h>

namespace AzFramework
{
    namespace Components
    {
        template<typename TController, typename TConfiguration>
        ComponentAdapter<TController, TConfiguration>::ComponentAdapter(const TConfiguration& configuration)
            : m_controller(configuration)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // Serialization and version conversion

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::Reflect(AZ::ReflectContext* context)
        {
            TController::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                // clang-format off
                serializeContext->Class<ComponentAdapter, Component>()
                    ->Version(1)
                    ->Field("Controller", &ComponentAdapter::m_controller)
                    ;
                // clang-format on
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Get*Services functions

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            GetProvidedServicesHelper<TController>(services, typename AZ::HasComponentProvidedServices<TController>::type());
        }

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            GetRequiredServicesHelper<TController>(services, typename AZ::HasComponentRequiredServices<TController>::type());
        }

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            GetIncompatibleServicesHelper<TController>(services, typename AZ::HasComponentIncompatibleServices<TController>::type());
        }

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            GetDependentServicesHelper<TController>(services, typename AZ::HasComponentDependentServices<TController>::type());
        }

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::Init()
        {
            ComponentInitHelper<TController>::Init(m_controller);
        }

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::Activate()
        {
            ComponentActivateHelper<TController>::Activate(m_controller, AZ::EntityComponentIdPair(GetEntityId(), GetId()));
        }

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::Deactivate()
        {
            m_controller.Deactivate();
        }

        template<typename TController, typename TConfiguration>
        bool ComponentAdapter<TController, TConfiguration>::ReadInConfig(const AZ::ComponentConfig* baseConfig)
        {
            if (const auto config = azrtti_cast<const TConfiguration*>(baseConfig))
            {
                m_controller.SetConfiguration(*config);
                return true;
            }
            return false;
        }

        template<typename TController, typename TConfiguration>
        bool ComponentAdapter<TController, TConfiguration>::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
        {
            if (auto config = azrtti_cast<TConfiguration*>(outBaseConfig))
            {
                *config = m_controller.GetConfiguration();
                return true;
            }
            return false;
        }
    } // namespace Components
} // namespace AzFramework
