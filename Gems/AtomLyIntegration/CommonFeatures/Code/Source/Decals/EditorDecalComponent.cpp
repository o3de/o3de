/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Decals/EditorDecalComponent.h>

#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzCore/Math/IntersectSegment.h>

namespace AZ
{
    namespace Render
    {
        EditorDecalComponent::EditorDecalComponent(const DecalComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorDecalComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDecalComponent, BaseClass>()
                    ->Version(2, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDecalComponent>(
                        "Decal", "The Decal component allows an entity to project a texture or material onto a mesh")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Decal.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Decal.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/decal/")
                    ;

                    editContext->Class<DecalComponentController>(
                        "DecalComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DecalComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                    editContext->Class<DecalComponentConfig>(
                        "DecalComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DecalComponentConfig::m_attenuationAngle, "Attenuation Angle", "Controls how much the angle between geometry and the decal affects decal opacity.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DecalComponentConfig::m_opacity, "Opacity", "The opacity of the decal.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DecalComponentConfig::m_sortKey, "Sort Key", "Decals with a larger sort key appear over top of smaller sort keys.")
                        ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<uint8_t>::min())
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<uint8_t>::max())

                        ->DataElement(AZ::Edit::UIHandlers::Default, &DecalComponentConfig::m_materialAsset, "Material", "The material of the decal.")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorDecalComponent>()->RequestBus("DecalRequestBus");

                behaviorContext->ConstantProperty("EditorDecalComponentTypeId", BehaviorConstant(Uuid(EditorDecalComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        void EditorDecalComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
            AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorDecalComponent::Deactivate()
        {
            AzFramework::BoundsRequestBus::Handler::BusDisconnect();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        AZ::Transform EditorDecalComponent::GetWorldTransform() const
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            return transform;
        }

        AZ::Matrix3x4 EditorDecalComponent::GetWorldTransformWithNonUniformScale() const
        {
            const AZ::Transform worldTransform = GetWorldTransform();
            const AZ::Matrix3x3 rotationMat = AZ::Matrix3x3::CreateFromQuaternion(worldTransform.GetRotation());

            const AZ::Vector3 nonUniformScale = m_controller.m_cachedNonUniformScale * worldTransform.GetUniformScale();
            const AZ::Matrix3x3 nonUniformScaleMat = AZ::Matrix3x3::CreateScale(nonUniformScale);
            const AZ::Matrix3x3 rotationAndScale = rotationMat * nonUniformScaleMat;

            return AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(rotationAndScale, worldTransform.GetTranslation());
        }

        void EditorDecalComponent::DisplayEntityViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (!IsSelected())
            {
                return;
            }

            debugDisplay.SetColor(AZ::Colors::Red);
            const AZ::Matrix3x4 transform = GetWorldTransformWithNonUniformScale();
            debugDisplay.PushPremultipliedMatrix(transform);
            debugDisplay.DrawWireBox(-AZ::Vector3::CreateOne(), AZ::Vector3::CreateOne());

            AZ::Vector3 x1 = AZ::Vector3(-1, 0, 1);
            AZ::Vector3 x2 = AZ::Vector3(1, 0, 1);
            AZ::Vector3 y1 = AZ::Vector3(0, -1, 1);
            AZ::Vector3 y2 = AZ::Vector3(0, 1, 1);

            debugDisplay.DrawLine(x1, x2);  // Draw horizontal line
            debugDisplay.DrawLine(y1, y2);  // Draw vertical line

            AZ::Vector3 p0 = AZ::Vector3(-1, -1, 1);
            AZ::Vector3 p1 = AZ::Vector3(-1, 1, 1);
            AZ::Vector3 p2 = AZ::Vector3(1, 1, 1);
            AZ::Vector3 p3 = AZ::Vector3(1, -1, 1);

            // Two diagonal edges
            debugDisplay.DrawLine(p0, p2);
            debugDisplay.DrawLine(p1, p3);
            debugDisplay.PopPremultipliedMatrix();
        }

        AZ::Aabb EditorDecalComponent::GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return GetWorldBounds();
        }

        bool EditorDecalComponent::EditorSelectionIntersectRayViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            AZ::Vector3 p0 = AZ::Vector3(-1, -1, 0);
            AZ::Vector3 p1 = AZ::Vector3(-1, 1, 0);
            AZ::Vector3 p2 = AZ::Vector3(1, 1, 0);
            AZ::Vector3 p3 = AZ::Vector3(1, -1, 0);
            float t{ 0.0f };

            bool hitResult = AZ::Intersect::IntersectRayQuad(src, dir, p0, p1, p2, p3, t) != 0;
            distance = t;

            return hitResult;
        }

        AZ::Aabb EditorDecalComponent::GetWorldBounds()
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            return GetLocalBounds().GetTransformedAabb(transform);
        }

        AZ::Aabb EditorDecalComponent::GetLocalBounds()
        {
            AZ::Aabb bbox = AZ::Aabb::CreateNull();
            bbox.AddPoint(AZ::Vector3(-1, -1, 0));
            bbox.AddPoint(AZ::Vector3(-1, 1, 0));
            bbox.AddPoint(AZ::Vector3(1, 1, 0));
            bbox.AddPoint(AZ::Vector3(1, -1, 0));
            return bbox;
        }

        u32 EditorDecalComponent::OnConfigurationChanged()
        {
            m_controller.ConfigurationChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        bool EditorDecalComponent::SupportsEditorRayIntersect()
        {
            return true;
        }
    } // namespace Render
} // namespace AZ
