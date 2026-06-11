/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientGI/EditorGradientGIComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        // =====================================================================
        // Reflect
        // =====================================================================

        void EditorGradientGIComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorGradientGIComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>)
                    ;

                if (auto* editContext = serializeContext->GetEditContext())
                {
                    // =========================================================
                    // Editor Component UI
                    // =========================================================

                    editContext->Class<EditorGradientGIComponent>(
                        "Gradient GI (IBL)", "Procedural gradient cubemap for ambient image-based lighting (IBL)")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;

                    // =========================================================
                    // Controller Configuration UI
                    // =========================================================

                    editContext->Class<GradientGIComponentController>(
                        "GradientGIComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GradientGIComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    // =========================================================
                    // Config Fields UI
                    // =========================================================

                    editContext->Class<GradientGIComponentConfig>(
                        "GradientGIComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        // -- Gradient Colors group --
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Gradient Colors")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &GradientGIComponentConfig::m_highColor, "High (Zenith)", "Color at the top of the sky gradient")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &GradientGIComponentConfig::m_midColor, "Mid (Horizon)", "Color at the horizon of the sky gradient")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &GradientGIComponentConfig::m_lowColor, "Low (Nadir)", "Color at the bottom of the sky gradient")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        // -- Settings group --
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Settings")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientGIComponentConfig::m_exposure, "Exposure", "IBL exposure in EV stops. Drag fully left to dim the ambient fill to black.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax,  5.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max,  20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientGIComponentConfig::m_faceResolution, "Resolution", "Cubemap face size in pixels (4-256). Higher values give sharper ambient detail at greater cost.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::Min, 4u)
                            ->Attribute(AZ::Edit::Attributes::Max, 256u)

                        // -- Update Mode group --
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GradientGIComponentConfig::m_updateMode,
                            "Update Mode",
                            "CPU: CPU-generated StreamingImage (mobile-safe, updates on change).\n"
                            "GPU: GPU compute pass writing every frame (falls back to CPU if unsupported).")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->EnumAttribute(GradientGIUpdateMode::Static,  "CPU")
                            ->EnumAttribute(GradientGIUpdateMode::Dynamic, "GPU")
                        ;
                }
            }

            if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorGradientGIComponent>()->RequestBus("GradientGIComponentRequestBus");

                behaviorContext->ConstantProperty("EditorGradientGIComponentTypeId",
                    BehaviorConstant(Uuid(EditorGradientGIComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        // =====================================================================
        // Constructor
        // =====================================================================

        EditorGradientGIComponent::EditorGradientGIComponent(const GradientGIComponentConfig& config)
            : BaseClass(config)
        {
        }

        // =====================================================================
        // ChangeNotify Handlers
        // =====================================================================

        AZ::u32 EditorGradientGIComponent::OnColorChanged()
        {
            m_controller.UpdateColors();
            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        AZ::u32 EditorGradientGIComponent::OnExposureChanged()
        {
            m_controller.SetExposure(m_controller.m_configuration.m_exposure);
            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        AZ::u32 EditorGradientGIComponent::OnResolutionChanged()
        {
            m_controller.SetFaceResolution(static_cast<int>(m_controller.m_configuration.m_faceResolution));
            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        AZ::u32 EditorGradientGIComponent::OnUpdateModeChanged()
        {
            m_controller.SetUpdateMode(m_controller.m_configuration.m_updateMode);
            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

    } // namespace Render
} // namespace AZ
