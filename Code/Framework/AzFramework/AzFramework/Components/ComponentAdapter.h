/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

namespace AZ
{
    class ReflectContxt;
}

namespace AzFramework
{
    namespace Components
    {
        /** ComponentAdapter is a utility base class that provides a consistent pattern for implementing components
        that operate in the launcher but may need to share code in different contexts like the editor.

        ComponentAdapter achieves this by delegating to a controller class that implements common behavior instead of
        duplicating code between multiple components.

        To use the ComponentAdapter, 2 classes are required for the template:
         - a class that implements the functions required for TController (see below)
         - a configuration struct/class which extends AZ::ComponentConfig

         The concrete component extends the adapter and implements behavior which is unique to the component.

        TController can handle any common functionality between the runtime and editor component and is where most of the code for the
        component will live

        TConfiguration is where any data that needs to be serialized out should live.

        TController must implement certain functions to conform to the template. These functions mirror those in
        AZ::Component and must be accesible to any adapter that follows this pattern:

        @code
            static void Reflect(AZ::ReflectContext* context);
            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ComponentConfigurationType& config);
            const ComponentConfigurationType& GetConfiguration() const;
        @endcode

        In addition, certain functions will optionally be called if they are available:

        @code
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            void Init();
        @endcode

        It is recommended that TController handle the SerializeContext, but the editor components handle
        the EditContext. TController can friend itself to the editor component to make this work if required.
    */
        template<typename TController, typename TConfiguration = AZ::ComponentConfig>
        class ComponentAdapter : public AZ::Component
        {
        public:
            AZ_RTTI((ComponentAdapter, "{644A9187-4FDB-42C1-9D59-DD75304B551A}", TController, TConfiguration), AZ::Component);

            ComponentAdapter() = default;
            explicit ComponentAdapter(const TConfiguration& configuration);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            // AZ::Component overrides ...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

        protected:
            static void Reflect(AZ::ReflectContext* context);

            // AZ::Component overrides ...
            bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
            bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

            TController m_controller;
        };
    } // namespace Components
} // namespace AzFramework

#include "ComponentAdapter.inl"
