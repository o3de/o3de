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

#include <CoreLights/EditorPointLightComponent.h>

#include <Atom/RPI.Edit/Common/ColorUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzCore/Math/IntersectSegment.h>

namespace AZ
{
    namespace Render
    {
        EditorPointLightComponent::EditorPointLightComponent(const PointLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorPointLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPointLightComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorPointLightComponent>(
                        "Point Light", "A point light emits light in all directions from a single point in space.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-point-light.html")
                        ;

                    editContext->Class<PointLightComponentController>(
                        "PointLightComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PointLightComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<PointLightComponentConfig>(
                        "PointLightComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &PointLightComponentConfig::m_color, "Color", "Color of the light")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute("ColorEditorConfiguration", AZ::RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(Edit::UIHandlers::ComboBox, &PointLightComponentConfig::m_intensityMode, "Intensity Mode", "Allows specifying light values in candelas or lumens")
                            ->EnumAttribute(PhotometricUnit::Candela, "Candela")
                            ->EnumAttribute(PhotometricUnit::Lumen, "Lumen")
                            ->EnumAttribute(PhotometricUnit::Nit, "Nit")
                            ->EnumAttribute(PhotometricUnit::Ev100Luminance, "Ev100")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PointLightComponentConfig::m_intensity, "Intensity", "Intensity of the light")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, &PointLightComponentConfig::GetIntensitySuffix)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &PointLightComponentConfig::m_bulbRadius, "Bulb Radius", "The size of the bulb in meters")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Attenuation Radius")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &PointLightComponentConfig::m_attenuationRadiusMode, "Mode", "Controls whether the attenation radius is calculated automatically or set explicitly.")
                            ->EnumAttribute(LightAttenuationRadiusMode::Automatic, "Automatic")
                            ->EnumAttribute(LightAttenuationRadiusMode::Explicit, "Explicit")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PointLightComponentConfig::m_attenuationRadius, "Radius", "The distance at which this light no longer has an affect.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &PointLightComponentConfig::IsAttenuationRadiusModeAutomatic)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorPointLightComponent>()->RequestBus("PointLightRequestBus");

                behaviorContext->ConstantProperty("EditorPointLightComponentTypeId", BehaviorConstant(Uuid(EditorPointLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        void EditorPointLightComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
            TransformNotificationBus::Handler::BusConnect(GetEntityId());
            AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorPointLightComponent::Deactivate()
        {
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        AZStd::tuple<float, Vector3> EditorPointLightComponent::GetRadiusAndPosition() const
        {
            Vector3 position = Vector3::CreateZero();
            AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
            return AZStd::tuple<float, Vector3>(m_controller.GetBulbRadius(), position);
        }

        AZStd::tuple<float, Vector3> EditorPointLightComponent::GetViewportRadiusAndPosition(const AzFramework::ViewportInfo& viewportInfo) const
        {
            const auto [radius, position] = GetRadiusAndPosition();

            constexpr float PixelRadius = 10.0f;
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            const float distance = cameraState.m_position.GetDistance(position);
            const float screenScale = (distance * cameraState.m_fovOrZoom) / cameraState.m_viewportSize.GetX();

            return AZStd::tuple<float, Vector3>(AZ::GetMax(radius, screenScale * PixelRadius), position);
        }

        void EditorPointLightComponent::DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            debugDisplay.SetColor(m_controller.GetColor());

            // Draw a sphere for the light itself.
            auto [sphereSize, position] = GetViewportRadiusAndPosition(viewportInfo);
            debugDisplay.DrawWireSphere(position, sphereSize);

            // Don't draw extra visualization unless selected.
            if (!IsSelected())
            {
                return;
            }

            // Draw a sphere for the attenuation radius
            debugDisplay.DrawWireSphere(position, m_controller.GetAttenuationRadius());
        }

        AZ::Aabb EditorPointLightComponent::GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo)
        {
            auto [radius, position] = GetViewportRadiusAndPosition(viewportInfo);
            return Aabb::CreateCenterRadius(position, radius);
        }

        bool EditorPointLightComponent::EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
        {
            auto [radius, position] = GetViewportRadiusAndPosition(viewportInfo);
            return AZ::Intersect::IntersectRaySphere(src, dir, position, radius, distance) > 0;
        }

        void EditorPointLightComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
        {
            // Transform scale impacts the bulb radius and intensity of the light, so refresh the values.
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_Values);
        }

        AZ::Aabb EditorPointLightComponent::GetWorldBounds()
        {
            auto [radius, position] = GetRadiusAndPosition();
            return Aabb::CreateCenterRadius(position, radius);
        }

        AZ::Aabb EditorPointLightComponent::GetLocalBounds()
        {
            return Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), m_controller.GetBulbRadius());
        }

        u32 EditorPointLightComponent::OnConfigurationChanged()
        {
            // Set the intenstiy of the photometric unit in case the controller is disabled. This is needed to correctly convert between photometric units.
            m_controller.m_photometricValue.SetIntensity(m_controller.m_configuration.m_intensity);

            // If the intensity mode changes in the editor, convert the photometric value and update the intensity
            if (m_controller.m_configuration.m_intensityMode != m_controller.m_photometricValue.GetType())
            {
                m_controller.m_photometricValue.SetArea(m_controller.m_configuration.GetArea());
                m_controller.m_photometricValue.ConvertToPhotometricUnit(m_controller.m_configuration.m_intensityMode);
                m_controller.m_configuration.m_intensity = m_controller.m_photometricValue.GetIntensity();
            }

            BaseClass::OnConfigurationChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
