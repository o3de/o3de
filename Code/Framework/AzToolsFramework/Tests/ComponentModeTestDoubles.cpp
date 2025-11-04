/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeTestDoubles.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        AZ_CLASS_ALLOCATOR_IMPL(PlaceHolderComponentMode, AZ::SystemAllocator)
        AZ_CLASS_ALLOCATOR_IMPL(AnotherPlaceHolderComponentMode, AZ::SystemAllocator)
        AZ_CLASS_ALLOCATOR_IMPL(OverrideMouseInteractionComponentMode, AZ::SystemAllocator)

        void PlaceholderEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PlaceholderEditorComponent>()
                    ->Version(0)
                    ->Field("ComponentMode", &PlaceholderEditorComponent::m_componentModeDelegate);
            }
        }

        void PlaceholderEditorComponent::Activate()
        {
            EditorComponentBase::Activate();

            m_componentModeDelegate.ConnectWithSingleComponentMode<
                PlaceholderEditorComponent, PlaceHolderComponentMode>(
                    AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
        }

        void PlaceholderEditorComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();

            m_componentModeDelegate.Disconnect();
        }

        void AnotherPlaceholderEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AnotherPlaceholderEditorComponent>()
                    ->Version(0)
                    ->Field("ComponentMode", &AnotherPlaceholderEditorComponent::m_componentModeDelegate);
            }
        }

        void AnotherPlaceholderEditorComponent::GetProvidedServices(
            AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("InterestingService"));
        }

        void AnotherPlaceholderEditorComponent::GetIncompatibleServices(
            AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("InterestingService"));
        }

        void AnotherPlaceholderEditorComponent::Activate()
        {
            EditorComponentBase::Activate();

            m_componentModeDelegate.ConnectWithSingleComponentMode<
                AnotherPlaceholderEditorComponent, PlaceHolderComponentMode>(
                    AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
        }

        void AnotherPlaceholderEditorComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();

            m_componentModeDelegate.Disconnect();
        }

        void DependentPlaceholderEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DependentPlaceholderEditorComponent>()
                    ->Version(0)
                    ->Field("ComponentMode", &DependentPlaceholderEditorComponent::m_componentModeDelegate);
            }
        }

        void DependentPlaceholderEditorComponent::Activate()
        {
            EditorComponentBase::Activate();

            // connect the ComponentMode delegate to this entity/component id pair
            m_componentModeDelegate.Connect<DependentPlaceholderEditorComponent>(AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
            // setup the ComponentMode(s) to add for the editing of this Component (in this case Spline and Tube ComponentModes)
            m_componentModeDelegate.SetAddComponentModeCallback([this](const AZ::EntityComponentIdPair& entityComponentIdPair)
            {
                using namespace AzToolsFramework::ComponentModeFramework;

                // builder for PlaceHolderComponentMode for DependentPlaceholderEditorComponent
                const auto placeholdComponentModeBuilder =
                    CreateComponentModeBuilder<DependentPlaceholderEditorComponent, PlaceHolderComponentMode>(
                        entityComponentIdPair);

                // must have AnotherPlaceholderEditorComponent when using DependentPlaceholderEditorComponent
                const auto componentId = GetEntity()->FindComponent<AnotherPlaceholderEditorComponent>()->GetId();
                const auto anotherPlaceholdComponentModeBuilder =
                    CreateComponentModeBuilder<AnotherPlaceholderEditorComponent, AnotherPlaceHolderComponentMode>(
                        AZ::EntityComponentIdPair(GetEntityId(), componentId));

                // aggregate builders
                const auto entityAndComponentModeBuilder =
                    EntityAndComponentModeBuilders(
                        GetEntityId(), { placeholdComponentModeBuilder, anotherPlaceholdComponentModeBuilder });

                // updates modes to add when entering ComponentMode
                ComponentModeSystemRequestBus::Broadcast(
                    &ComponentModeSystemRequests::AddComponentModes, entityAndComponentModeBuilder);
            });
        }

        void DependentPlaceholderEditorComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();
            m_componentModeDelegate.Disconnect();
        }

        void IncompatiblePlaceholderEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<IncompatiblePlaceholderEditorComponent>()
                    ->Version(0)
                    ->Field("ComponentMode", &IncompatiblePlaceholderEditorComponent::m_componentModeDelegate);
            }
        }

        void IncompatiblePlaceholderEditorComponent::GetProvidedServices(
            AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("InterestingService"));
        }

        void IncompatiblePlaceholderEditorComponent::GetIncompatibleServices(
            AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("InterestingService"));
        }

        void IncompatiblePlaceholderEditorComponent::Activate()
        {
            EditorComponentBase::Activate();

            m_componentModeDelegate.ConnectWithSingleComponentMode<
                IncompatiblePlaceholderEditorComponent, AnotherPlaceHolderComponentMode>(
                    AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
        }

        void IncompatiblePlaceholderEditorComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();
            m_componentModeDelegate.Disconnect();
        }

        ComponentModeActionSignalNotificationChecker::ComponentModeActionSignalNotificationChecker(int busId)
        {
            ComponentModeActionSignalNotificationBus::Handler::BusConnect(busId);
        }

        ComponentModeActionSignalNotificationChecker::~ComponentModeActionSignalNotificationChecker()
        {
            ComponentModeActionSignalNotificationBus::Handler::BusDisconnect();
        }

        void ComponentModeActionSignalNotificationChecker::OnActionTriggered()
        {
            m_counter++;
        }

        PlaceHolderComponentMode::PlaceHolderComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
            : EditorBaseComponentMode(entityComponentIdPair, componentType)
        {
            ComponentModeActionSignalRequestBus::Handler::BusConnect(entityComponentIdPair);
        }

        PlaceHolderComponentMode::~PlaceHolderComponentMode()
        {
            ComponentModeActionSignalRequestBus::Handler::BusDisconnect();
        }

        AZStd::vector<AzToolsFramework::ActionOverride> PlaceHolderComponentMode::PopulateActionsImpl()
        {
            const AZ::Crc32 placeHolderComponentModeAction = AZ_CRC_CE("org.o3de.action.placeholder.test");

            return AZStd::vector<AzToolsFramework::ActionOverride>
            {
                // setup an event to notify us when an action fires
                AzToolsFramework::ActionOverride()
                    .SetUri(placeHolderComponentModeAction)
                    .SetKeySequence(QKeySequence(Qt::Key_Space))
                    .SetTitle("Test action")
                    .SetTip("This is a test action")
                    .SetEntityComponentIdPair(AZ::EntityComponentIdPair(GetEntityId(), GetComponentId()))
                    .SetCallback([this]()
                {
                    ComponentModeActionSignalNotificationBus::Event(
                        m_componentModeActionSignalNotificationBusId, &ComponentModeActionSignalNotifications::OnActionTriggered);
                })
            };
        }

        AZStd::string PlaceHolderComponentMode::GetComponentModeName() const
        {
            return "PlaceHolder Edit Mode";
        }

        AZ::Uuid PlaceHolderComponentMode::GetComponentModeType() const
        {
            return azrtti_typeid<PlaceHolderComponentMode>();
        }

        void PlaceHolderComponentMode::SetComponentModeActionNotificationBusToNotify(const int busId)
        {
            m_componentModeActionSignalNotificationBusId = busId;
        }

        AnotherPlaceHolderComponentMode::AnotherPlaceHolderComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
            : EditorBaseComponentMode(entityComponentIdPair, componentType) {}

        AZStd::string AnotherPlaceHolderComponentMode::GetComponentModeName() const
        {
            return "AnotherPlaceHolder Edit Mode";
        }

        AZ::Uuid AnotherPlaceHolderComponentMode::GetComponentModeType() const
        {
            return azrtti_typeid<AnotherPlaceHolderComponentMode>();
        }

        OverrideMouseInteractionComponentMode::OverrideMouseInteractionComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
            : EditorBaseComponentMode(entityComponentIdPair, componentType) {}

        AZStd::string OverrideMouseInteractionComponentMode::GetComponentModeName() const
        {
            return "OverrideMouseInteraction Edit Mode";
        }

        AZ::Uuid OverrideMouseInteractionComponentMode::GetComponentModeType() const
        {
            return azrtti_typeid<OverrideMouseInteractionComponentMode>();
        }

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
