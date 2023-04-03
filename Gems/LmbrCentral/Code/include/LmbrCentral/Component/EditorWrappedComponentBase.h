/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //! A base class for editor components that need to wrap runtime components, use a configuration object, and respond to visibility toggling
    template<typename TComponent, typename TConfiguration>
    class EditorWrappedComponentBase
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EditorVisibilityNotificationBus::Handler
    {
    public:
        using WrappedComponentType = TComponent;
        using WrappedConfigType = TConfiguration;

        AZ_RTTI((EditorWrappedComponentBase, "{059BC2AF-B086-4D5E-8F6C-2827AB69ED16}", TComponent, TConfiguration), EditorComponentBase);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorVisibilityNotificationBus
        void OnEntityVisibilityChanged(bool visibility) override;

    protected:
        template<typename TDerivedClass, typename TBaseClass>
        static void ReflectSubClass(AZ::ReflectContext* context, unsigned int version = 0, AZ::SerializeContext::VersionConverter versionConverter = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        virtual AZ::u32 ConfigurationChanged();

        TComponent m_component;
        TConfiguration m_configuration;
        bool m_visible = true;
        bool m_runtimeComponentActive = false;
    };

} // namespace LmbrCentral

#include "EditorWrappedComponentBase.inl"
