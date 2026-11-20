/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientGlobalIllumination/EditorGradientGlobalIlluminationComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::Render
{
    void EditorGradientGlobalIlluminationComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGradientGlobalIlluminationComponent, BaseClass>()->Version(1, ConvertToEditorRenderComponentAdapter<1>);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorGradientGlobalIlluminationComponent>(
                        "Gradient Global Illumination (IBL)",
                        "Procedural gradient cubemap for ambient image-based lighting (IBL)")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<GradientGlobalIlluminationComponentController>("GradientGlobalIlluminationComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientGlobalIlluminationComponentController::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext->Class<GradientGlobalIlluminationComponentConfig>("GradientGlobalIlluminationComponentConfig", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Gradient Colors")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color,
                        &GradientGlobalIlluminationComponentConfig::m_highColor,
                        "High (Zenith)",
                        "Color at the top of the sky gradient")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color,
                        &GradientGlobalIlluminationComponentConfig::m_midColor,
                        "Mid (Horizon)",
                        "Color at the horizon of the sky gradient")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color,
                        &GradientGlobalIlluminationComponentConfig::m_lowColor,
                        "Low (Nadir)",
                        "Color at the bottom of the sky gradient")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Settings")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &GradientGlobalIlluminationComponentConfig::m_exposure,
                        "Exposure",
                        "IBL exposure in EV stops. Drag fully left to dim the ambient fill to black.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -20.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                    ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &GradientGlobalIlluminationComponentConfig::m_faceResolution,
                        "Resolution",
                        "Cubemap face size in pixels (4-256). Higher values give sharper ambient detail at greater cost. "
                        "In CPU mode the value snaps to the nearest power of two on release.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Min, 4u)
                    ->Attribute(AZ::Edit::Attributes::Max, 256u)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &GradientGlobalIlluminationComponentConfig::m_updateMode,
                        "Update Mode",
                        "CPU: Rebuild the gradient when inputs change. Texture layers are unavailable.\n"
                        "GPU: Composite the gradient and texture layers on change. Falls back to CPU if unavailable.")
                    // The GPU-only texture controls must update when the mode changes.
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->EnumAttribute(GradientGlobalIlluminationUpdateMode::Static, "CPU")
                    ->EnumAttribute(GradientGlobalIlluminationUpdateMode::Dynamic, "GPU")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Detail Texture")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &GradientGlobalIlluminationComponentConfig::m_detailTexture,
                        "Texture",
                        "Optional detail/grit texture overlaid on the gradient (GPU mode only).")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &GradientGlobalIlluminationComponentConfig::m_detailMapping,
                        "Mapping",
                        "Map a 2D texture per face or equirectangularly, or sample a cubemap by direction.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->EnumAttribute(GradientGlobalIlluminationTextureMapping::Tiled, "Tiled")
                    ->EnumAttribute(GradientGlobalIlluminationTextureMapping::Stretched, "Equirectangular")
                    ->EnumAttribute(GradientGlobalIlluminationTextureMapping::Cube, "Cubemap")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &GradientGlobalIlluminationComponentConfig::m_detailBlend,
                        "Blend",
                        "How the texture combines with the gradient color beneath it.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Multiply, "Multiply")
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Add, "Add")
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Screen, "Screen")
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Overlay, "Overlay")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &GradientGlobalIlluminationComponentConfig::m_detailStrength,
                        "Strength",
                        "Blend weight of the detail layer (0 = gradient only, 1 = full detail).")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Specular Texture")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &GradientGlobalIlluminationComponentConfig::m_specularTexture,
                        "Texture",
                        "Optional specular texture that composites into the specular reflection only (GPU mode only).")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &GradientGlobalIlluminationComponentConfig::m_specularMapping,
                        "Mapping",
                        "Map a 2D texture per face or equirectangularly, or sample a cubemap by direction.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->EnumAttribute(GradientGlobalIlluminationTextureMapping::Tiled, "Tiled")
                    ->EnumAttribute(GradientGlobalIlluminationTextureMapping::Stretched, "Equirectangular")
                    ->EnumAttribute(GradientGlobalIlluminationTextureMapping::Cube, "Cubemap")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &GradientGlobalIlluminationComponentConfig::m_specularBlend,
                        "Blend",
                        "How the specular texture combines with the base color (Replace = use the texture alone).")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Multiply, "Multiply")
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Add, "Add")
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Screen, "Screen")
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Overlay, "Overlay")
                    ->EnumAttribute(GradientGlobalIlluminationBlendMode::Replace, "Replace")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &GradientGlobalIlluminationComponentConfig::m_specularStrength,
                        "Strength",
                        "Blend weight of the specular layer (0 = matches diffuse, 1 = full specular texture).")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientGlobalIlluminationComponentConfig::GetGpuLayerVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            }
        }

        if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorGradientGlobalIlluminationComponent>()->RequestBus("GradientGlobalIlluminationComponentRequestBus");

            behaviorContext
                ->ConstantProperty("EditorGradientGlobalIlluminationComponentTypeId", BehaviorConstant(Uuid(EditorGradientGlobalIlluminationComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

    EditorGradientGlobalIlluminationComponent::EditorGradientGlobalIlluminationComponent(const GradientGlobalIlluminationComponentConfig& config)
        : BaseClass(config)
    {
    }

    AZ::u32 EditorGradientGlobalIlluminationComponent::OnConfigurationChanged()
    {
        // Snap before the adapter reactivates the controller and refreshes the inspector.
        m_controller.SnapResolutionForCurrentMode();
        // Reactivation makes this component the newest scene owner.
        BaseClass::OnConfigurationChanged();
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }
} // namespace AZ::Render
