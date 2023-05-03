/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        /// Utility factory function to create a ComponentModeBuilder for a specific EditorComponent
        template<typename EditorComponentType, typename EditorComponentModeType, typename ...Params>
        ComponentModeBuilder CreateComponentModeBuilder(const AZ::EntityComponentIdPair& entityComponentIdPair, Params&&... params)
        {
            const auto componentModeBuilderFunc = AZStd::bind(
                [entityComponentIdPair](Params&&... params)
                {
                return AZStd::make_unique<EditorComponentModeType>(
                    entityComponentIdPair, AZ::AzTypeInfo<EditorComponentType>::Uuid(), AZStd::forward<Params>(params)...);
                }, AZStd::forward<Params>(params)...);

            return ComponentModeBuilder(
                entityComponentIdPair.GetComponentId(), AZ::AzTypeInfo<EditorComponentType>::Uuid(), componentModeBuilderFunc);
        }

        /// Helper to provide ComponentMode button in the Entity Inspector and double click
        /// handling in the viewport for entering/exiting ComponentMode.
        class ComponentModeDelegate
            : private ComponentModeDelegateRequestBus::Handler
            , private EntitySelectionEvents::Bus::Handler
            , private EditorEntityVisibilityNotificationBus::Handler
            , private EditorEntityLockComponentNotificationBus::Handler
        {
        public:
            /// @cond
            AZ_CLASS_ALLOCATOR(ComponentModeDelegate, AZ::SystemAllocator);
            AZ_RTTI(ComponentModeDelegate, "{635B28F0-601A-43D2-A42A-02C4A88CD9C2}");

            static void Reflect(AZ::ReflectContext* context);
            /// @endcond

            /// Connect the ComponentModeDelegate to listen for Editor selection events.
            /// Editor Component must call Connect (or variant of Connect), usually in Component::Activate, and
            /// Disconnect, most likely in Component::Deactivate.
            template<typename EditorComponentType>
            void Connect(
                const AZ::EntityComponentIdPair& entityComponentIdPair, EditorComponentSelectionRequestsBus::Handler* handler)
            {
                ConnectInternal(entityComponentIdPair, AZ::AzTypeInfo<EditorComponentType>::Uuid(), handler);
            }

            /// Connect the ComponentModeDelegate to listen for Editor selection events and
            /// simultaneously add a single concrete ComponentMode (common case utility).
            template<typename EditorComponentType, typename EditorComponentModeType, typename ...Params>
            void ConnectWithSingleComponentMode(
                const AZ::EntityComponentIdPair& entityComponentIdPair,
                EditorComponentSelectionRequestsBus::Handler* handler,
                Params&&... params)
            {
                Connect<EditorComponentType>(entityComponentIdPair, handler);
                IndividualComponentMode<EditorComponentType, EditorComponentModeType>(AZStd::forward<Params>(params)...);
            }

            /// Disconnect the ComponentModeDelegate to stop listening for Editor selection events.
            void Disconnect();

            /// Has this specific ComponentModeDelegate (for a specific Entity and Component)
            /// been added to ComponentMode.
            bool AddedToComponentMode() const;
            /// The function to call when this ComponentModeDelegate detects an event to enter ComponentMode.
            void SetAddComponentModeCallback(
                const AZStd::function<void(const AZ::EntityComponentIdPair&)>& addComponentModeCallback);
            
            /// Is the EntityComponentIdPair connected to the ComponentModeDelegate.
            bool IsConnected() const
            {
                return m_entityComponentIdPair != AZ::EntityComponentIdPair(AZ::EntityId(), AZ::InvalidComponentId) &&
                    !m_componentType.IsNull();  
            }

        private:
            void ConnectInternal(
                const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType,
                EditorComponentSelectionRequestsBus::Handler* handler);

            /// Utility function for the common case of creating a single ComponentMode for a Component.
            template<typename EditorComponentType, typename EditorComponentModeType, typename ...Params>
            void IndividualComponentMode(Params&&... params)
            {
                SetAddComponentModeCallback(AZStd::bind(
                    [](const AZ::EntityComponentIdPair& entityComponentIdPair, Params&&... params)
                    {
                        const auto componentModeBuilder = CreateComponentModeBuilder<EditorComponentType, EditorComponentModeType>(
                            entityComponentIdPair, AZStd::forward<Params>(params)...);

                        const auto entityAndComponentModeBuilder =
                            EntityAndComponentModeBuilders(entityComponentIdPair.GetEntityId(), componentModeBuilder);

                        ComponentModeSystemRequestBus::Broadcast(
                            &ComponentModeSystemRequests::AddComponentModes, entityAndComponentModeBuilder);
                    },
                    AZStd::placeholders::_1,
                    AZStd::forward<Params>(params)...));
            }

            void AddComponentMode();

            // EntitySelectionEvents
            void OnSelected() override;
            void OnDeselected() override;

            // ComponentModeDelegateRequestBus
            bool DetectEnterComponentModeInteraction(
                const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
            bool DetectLeaveComponentModeInteraction(
                const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
            void AddComponentModeOfType(AZ::Uuid componentType) override;

            // EditorEntityVisibilityNotificationBus
            void OnEntityVisibilityChanged(bool visibility) override;

            // EditorEntityLockComponentNotificationBus
            void OnEntityLockChanged(bool locked) override;

            /// Is the ComponentMode button active/operational.
            /// It will not be if the entity with this component is either locked or hidden.
            /// It will also not be active if the Entity is pinned but not selected.
            bool ComponentModeButtonInactive() const;

            AZ::Uuid m_componentType; ///< The type of component entering ComponentMode.
            AZ::EntityComponentIdPair m_entityComponentIdPair; ///< The Entity and Component Id this ComponentMode is bound to.
            EditorComponentSelectionRequestsBus::Handler* m_handler = nullptr; /**< Selection handler (used for double clicking
                                                                                 *  on a component to enter ComponentMode). */
            AZStd::function<void(const AZ::EntityComponentIdPair&)> m_addComponentMode; ///< Callback to add ComponentMode for this component.

            /// ComponentMode Button
            void OnComponentModeEnterButtonPressed();
            void OnComponentModeLeaveButtonPressed();
        };

        /// If this Entity had a Component supporting a ComponentMode, would
        /// it be possible for it to enter it given the current Editor state.
        bool CouldBeginComponentModeWithEntity(AZ::EntityId entityId);

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
