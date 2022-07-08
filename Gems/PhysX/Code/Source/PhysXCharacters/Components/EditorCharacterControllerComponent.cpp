/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/Components/EditorCharacterControllerComponent.h>
#include <PhysXCharacters/Components/CharacterControllerComponent.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

#include <System/PhysXSystem.h>

namespace PhysX
{
    // this epsilon is deliberately chosen to be somewhat larger than AZ::Constants::FloatEpsilon so that it does not vanish
    // when compared to the typical height of a character
    static const float HeightEpsilon = 1e-5f;

    void EditorCharacterControllerProxyShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorCharacterControllerProxyShapeConfig>()
                ->Version(1)
                ->Field("ShapeType", &EditorCharacterControllerProxyShapeConfig::m_shapeType)
                ->Field("Box", &EditorCharacterControllerProxyShapeConfig::m_box)
                ->Field("Capsule", &EditorCharacterControllerProxyShapeConfig::m_capsule)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCharacterControllerProxyShapeConfig>(
                    "EditorCharacterControllerProxyShapeConfig", "PhysX character controller shape.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorCharacterControllerProxyShapeConfig::m_shapeType, "Shape",
                        "The shape of the character controller.")
                    ->EnumAttribute(Physics::ShapeType::Capsule, "Capsule")
                    ->EnumAttribute(Physics::ShapeType::Box, "Box")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterControllerProxyShapeConfig::m_box, "Box",
                        "Configuration of box shape.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorCharacterControllerProxyShapeConfig::IsBoxConfig)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterControllerProxyShapeConfig::m_capsule, "Capsule",
                        "Configuration of capsule shape.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorCharacterControllerProxyShapeConfig::IsCapsuleConfig)
                    ;
            }
        }
    }

    bool EditorCharacterControllerProxyShapeConfig::IsBoxConfig() const
    {
        return m_shapeType == Physics::ShapeType::Box;
    }

    bool EditorCharacterControllerProxyShapeConfig::IsCapsuleConfig() const
    {
        return m_shapeType == Physics::ShapeType::Capsule;
    }

    const Physics::ShapeConfiguration& EditorCharacterControllerProxyShapeConfig::GetCurrent() const
    {
        switch (m_shapeType)
        {
        case Physics::ShapeType::Box:
            return m_box;
        case Physics::ShapeType::Capsule:
            return m_capsule;
        default:
            AZ_Warning("EditorCharacterControllerProxyShapeConfig", false, "Unsupported shape type.");
            return m_capsule;
        }
    }

    void EditorCharacterControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorCharacterControllerProxyShapeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorCharacterControllerComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorCharacterControllerComponent::m_configuration)
                ->Field("ShapeConfig", &EditorCharacterControllerComponent::m_proxyShapeConfiguration)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCharacterControllerComponent>(
                    "PhysX Character Controller",
                    "Provides basic character interactions with the physical world, such as preventing movement through other PhysX bodies.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/character-controller/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterControllerComponent::m_configuration,
                        "Configuration", "Configuration for the character controller.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCharacterControllerComponent::OnControllerConfigChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterControllerComponent::m_proxyShapeConfiguration,
                        "Shape Configuration", "The configuration for the shape associated with the character controller.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCharacterControllerComponent::OnShapeConfigChanged)
                    ;
            }
        }
    }

    EditorCharacterControllerComponent::EditorCharacterControllerComponent()
        : m_physXConfigChangedHandler(
            []([[maybe_unused]]const AzPhysics::SystemConfiguration* config)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                    AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
            })
    {

    }

    void EditorCharacterControllerComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
    }

    void EditorCharacterControllerComponent::Deactivate()
    {
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorCharacterControllerComponent::OnSelected()
    {
        if (auto* physXSystem = GetPhysXSystem())
        {
            if (!m_physXConfigChangedHandler.IsConnected())
            {
                physXSystem->RegisterSystemConfigurationChangedEvent(m_physXConfigChangedHandler);
            }
        }
    }

    void EditorCharacterControllerComponent::OnDeselected()
    {
        m_physXConfigChangedHandler.Disconnect();
    }

    // AzFramework::EntityDebugDisplayEventBus
    void EditorCharacterControllerComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (IsSelected())
        {
            const AZ::Vector3 upDirectionNormalized = m_configuration.m_upDirection.IsZero()
                ? AZ::Vector3::CreateAxisZ()
                : m_configuration.m_upDirection.GetNormalized();

            // PhysX uses the x-axis as the height direction of the controller, and so takes the shortest arc from the 
            // x-axis to the up direction.  To obtain the same orientation in the LY co-ordinate system (which uses z
            // as the height direction), we need to combine a rotation from the x-axis to the up direction with a 
            // rotation from the z-axis to the x-axis.
            const AZ::Quaternion upDirectionQuat = AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisX(),
                upDirectionNormalized) * AZ::Quaternion::CreateRotationY(AZ::Constants::HalfPi);

            if (m_proxyShapeConfiguration.IsCapsuleConfig())
            {
                const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(m_proxyShapeConfiguration.GetCurrent());
                const float heightOffset = 0.5f * capsuleConfig.m_height + m_configuration.m_contactOffset;
                const AZ::Transform controllerTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                    upDirectionQuat, GetWorldTM().GetTranslation() + heightOffset * upDirectionNormalized);

                debugDisplay.PushMatrix(controllerTransform);

                // draw the actual shape
                LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                    &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_radius,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_height,
                    16, 8,
                    m_vertexBuffer,
                    m_indexBuffer,
                    m_lineBuffer
                );

                debugDisplay.SetLineWidth(2.0f);
                debugDisplay.DrawTrianglesIndexed(m_vertexBuffer, m_indexBuffer, AzFramework::ViewportColors::SelectedColor);
                debugDisplay.DrawLines(m_lineBuffer, AzFramework::ViewportColors::WireColor);

                // draw the shape inflated by the contact offset
                LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                    &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_radius + m_configuration.m_contactOffset,
                    m_configuration.m_scaleCoefficient * capsuleConfig.m_height + 2.0f * m_configuration.m_contactOffset,
                    16, 8,
                    m_vertexBuffer,
                    m_indexBuffer,
                    m_lineBuffer
                );

                debugDisplay.DrawLines(m_lineBuffer, AzFramework::ViewportColors::WireColor);
                debugDisplay.PopMatrix();
            }

            if (m_proxyShapeConfiguration.IsBoxConfig())
            {
                const auto& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(m_proxyShapeConfiguration.GetCurrent());
                const float heightOffset = 0.5f * boxConfig.m_dimensions.GetZ() + m_configuration.m_contactOffset;
                const AZ::Transform controllerTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                    upDirectionQuat, GetWorldTM().GetTranslation() + heightOffset * upDirectionNormalized);

                const AZ::Vector3 boxHalfExtentsScaled = 0.5f * m_configuration.m_scaleCoefficient * boxConfig.m_dimensions;
                const AZ::Vector3 boxHalfExtentsScaledWithContactOffset = boxHalfExtentsScaled +
                    m_configuration.m_contactOffset * AZ::Vector3::CreateOne();

                debugDisplay.PushMatrix(controllerTransform);

                // draw the actual shape
                debugDisplay.SetLineWidth(2.0f);
                debugDisplay.SetColor(AzFramework::ViewportColors::SelectedColor);
                debugDisplay.DrawSolidBox(-boxHalfExtentsScaled, boxHalfExtentsScaled);
                debugDisplay.SetColor(AzFramework::ViewportColors::WireColor);
                debugDisplay.DrawWireBox(-boxHalfExtentsScaled, boxHalfExtentsScaled);

                // draw the shape inflated by the contact offset
                debugDisplay.DrawWireBox(-boxHalfExtentsScaledWithContactOffset, boxHalfExtentsScaledWithContactOffset);
                debugDisplay.PopMatrix();
            }
        }
    }

    // EditorComponentBase
    void EditorCharacterControllerComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        switch (m_proxyShapeConfiguration.m_shapeType)
        {
        case Physics::ShapeType::Box:
            gameEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::make_unique<CharacterControllerConfiguration>(m_configuration),
                AZStd::make_unique<Physics::BoxShapeConfiguration>(m_proxyShapeConfiguration.m_box));
            break;
        case Physics::ShapeType::Capsule:
            gameEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::make_unique<CharacterControllerConfiguration>(m_configuration),
                AZStd::make_unique<Physics::CapsuleShapeConfiguration>(m_proxyShapeConfiguration.m_capsule));
            break;
        }
    }

    // editor change notifications
    AZ::u32 EditorCharacterControllerComponent::OnControllerConfigChanged()
    {
        if (m_proxyShapeConfiguration.IsCapsuleConfig())
        {
            float capsuleHeight = m_proxyShapeConfiguration.m_capsule.m_height;
            m_configuration.m_stepHeight = AZ::GetClamp(m_configuration.m_stepHeight, 0.0f, capsuleHeight - HeightEpsilon);
        }
        else if (m_proxyShapeConfiguration.IsBoxConfig())
        {
            float boxHeight = m_proxyShapeConfiguration.m_box.m_dimensions.GetZ();
            m_configuration.m_stepHeight = AZ::GetClamp(m_configuration.m_stepHeight, 0.0f, boxHeight - HeightEpsilon);
        }

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::u32 EditorCharacterControllerComponent::OnShapeConfigChanged()
    {
        float minHeight = m_configuration.m_stepHeight + HeightEpsilon;

        if (m_proxyShapeConfiguration.IsCapsuleConfig())
        {
            m_proxyShapeConfiguration.m_capsule.m_height = AZ::GetMax(m_proxyShapeConfiguration.m_capsule.m_height, minHeight);
        }
        else if (m_proxyShapeConfiguration.IsBoxConfig())
        {
            float height = m_proxyShapeConfiguration.m_box.m_dimensions.GetZ();
            m_proxyShapeConfiguration.m_box.m_dimensions.SetZ(AZ::GetMax(height, minHeight));
        }

        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }
} // namespace PhysX
