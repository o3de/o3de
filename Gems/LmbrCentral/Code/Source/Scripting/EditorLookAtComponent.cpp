/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorLookAtComponent.h"
#include "LookAtComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>

namespace LmbrCentral
{
    //=========================================================================
    void EditorLookAtComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorLookAtComponent, AZ::Component>()
                ->Version(1)
                ->Field("Target", &EditorLookAtComponent::m_targetId)
                ->Field("ForwardAxis", &EditorLookAtComponent::m_forwardAxis)
                ->Field("Strength", &EditorLookAtComponent::m_strength)
                ->Field("FixatePitch", &EditorLookAtComponent::m_fixatePitch)
                ->Field("FixateRoll", &EditorLookAtComponent::m_fixateRoll)
                ->Field("FixateYaw", &EditorLookAtComponent::m_fixateYaw)
                ->Field("Enabled", &EditorLookAtComponent::m_enabled)
                ->Field("ApplyLookAtTransform", &EditorLookAtComponent::m_applyLookAtTransform)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorLookAtComponent>("Look At", "Force an entity to always look at a given target")

                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/LookAt.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/LookAt.svg")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/gameplay/look-at/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLookAtComponent::m_targetId, "Target", "The entity to look at")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLookAtComponent::OnTargetChanged)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorLookAtComponent::m_forwardAxis, "Forward Axis", "The local axis that should point at the target")
                        ->EnumAttribute(AZ::Transform::Axis::YPositive, "Y+")
                        ->EnumAttribute(AZ::Transform::Axis::YNegative, "Y-")
                        ->EnumAttribute(AZ::Transform::Axis::XPositive, "X+")
                        ->EnumAttribute(AZ::Transform::Axis::XNegative, "X-")
                        ->EnumAttribute(AZ::Transform::Axis::ZPositive, "Z+")
                        ->EnumAttribute(AZ::Transform::Axis::ZNegative, "Z-")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLookAtComponent::RecalculateTransform)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorLookAtComponent::m_strength, "Strength", "Determines how quickly the rotation is performed and how much it resists changes in the rotation")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLookAtComponent::m_fixatePitch, "Fixate Pitch", "Whether the pitch is fixated towards the Look At entity / point")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLookAtComponent::RecalculateTransform)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLookAtComponent::m_fixateRoll, "Fixate Roll", "Whether the pitch is fixated towards the Look At entity / point")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLookAtComponent::RecalculateTransform)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLookAtComponent::m_fixateYaw, "Fixate Yaw", "Whether the pitch is fixated towards the Look At entity / point")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLookAtComponent::RecalculateTransform)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLookAtComponent::m_enabled, "Enabled", "Whether the Look At component is enabled or disabled")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLookAtComponent::OnEnabledChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLookAtComponent::m_applyLookAtTransform, "Apply Look At Transform", "Whether the Look At transform is applied when the component is enabled")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLookAtComponent::RecalculateTransform)
                    ;
            }
        }
    }

    //=========================================================================
    void EditorLookAtComponent::Activate()
    {
        if (m_targetId.IsValid())
        {
            AZ::EntityBus::Handler::BusConnect(m_targetId);
        }
    }

    //=========================================================================
    void EditorLookAtComponent::Deactivate()
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void EditorLookAtComponent::OnEntityActivated(const AZ::EntityId& /*entity*/)
    {
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_targetId);
    }

    //=========================================================================
    void EditorLookAtComponent::OnEntityDeactivated(const AZ::EntityId& /*entity*/)
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(GetEntityId());
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_targetId);
    }

    //=========================================================================
    void EditorLookAtComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        // We need to defer the Look-At transform change.  Can't flush it through here because
        // that will cause a feedback loop and the originator of the transform change might
        // not be finished broadcasting out to listeners.  If we set look-at here, the look-at
        // transform can be stomped later by the original data.

        // Method 1: Connect to the Tick bus for a frame.  In the next OnTick we set the
        // Look-At transform and disconnect.
        AZ::TickBus::Handler::BusConnect();


        // Method 2: We may want to stay connected to the TickBus if the transform is constantly
        // changing.  Without a good heuristic to detect this case, we'll stick with Method 1.
        // Here's a gist of this method:

        // connect/disconnect to TickBus in OnEntityActivated/OnEntityDeactivated.
        // set a 'shouldRecalc' flag here in OnTransformChanged to true;
        // in OnTick do this:
        // if (shouldRecalc)
        // {
        //      RecalculateTransform();
        //      shouldRecalc = false;
        // }
    }

    //=========================================================================
    void EditorLookAtComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        LookAtComponent* lookAtComponent = gameEntity->CreateComponent<LookAtComponent>();

        if (lookAtComponent)
        {
            lookAtComponent->m_targetId = m_targetId;
            lookAtComponent->m_forwardAxis = m_forwardAxis;
            lookAtComponent->m_strength = m_strength;
            lookAtComponent->m_fixatePitch = m_fixatePitch;
            lookAtComponent->m_fixateRoll = m_fixateRoll;
            lookAtComponent->m_fixateYaw = m_fixateYaw;
            lookAtComponent->m_enabled = m_enabled;
            lookAtComponent->m_applyLookAtTransform = m_applyLookAtTransform;
        }
    }

    //=========================================================================
    void EditorLookAtComponent::OnTargetChanged()
    {
        if (m_oldTargetId.IsValid())
        {
            // Disconnect from the old target entity
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_oldTargetId);
            AZ::EntityBus::Handler::BusDisconnect(m_oldTargetId);
            m_oldTargetId = AZ::EntityId();
        }

        if (m_targetId.IsValid())
        {
            // Connect to the new target entity
            // Won't connect to the new target's transform bus until we receive notification
            // that target is activated via the EntityBus.
            AZ::EntityBus::Handler::BusConnect(m_targetId);
            m_oldTargetId = m_targetId;

            RecalculateTransform();
        }
        else
        {
            // If the target is invalid (nothing to look at), stop listening to everything
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
        }
    }

    //=========================================================================
    void EditorLookAtComponent::OnEnabledChanged()
    {
        if (m_enabled)
        {
            RecalculateTransform();
        }
    }

    //=========================================================================
    void EditorLookAtComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        RecalculateTransform();
        AZ::TickBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void EditorLookAtComponent::RecalculateTransform()
    {
        if (!m_enabled)
        {
            return;
        }

        if (m_targetId.IsValid())
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(GetEntityId());
            {
                AZ::Transform sourceTM = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(sourceTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

                AZ::Transform targetTM = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(targetTM, m_targetId, &AZ::TransformBus::Events::GetWorldTM);

                AZ::Transform lookAtTransform = AZ::Transform::CreateLookAt(
                    sourceTM.GetTranslation(),
                    targetTM.GetTranslation(),
                    m_forwardAxis
                    );

                if (!m_fixatePitch)
                {
                    lookAtTransform.SetFromEulerRadians(AZ::Vector3(sourceTM.GetRotation().GetEulerRadians().GetX(), lookAtTransform.GetRotation().GetEulerRadians().GetY(), lookAtTransform.GetRotation().GetEulerRadians().GetZ()));
                }
                if (!m_fixateRoll)
                {
                    lookAtTransform.SetFromEulerRadians(AZ::Vector3(lookAtTransform.GetRotation().GetEulerRadians().GetX(), sourceTM.GetRotation().GetEulerRadians().GetY(), lookAtTransform.GetRotation().GetEulerRadians().GetZ()));
                }
                if (!m_fixateYaw)
                {
                    lookAtTransform.SetFromEulerRadians(AZ::Vector3(lookAtTransform.GetRotation().GetEulerRadians().GetX(), lookAtTransform.GetRotation().GetEulerRadians().GetY(), sourceTM.GetRotation().GetEulerRadians().GetZ()));
                }

                lookAtTransform.SetTranslation(sourceTM.GetTranslation());

                // When strength is maxed out, update the rotation and translation for sourceTM based on lookAtTransform, but leave scale unchanged
                if (m_strength == 1.f && m_applyLookAtTransform)
                {
                    sourceTM.SetRotation(lookAtTransform.GetRotation());
                    sourceTM.SetTranslation(lookAtTransform.GetTranslation());

                    AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, lookAtTransform);
                    AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, sourceTM);
                }
            }
            AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
        }
    }

}//namespace LmbrCentral
