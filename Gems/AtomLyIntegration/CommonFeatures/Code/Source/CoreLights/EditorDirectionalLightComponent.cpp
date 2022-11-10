/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/EditorDirectionalLightComponent.h>

#include <Atom/RPI.Edit/Common/ColorUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <Atom/Feature/CoreLights/PhotometricValue.h>

namespace AZ
{
    namespace Render
    {
        EditorDirectionalLightComponent::EditorDirectionalLightComponent(const DirectionalLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorDirectionalLightComponent::Reflect(ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EditorDirectionalLightComponent, BaseClass>()
                    ->Version(3, ConvertToEditorRenderComponentAdapter<2>);

                if (EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDirectionalLightComponent>(
                        "Directional Light", "A directional light to cast a shadow of meshes onto meshes.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/Lighting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO][ATOM-1998] create icons.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO][ATOM-1998] create icons.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/directional-light/") // [GFX TODO][ATOM-1998] create page
                        ;

                    editContext->Class<DirectionalLightComponentController>(
                        "DirectionalLightComponentController", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Default, &DirectionalLightComponentController::m_configuration, "Configuration", "")
                        ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<DirectionalLightComponentConfig>("DirectionalLightComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(Edit::UIHandlers::Color, &DirectionalLightComponentConfig::m_color, "Color", "Color of the light")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute("ColorEditorConfiguration", AZ::RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(Edit::UIHandlers::ComboBox, &DirectionalLightComponentConfig::m_intensityMode, "Intensity mode", "Allows specifying light values in lux or Ev100")
                            ->EnumAttribute(PhotometricUnit::Lux, "Lux")
                            ->EnumAttribute(PhotometricUnit::Ev100Illuminance, "Ev100")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_intensity, "Intensity", "Intensity of the light in the set photometric unit.")
                            ->Attribute(Edit::Attributes::Min, &DirectionalLightComponentConfig::GetIntensityMin)
                            ->Attribute(Edit::Attributes::Max, &DirectionalLightComponentConfig::GetIntensityMax)
                            ->Attribute(Edit::Attributes::SoftMin, &DirectionalLightComponentConfig::GetIntensitySoftMin)
                            ->Attribute(Edit::Attributes::SoftMax, &DirectionalLightComponentConfig::GetIntensitySoftMax)
                            ->Attribute(Edit::Attributes::Suffix, &DirectionalLightComponentConfig::GetIntensitySuffix)
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_angularDiameter, "Angular diameter", "Angular diameter of the directional light in degrees. The sun is about 0.5.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 5.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 1.0f)
                            ->Attribute(Edit::Attributes::Suffix, " deg")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Shadow")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::EntityId, &DirectionalLightComponentConfig::m_cameraEntityId, "Camera", "Entity of the camera for cascaded shadowmap view frustum.")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::Default, &DirectionalLightComponentConfig::m_shadowFarClipDistance, "Shadow far clip", "Shadow specific far clip distance.")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::ComboBox, &DirectionalLightComponentConfig::m_shadowmapSize, "Shadowmap size", "Width/Height of shadowmap")
                            ->EnumAttribute(ShadowmapSize::Size256, " 256")
                            ->EnumAttribute(ShadowmapSize::Size512, " 512")
                            ->EnumAttribute(ShadowmapSize::Size1024, "1024")
                            ->EnumAttribute(ShadowmapSize::Size2048, "2048")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_cascadeCount, "Cascade count", "Number of cascades")
                            ->Attribute(Edit::Attributes::Min, 1)
                            ->Attribute(Edit::Attributes::Max, Shadow::MaxNumberOfCascades)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::Default, &DirectionalLightComponentConfig::m_isShadowmapFrustumSplitAutomatic, "Automatic splitting",
                            "Switch splitting of shadowmap frustum to cascades automatically or not.")
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_shadowmapFrustumSplitSchemeRatio, "Split ratio",
                            "Ratio to lerp between the two types of frustum splitting scheme.\n"
                            "0 = Uniform scheme which will split the frustum evenly across all cascades.\n"
                            "1 = Logarithmic scheme which is designed to split the frustum in a logarithmic fashion "
                            "in order to enable us to produce a more optimal perspective aliasing across the frustum.")
                            ->Attribute(Edit::Attributes::Min, 0.f)
                            ->Attribute(Edit::Attributes::Max, 1.f)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::ReadOnly, &DirectionalLightComponentConfig::IsSplitManual)
                        ->DataElement(Edit::UIHandlers::Vector4, &DirectionalLightComponentConfig::m_cascadeFarDepths, "Far depth cascade",
                            "Far depth of each cascade.  The value of the index greater than or equal to cascade count is ignored.")
                            ->Attribute(Edit::Attributes::Min, 0.01f)
                            ->Attribute(Edit::Attributes::Max, &DirectionalLightComponentConfig::m_shadowFarClipDistance)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::ReadOnly, &DirectionalLightComponentConfig::IsSplitAutomatic)
                        ->DataElement(Edit::UIHandlers::Default, &DirectionalLightComponentConfig::m_groundHeight, "Ground height",
                            "Height of the ground. Used to correct position of cascades.")
                            ->Attribute(Edit::Attributes::Suffix, " m")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::ReadOnly, &DirectionalLightComponentConfig::IsCascadeCorrectionDisabled)
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DirectionalLightComponentConfig::m_isCascadeCorrectionEnabled, "Cascade correction",
                            "Enable position correction of cascades to optimize the appearance for certain camera positions.")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DirectionalLightComponentConfig::m_isDebugColoringEnabled, "Debug coloring",
                            "Enable coloring to see how cascades places 0:red, 1:green, 2:blue, 3:yellow.")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::ComboBox, &DirectionalLightComponentConfig::m_shadowFilterMethod, "Shadow filter method",
                            "Filtering method of edge-softening of shadows.\n"
                            "  None: No filtering\n"
                            "  PCF: Percentage-closer filtering\n"
                            "  ESM: Exponential shadow maps\n"
                            "  ESM+PCF: ESM with a PCF fallback\n"
                            "For BehaviorContext (or TrackView), None=0, PCF=1, ESM=2, ESM+PCF=3")
                            ->EnumAttribute(ShadowFilterMethod::None, "None")
                            ->EnumAttribute(ShadowFilterMethod::Pcf, "PCF")
                            ->EnumAttribute(ShadowFilterMethod::Esm, "ESM")
                            ->EnumAttribute(ShadowFilterMethod::EsmPcf, "ESM+PCF")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_filteringSampleCount, "Filtering sample count",
                            "This is used only when the pixel is predicted to be on the boundary.\n"
                            "Specific to PCF and ESM+PCF.")
                            ->Attribute(Edit::Attributes::Min, 4)
                            ->Attribute(Edit::Attributes::Max, 64)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::ReadOnly, &DirectionalLightComponentConfig::IsShadowPcfDisabled)
                        ->DataElement(
                            Edit::UIHandlers::CheckBox, &DirectionalLightComponentConfig::m_receiverPlaneBiasEnabled,
                            "Shadow Receiver Plane Bias Enable",
                            "This reduces shadow acne when using large pcf kernels.")
                          ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                          ->Attribute(Edit::Attributes::ReadOnly, &DirectionalLightComponentConfig::IsShadowPcfDisabled) 
                         ->DataElement(
                            Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_shadowBias,
                            "Shadow Bias",
                            "Reduces acne by applying a fixed bias along z in shadow-space.\n"
                            "If this is 0, no biasing is applied.") 
                           ->Attribute(Edit::Attributes::Min, 0.f)
                           ->Attribute(Edit::Attributes::Max, 0.2) 
                           ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(
                           Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_normalShadowBias, "Normal Shadow Bias",
                           "Reduces acne by biasing the shadowmap lookup along the geometric normal.\n"
                           "If this is 0, no biasing is applied.")
                           ->Attribute(Edit::Attributes::Min, 0.f)
                           ->Attribute(Edit::Attributes::Max, 10.0f)
                           ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(
                            Edit::UIHandlers::CheckBox, &DirectionalLightComponentConfig::m_cascadeBlendingEnabled,
                            "Blend between cascades", "Enables smooth blending between shadow map cascades.")
                        ->Attribute(Edit::Attributes::ReadOnly, &DirectionalLightComponentConfig::IsShadowPcfDisabled)

                        // Fullscreen Shadow Blur...
                        ->DataElement(Edit::UIHandlers::CheckBox, &DirectionalLightComponentConfig::m_fullscreenBlurEnabled,
                            "Fullscreen Blur",
                            "Enables fullscreen blur on fullscreen sunlight shadows.")
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_fullscreenBlurConstFalloff,
                                "Fullscreen Blur Strength",
                                "Affects how strong the fullscreen shadow blur is. Recommended value is 0.67")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 0.95f)
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_fullscreenBlurDepthFalloffStrength,
                                "Fullscreen Blur Sharpness",
                                "Affects how sharp the fullscreen shadow blur appears around edges. Recommended value is 50")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 400.0f)

                        ->ClassElement(Edit::ClassElements::Group, "Global Illumination")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::CheckBox, &DirectionalLightComponentConfig::m_affectsGI, "Affects GI", "Controls whether this light affects diffuse global illumination.")
                        ->DataElement(Edit::UIHandlers::Slider, &DirectionalLightComponentConfig::m_affectsGIFactor, "Factor", "Multiplier on the amount of contribution to diffuse global illumination.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 2.0f)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorDirectionalLightComponent>()->RequestBus("DirectionalLightRequestBus");

                behaviorContext->ConstantProperty("EditorDirectionalLightComponentTypeId", BehaviorConstant(Uuid(EditorDirectionalLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

            }
        }

        void EditorDirectionalLightComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        }

        void EditorDirectionalLightComponent::Deactivate()
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorDirectionalLightComponent::DisplayEntityViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            static const auto forward = Vector3::CreateAxisY();
            static const auto up = Vector3::CreateAxisZ();
            static const auto right = Vector3::CreateAxisX();
            static const float arrowScale = 0.5f;
            static const float arrowLength = 1.5f;
            static const float arrowOffset = 0.75f;
            static const float originScale = 0.2f;
            static const float originScale0 = 0.1f;
            static const float originScale1 = 0.05f;

            auto transform = Transform::CreateIdentity();
            TransformBus::EventResult(
                transform,
                GetEntityId(),
                &TransformBus::Events::GetWorldTM);

            transform.ExtractUniformScale();
            const Vector3 origin = transform.GetTranslation();
            const Vector3 originOffset = origin - (transform.TransformVector(forward) * arrowOffset);
            const Vector3 target = origin - (transform.TransformVector(forward) * (arrowLength + arrowOffset));

            const Vector3 originPozZ = originOffset + (transform.TransformVector(up) * (originScale + originScale1));
            const Vector3 targetPosZ = target + (transform.TransformVector(up) * (originScale + originScale1));
            const Vector3 originPosX = originOffset + (transform.TransformVector(right) * (originScale + originScale1));
            const Vector3 targetPosX = target + (transform.TransformVector(right) * (originScale + originScale1));
            const Vector3 originNegX = originOffset - (transform.TransformVector(right) * (originScale + originScale1));
            const Vector3 targetNegX = target - (transform.TransformVector(right) * (originScale + originScale1));
            const Vector3 originNegZ = originOffset - (transform.TransformVector(up) * (originScale + originScale1));
            const Vector3 targetNegZ = target - (transform.TransformVector(up) * (originScale + originScale1));

            debugDisplay.SetColor(m_controller.GetColor());
            debugDisplay.DrawWireDisk(origin, transform.TransformVector(-forward), originScale + originScale0);
            debugDisplay.DrawArrow(targetPosZ, originPozZ, arrowScale);
            debugDisplay.DrawArrow(targetNegZ, originNegZ, arrowScale);
            debugDisplay.DrawArrow(targetPosX, originPosX, arrowScale);
            debugDisplay.DrawArrow(targetNegX, originNegX, arrowScale);
            debugDisplay.DrawDisk(origin, transform.TransformVector(forward), originScale);
        }

        u32 EditorDirectionalLightComponent::OnConfigurationChanged()
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
