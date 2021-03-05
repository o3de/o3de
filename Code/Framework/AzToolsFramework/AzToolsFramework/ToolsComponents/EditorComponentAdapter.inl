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

namespace AzToolsFramework
{
    namespace Components
    {
        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::EditorComponentAdapter(const TConfiguration& configuration)
            : m_controller(configuration)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // Serialization and version conversion

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorComponentAdapter, EditorComponentBase>()
                    ->Version(1)
                    ->Field("Controller", &EditorComponentAdapter::m_controller)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorComponentAdapter>(
                        "EditorComponentAdapter", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorComponentAdapter::m_controller, "Controller", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorComponentAdapter::OnConfigurationChanged)
                        ;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Get*Services functions

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            AzFramework::Components::GetProvidedServicesHelper<TController>(services, typename AZ::HasComponentProvidedServices<TController>::type());
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            AzFramework::Components::GetRequiredServicesHelper<TController>(services, typename AZ::HasComponentRequiredServices<TController>::type());
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            AzFramework::Components::GetIncompatibleServicesHelper<TController>(services, typename AZ::HasComponentIncompatibleServices<TController>::type());
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            AzFramework::Components::GetDependentServicesHelper<TController>(services, typename AZ::HasComponentDependentServices<TController>::type());
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::BuildGameEntity(AZ::Entity* gameEntity)
        {
            gameEntity->CreateComponent<TRuntimeComponent>(m_controller.GetConfiguration());
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::Init()
        {
            EditorComponentBase::Init();
            AzFramework::Components::ComponentInitHelper<TController>::Init(m_controller);
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::Activate()
        {
            EditorComponentBase::Activate();

            if (ShouldActivateController())
            {
                m_controller.Activate(GetEntityId());
            }
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::Deactivate()
        {
            m_controller.Deactivate();
            EditorComponentBase::Deactivate();
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        bool EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::ReadInConfig(const AZ::ComponentConfig* baseConfig)
        {
            if (const auto config = azrtti_cast<const TConfiguration*>(baseConfig))
            {
                m_controller.SetConfiguration(*config);
                return true;
            }
            return false;
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        bool EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
        {
            if (auto config = azrtti_cast<TConfiguration*>(outBaseConfig))
            {
                *config = m_controller.GetConfiguration();
                return true;
            }
            return false;
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        AZ::u32 EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::OnConfigurationChanged()
        {
            m_controller.Deactivate();

            if (ShouldActivateController())
            {
                m_controller.Activate(GetEntityId());
            }

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        bool EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>::ShouldActivateController() const
        {
            return true;
        }
    } // namespace Components
} // namespace AzToolsFramework
