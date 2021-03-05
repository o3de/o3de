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
                serializeContext->Class<ComponentAdapter, Component>()
                    ->Version(1)
                    ->Field("Controller", &ComponentAdapter::m_controller)
                    ;
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

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::Init()
        {
            ComponentInitHelper<TController>::Init(m_controller);
        }

        template<typename TController, typename TConfiguration>
        void ComponentAdapter<TController, TConfiguration>::Activate()
        {
            m_controller.Activate(GetEntityId());
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
