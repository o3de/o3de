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

#include <CoreLights/EditorSpotLightComponent.h>
#include <Atom/RPI.Edit/Common/ColorUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <CoreLights/SpotLightComponent.h>

namespace AZ
{
    namespace Render
    {
        EditorSpotLightComponent::EditorSpotLightComponent(const SpotLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorSpotLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorSpotLightComponent, BaseClass>()
                    ->Version(3, ConvertToEditorRenderComponentAdapter<2>);

                if (AZ::EditContext * editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorSpotLightComponent>(
                        "Spot Light", "A spot light emits light in a cone from a single point in space.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-spot-light.html")
                        ;

                    editContext->Class<SpotLightComponentController>(
                        "SpotLightComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpotLightComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<SpotLightComponentConfig>(
                        "SpotLightComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &SpotLightComponentConfig::m_color, "Color", "Color of the light")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute("ColorEditorConfiguration", AZ::RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(Edit::UIHandlers::ComboBox, &SpotLightComponentConfig::m_intensityMode, "Intensity Mode",
                            "Allows specifying light values in candelas or lumens")
                            ->EnumAttribute(PhotometricUnit::Candela, "Candela")
                            ->EnumAttribute(PhotometricUnit::Lumen, "Lumen")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpotLightComponentConfig::m_intensity, "Intensity", "Intensity of the light")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, &SpotLightComponentConfig::GetIntensitySuffix)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SpotLightComponentConfig::m_bulbRadius, "Bulb Radius",
                            "Radius of the disk that represents the spot light bulb.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 0.25f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Cone Configuration")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SpotLightComponentConfig::m_innerConeDegrees, "Inner Cone Angle",
                            "Angle from the direction axis at which this light starts to fall off.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Max, &SpotLightComponentConfig::GetConeDegrees)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SpotLightComponentConfig::m_outerConeDegrees, "Outer Cone Angle",
                            "Angle from the direction axis at which this light no longer has an effect.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Max, &SpotLightComponentConfig::GetConeDegrees)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SpotLightComponentConfig::m_penumbraBias, "Penumbra Bias",
                            "Controls biasing the fall off curve of the penumbra towards the inner or outer cone angles.")
                            ->Attribute(AZ::Edit::Attributes::Min, -1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Attenuation Radius")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SpotLightComponentConfig::m_attenuationRadiusMode, "Mode",
                            "Controls whether the attenuation radius is calculated automatically or set explicitly.")
                            ->EnumAttribute(LightAttenuationRadiusMode::Automatic, "Automatic")
                            ->EnumAttribute(LightAttenuationRadiusMode::Explicit, "Explicit")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpotLightComponentConfig::m_attenuationRadius, "Radius",
                            "The distance at which this light no longer has an affect.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &SpotLightComponentConfig::IsAttenuationRadiusModeAutomatic)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Shadow")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Default, &SpotLightComponentConfig::m_enabledShadow, "Enable Shadow", "Enable shadow for the light")
                        ->DataElement(Edit::UIHandlers::ComboBox, &SpotLightComponentConfig::m_shadowmapSize, "Shadowmap Size", "Width/Height of shadowmap")
                            ->EnumAttribute(ShadowmapSize::Size256, " 256")
                            ->EnumAttribute(ShadowmapSize::Size512, " 512")
                            ->EnumAttribute(ShadowmapSize::Size1024, "1024")
                            ->EnumAttribute(ShadowmapSize::Size2048, "2048")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::ComboBox, &SpotLightComponentConfig::m_shadowFilterMethod, "Shadow Filter Method",
                            "Filtering method of edge-softening of shadows.\n"
                            "  None: no filtering\n"
                            "  PCF: Percentage-Closer Filtering\n"
                            "  ESM: Exponential Shadow Maps\n"
                            "  ESM+PCF: ESM with a PCF fallback\n"
                            "For BehaviorContext (or TrackView), None=0, PCF=1, ESM=2, ESM+PCF=3")
                            ->EnumAttribute(ShadowFilterMethod::None, "None")
                            ->EnumAttribute(ShadowFilterMethod::Pcf, "PCF")
                            ->EnumAttribute(ShadowFilterMethod::Esm, "ESM")
                            ->EnumAttribute(ShadowFilterMethod::EsmPcf, "ESM+PCF")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::Slider, &SpotLightComponentConfig::m_boundaryWidthInDegrees, "Softening Boundary Width",
                            "Width of the boundary between shadowed area and lit one. "
                            "Units are in degrees. "
                            "If this is 0, softening edge is disabled.")
                            ->Attribute(Edit::Attributes::Min, 0.f)
                            ->Attribute(Edit::Attributes::Max, 1.f)
                            ->Attribute(Edit::Attributes::Suffix, " deg")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::ReadOnly, &SpotLightComponentConfig::IsPcfBoundarySearchDisabled)
                        ->DataElement(Edit::UIHandlers::Slider, &SpotLightComponentConfig::m_predictionSampleCount, "Prediction Sample Count",
                            "Sample Count for prediction of whether the pixel is on the boundary. Specific to PCF and ESM+PCF.")
                            ->Attribute(Edit::Attributes::Min, 4)
                            ->Attribute(Edit::Attributes::Max, 16)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::ReadOnly, &SpotLightComponentConfig::IsPcfBoundarySearchDisabled)
                        ->DataElement(Edit::UIHandlers::Slider, &SpotLightComponentConfig::m_filteringSampleCount, "Filtering Sample Count",
                            "It is used only when the pixel is predicted to be on the boundary. Specific to PCF and ESM+PCF.")
                            ->Attribute(Edit::Attributes::Min, 4)
                            ->Attribute(Edit::Attributes::Max, 64)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::ReadOnly, &SpotLightComponentConfig::IsShadowPcfDisabled)
                        ->DataElement(
                            Edit::UIHandlers::ComboBox, &SpotLightComponentConfig::m_pcfMethod, "Pcf Method",
                            "Type of Pcf to use.\n"
                            "  Boundary search: do several taps to first determine if we are on a shadow boundary\n"
                            "  Bicubic: a smooth, fixed-size kernel \n")
                        ->EnumAttribute(PcfMethod::BoundarySearch, "Boundary Search")
                        ->EnumAttribute(PcfMethod::Bicubic, "Bicubic")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &SpotLightComponentConfig::IsShadowPcfDisabled);
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorSpotLightComponent>()->RequestBus("SpotLightRequestBus");

                behaviorContext->ConstantProperty("EditorSpotLightComponentTypeId", BehaviorConstant(Uuid(EditorSpotLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        void EditorSpotLightComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        }

        void EditorSpotLightComponent::Deactivate()
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorSpotLightComponent::DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            const AZ::Vector3 position = worldTM.GetTranslation();

            debugDisplay.SetColor(m_controller.GetColor());

            // Draw a sphere for the light itself.
            const float pixelRadius = 10.0f;
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            const float distance = cameraState.m_position.GetDistance(position);
            const float screenScale = (distance * cameraState.m_fovOrZoom) / cameraState.m_viewportSize.GetX();
            debugDisplay.DrawWireSphere(position, screenScale * pixelRadius);

            // Don't draw extra visualization unless selected.
            if (!IsSelected())
            {
                return;
            }

            /*
             * Draw rays to show the affected volume of the spot light.
             * As well as two circle showing inner and outer cone angles.
             * Note: the circles will not be drawn if cone are angles go beyond 90 degrees.
             */
            worldTM.ExtractScale();
            debugDisplay.PushMatrix(worldTM);
            debugDisplay.SetColor(m_controller.GetColor());

            float outerConeHalfAngle = m_controller.GetOuterConeAngleInDegrees() * 0.5f;
            float innerConeHalfAngle = m_controller.GetInnerConeAngleInDegrees() * 0.5f;
            const float debugConeHeight = m_controller.GetAttenuationRadius();

            const float debugOuterConeRadius = tanf(AZ::DegToRad(outerConeHalfAngle)) * debugConeHeight;
            debugDisplay.DrawArrow(Vector3::CreateZero(), Vector3::CreateAxisY() * debugConeHeight * 0.5f, debugConeHeight * 0.2f);

            constexpr float rightAngleInDegrees = 90.0f;
            if (outerConeHalfAngle < rightAngleInDegrees)
            {
                // outer cone
                debugDisplay.DrawCircle(Vector3::CreateAxisY() * debugConeHeight, debugOuterConeRadius, 1);
            }
            if (innerConeHalfAngle < rightAngleInDegrees)
            {
                // inner cone
                const float debugInnerConeRadius = tanf(AZ::DegToRad(innerConeHalfAngle)) * debugConeHeight;
                debugDisplay.DrawCircle(Vector3::CreateAxisY() * debugConeHeight, debugInnerConeRadius, 1);
            }

            constexpr int debugRays = 6;
            for (int rayIndex = 0; rayIndex < debugRays; ++rayIndex)
            {
                const float angle = (AZ::Constants::TwoPi / debugRays) * rayIndex;
                const Vector3 spotRay = Vector3::CreateAxisY() * debugConeHeight + debugOuterConeRadius * Vector3(sinf(angle), 0.f, cosf(angle));
                debugDisplay.DrawLine(Vector3::CreateZero(), spotRay * (outerConeHalfAngle > rightAngleInDegrees ? -1.f : 1.f));
            }

            debugDisplay.PopMatrix();
        }

        u32 EditorSpotLightComponent::OnConfigurationChanged()
        {
            // Set the intenstiy of the photometric unit in case the controller is disabled. This is needed to correctly convert between photometric units.
            m_controller.m_photometricValue.SetIntensity(m_controller.m_configuration.m_intensity);

            // If the intensity mode changes in the editor, convert the photometric value and update the intensity
            if (m_controller.m_configuration.m_intensityMode != m_controller.m_photometricValue.GetType())
            {
                m_controller.m_photometricValue.ConvertToPhotometricUnit(m_controller.m_configuration.m_intensityMode);
                m_controller.m_configuration.m_intensity = m_controller.m_photometricValue.GetIntensity();
            }

            BaseClass::OnConfigurationChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
