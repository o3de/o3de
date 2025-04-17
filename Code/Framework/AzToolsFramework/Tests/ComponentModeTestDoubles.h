/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        /// @file
        /// A collection of placeholder Components to use in Component Mode tests.
        /// These Components do nothing useful in and of themselves but help verify Component Mode behavior.

        class PlaceholderEditorComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                PlaceholderEditorComponent, "{A246ABC8-B5AF-4302-BFE7-F1927EE0203F}",
                AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);

            // AZ::Component ...
            void Activate() override;
            void Deactivate() override;

        private:
            ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                           ///< and creating a concrete ComponentMode.
        };

        class AnotherPlaceholderEditorComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                AnotherPlaceholderEditorComponent, "{3CF10B26-461C-40F8-8E03-2F6BD3E093DA}",
                AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            // AZ::Component ...
            void Activate() override;
            void Deactivate() override;

        private:
            ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                           ///< and creating a concrete ComponentMode.
        };

        class DependentPlaceholderEditorComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                DependentPlaceholderEditorComponent, "{A5093BD0-5585-4DA5-92B8-408F67B147C0}",
                AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);

            // AZ::Component ...
            void Activate() override;
            void Deactivate() override;

        private:
            ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                           ///< and creating a concrete ComponentMode.
        };

        class IncompatiblePlaceholderEditorComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                IncompatiblePlaceholderEditorComponent, "{284C7965-87C2-41C7-909B-1345061B3DC7}",
                AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            // AZ::Component ...
            void Activate() override;
            void Deactivate() override;

        private:
            ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                           ///< and creating a concrete ComponentMode.
        };

        // Simple component for testing that can be supplied a component mode
        // type via template argument.
        template<typename ComponentModeT>
        class TestComponentModeComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                TestComponentModeComponent, "{57B53B5D-D51B-4CCB-A875-9CF630282667}",
                AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<TestComponentModeComponent>()
                        ->Version(0)
                        ->Field("ComponentMode", &TestComponentModeComponent::m_componentModeDelegate);
                }
            }

            // AZ::Component ...
            void Activate() override
            {
                EditorComponentBase::Activate();
                m_componentModeDelegate.ConnectWithSingleComponentMode<
                    TestComponentModeComponent, ComponentModeT>(
                        AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
            }

            void Deactivate() override
            {
                EditorComponentBase::Deactivate();
                m_componentModeDelegate.Disconnect();
            }

        private:
            ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                           ///< and creating a concrete ComponentMode.
        };

        /// A simple request bus to let us notify an entity component Id pair what address
        /// to listen on for ComponentModeActionSignalNotifications.
        class ComponentModeActionSignalRequests
            : public AZ::EntityComponentBus
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual void SetComponentModeActionNotificationBusToNotify(int busId) = 0;
        };

        using ComponentModeActionSignalRequestBus = AZ::EBus<ComponentModeActionSignalRequests>;

        /// A simple bus to raise an event when a particular action has occurred.
        class ComponentModeActionSignalNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = int;

            virtual void OnActionTriggered() {}
        };

        using ComponentModeActionSignalNotificationBus = AZ::EBus<ComponentModeActionSignalNotifications>;

        /// Implements ComponentModeActionSignalNotificationBus and increments a counter
        /// each time OnActionTriggered is called.
        class ComponentModeActionSignalNotificationChecker
            : private ComponentModeActionSignalNotificationBus::Handler
        {
        public:
            explicit ComponentModeActionSignalNotificationChecker(int busId);
            ~ComponentModeActionSignalNotificationChecker();
            
            int GetCount() const { return m_counter; }

        private:
            // ComponentModeActionSignalNotificationBus ...
            void OnActionTriggered() override;

            int m_counter = 0; ///< Counter to be incremented in OnActionTriggered.
        };

        class PlaceHolderComponentMode
            : public EditorBaseComponentMode
            , private ComponentModeActionSignalRequestBus::Handler
        {
        public:
            AZ_RTTI(PlaceHolderComponentMode, "{B4C7F635-852E-4D68-AE12-5AD281A0266F}", EditorBaseComponentMode);
            AZ_CLASS_ALLOCATOR_DECL

            PlaceHolderComponentMode(
                    const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
            PlaceHolderComponentMode(const PlaceHolderComponentMode&) = delete;
            PlaceHolderComponentMode& operator=(const PlaceHolderComponentMode&) = delete;
            PlaceHolderComponentMode(PlaceHolderComponentMode&&) = delete;
            PlaceHolderComponentMode& operator=(PlaceHolderComponentMode&&) = delete;
            ~PlaceHolderComponentMode();

            // EditorBaseComponentMode ...
            void Refresh() override {}
            AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
            AZStd::string GetComponentModeName() const override;
            AZ::Uuid GetComponentModeType() const override;

            // ComponentModeActionSignalRequestBus ...
            void SetComponentModeActionNotificationBusToNotify(int busId) override;

        private:
            int m_componentModeActionSignalNotificationBusId = 0; ///< This is the busId to send the action notification to.
        };

        class AnotherPlaceHolderComponentMode
            : public EditorBaseComponentMode
        {
        public:
            AZ_RTTI(AnotherPlaceHolderComponentMode, "{8F8C6D75-0696-488B-A15A-822085A94D4A}", EditorBaseComponentMode);
            AZ_CLASS_ALLOCATOR_DECL

            AnotherPlaceHolderComponentMode(
                const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
            AnotherPlaceHolderComponentMode(const AnotherPlaceHolderComponentMode&) = delete;
            AnotherPlaceHolderComponentMode& operator=(const AnotherPlaceHolderComponentMode&) = delete;
            AnotherPlaceHolderComponentMode(AnotherPlaceHolderComponentMode&&) = delete;
            AnotherPlaceHolderComponentMode& operator=(AnotherPlaceHolderComponentMode&&) = delete;
            ~AnotherPlaceHolderComponentMode() = default;

            // EditorBaseComponentMode ...
            void Refresh() override {}
            AZStd::string GetComponentModeName() const override;
            AZ::Uuid GetComponentModeType() const override;
        };

        // ComponentMode which overrides mouse events
        class OverrideMouseInteractionComponentMode
            : public EditorBaseComponentMode
            , public AzToolsFramework::ViewportInteraction::ViewportSelectionRequests
        {
        public:
            AZ_RTTI(OverrideMouseInteractionComponentMode, "{3398C51E-F4C8-4EFD-BCA2-7681718C44FE}", EditorBaseComponentMode);
            AZ_CLASS_ALLOCATOR_DECL

            OverrideMouseInteractionComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);

            // EditorBaseComponentMode ...
            void Refresh() override {}
            AZStd::string GetComponentModeName() const override;
            AZ::Uuid GetComponentModeType() const override;

        private:
            /// AzToolsFramework::ViewportInteraction::ViewportSelectionRequests ...
            bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override
            {
                // Pretend like we are handling some mouse interaction 
                AZ_UNUSED(mouseInteraction);
                return true;
            }
        };

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
