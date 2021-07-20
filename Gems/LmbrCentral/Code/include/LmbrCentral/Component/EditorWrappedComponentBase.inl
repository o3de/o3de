/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace LmbrCentral
{
    template <typename TComponent, typename TConfiguration, int TVersion>
    bool EditorWrappedComponentBaseVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < TVersion)
        {
            TConfiguration configData;
            AzToolsFramework::Components::EditorComponentBase editorComponentBaseData;

            if (!classElement.FindSubElementAndGetData(AZ_CRC("Configuration", 0xa5e2a5d7), configData)
                || !classElement.FindSubElementAndGetData(AZ_CRC("BaseClass1", 0xd4925735), editorComponentBaseData))
            {
                AZ_Error("LmbrCentral", false, "Failed to find and get data from Configuration or BaseClass1 element");
                return false;
            }

            if (!classElement.RemoveElementByName(AZ_CRC("Configuration", 0xa5e2a5d7))
                || !classElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735)))
            {
                AZ_Error("LmbrCentral", false, "Failed to remove Configuration or BaseClass1 element");
                return false;
            }

            EditorWrappedComponentBase<TComponent, TConfiguration> wrappedComponentBaseInstance;
            int baseIndex = classElement.AddElementWithData(context, "BaseClass1", wrappedComponentBaseInstance);

            auto& wrappedComponentBaseElement = classElement.GetSubElement(baseIndex);
            auto* editorComponentBaseElement = wrappedComponentBaseElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            auto* configurationElement = wrappedComponentBaseElement.FindSubElement(AZ_CRC("Configuration", 0xa5e2a5d7));

            if (!editorComponentBaseElement || !configurationElement)
            {
                AZ_Error("LmbrCentral", false, "Failed to find BaseClass1 or Configuration element");
                return false;
            }

            if (!editorComponentBaseElement->SetData(context, editorComponentBaseData)
                || !configurationElement->SetData(context, configData))
            {
                AZ_Error("LmbrCentral", false, "Failed to set data on BaseClass1 or Configuration element");
                return false;
            }
        }

        return true;
    }

    template <typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWrappedComponentBase, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(0)
                ->Field("Configuration", &EditorWrappedComponentBase::m_configuration);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorWrappedComponentBase>("WrappedComponentBase", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::DisplayOrder, 50) // There's no special meaning to 50, we just need this class to move down and display below any children
                    ->DataElement(0, &EditorWrappedComponentBase::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWrappedComponentBase::ConfigurationChanged);
            }
        }
    }

    template <typename TComponent, typename TConfiguration>
    template<typename TDerivedClass, typename TBaseClass>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::ReflectSubClass(AZ::ReflectContext* context, unsigned int version, AZ::SerializeContext::VersionConverter versionConverter)
    {
        TBaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TDerivedClass, TBaseClass>()
                ->Version(version, versionConverter)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TDerivedClass>(
                    TDerivedClass::s_componentName, TDerivedClass::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, TDerivedClass::s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, TDerivedClass::s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, TDerivedClass::s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, TDerivedClass::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Set of helper functions to handle types that don't have one of the Get*Services functions

    template<typename T>
    void GetProvidedServicesHelper(AZ::ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) {}

    template<typename T>
    void GetProvidedServicesHelper(AZ::ComponentDescriptor::DependencyArrayType& services, const AZStd::true_type&)
    {
        T::GetProvidedServices(services);
    }

    template<typename T>
    void GetRequiredServicesHelper(AZ::ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) {}

    template<typename T>
    void GetRequiredServicesHelper(AZ::ComponentDescriptor::DependencyArrayType& services, const AZStd::true_type&)
    {
        T::GetRequiredServices(services);
    }

    template<typename T>
    void GetIncompatibleServicesHelper(AZ::ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) {}

    template<typename T>
    void GetIncompatibleServicesHelper(AZ::ComponentDescriptor::DependencyArrayType& services, const AZStd::true_type&)
    {
        T::GetIncompatibleServices(services);
    }

    template<typename T>
    void GetDependentServicesHelper(AZ::ComponentDescriptor::DependencyArrayType&, const AZStd::false_type&) {}

    template<typename T>
    void GetDependentServicesHelper(AZ::ComponentDescriptor::DependencyArrayType& services, const AZStd::true_type&)
    {
        T::GetDependentServices(services);
    }
    //////////////////////////////////////////////////////////////////////////

    template<typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        GetRequiredServicesHelper<TComponent>(services, typename AZ::HasComponentRequiredServices<TComponent>::type());
    }

    template<typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        GetIncompatibleServicesHelper<TComponent>(services, typename AZ::HasComponentIncompatibleServices<TComponent>::type());
    }

    template<typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        GetProvidedServicesHelper<TComponent>(services, typename AZ::HasComponentProvidedServices<TComponent>::type());
    }

    template<typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        GetDependentServicesHelper<TComponent>(services, typename AZ::HasComponentDependentServices<TComponent>::type());
    }

    template<typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->AddComponent(aznew TComponent(m_configuration));
    }

    template <typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();
        m_component.ReadInConfig(&m_configuration);
        m_component.Init();
    }

    template <typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            m_visible, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        m_component.ReadInConfig(&m_configuration);
        m_component.SetEntity(GetEntity());

        if (m_visible)
        {
            m_component.Activate();
        }
    }

    template <typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::Deactivate()
    {
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_component.Deactivate();
        m_component.SetEntity(nullptr); // remove the entity association, in case the parent component is being removed, otherwise the component will be reactivated
    }

    template <typename TComponent, typename TConfiguration>
    void EditorWrappedComponentBase<TComponent, TConfiguration>::OnEntityVisibilityChanged(bool visibility)
    {
        m_visible = visibility;
        ConfigurationChanged();
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorWrappedComponentBase<TComponent, TConfiguration>::ConfigurationChanged()
    {
        m_component.Deactivate();
        m_component.ReadInConfig(&m_configuration);

        if (m_visible && m_component.GetEntity())
        {
            m_component.Activate();
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }
}
