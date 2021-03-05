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

#include <CoreLights/EditorAreaLightComponent.h>

#include <Atom/RPI.Edit/Common/ColorUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/EditorShapeComponentBus.h>

namespace AZ
{
    namespace Render
    {
        EditorAreaLightComponent::EditorAreaLightComponent(const AreaLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorAreaLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorAreaLightComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorAreaLightComponent>(
                        "Area Light", "An Area light emits light emits light from a goemetric shape.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-area-light.html")
                        ;

                    editContext->Class<AreaLightComponentController>(
                        "AreaLightComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &AreaLightComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<AreaLightComponentConfig>(
                        "AreaLightComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &AreaLightComponentConfig::m_color, "Color", "Color of the light")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute("ColorEditorConfiguration", AZ::RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AreaLightComponentConfig::m_intensityMode, "Intensity Mode", "Allows specifying which photometric unit to work in.")
                            ->EnumAttribute(PhotometricUnit::Candela, "Candela")
                            ->EnumAttribute(PhotometricUnit::Lumen, "Lumen")
                            ->EnumAttribute(PhotometricUnit::Nit, "Nit")
                            ->EnumAttribute(PhotometricUnit::Ev100Luminance, "Ev100")
                        ->DataElement(Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_intensity, "Intensity", "Intensity of the light in the set photometric unit.")
                            ->Attribute(Edit::Attributes::Min, &AreaLightComponentConfig::GetIntensityMin)
                            ->Attribute(Edit::Attributes::Max, &AreaLightComponentConfig::GetIntensityMax)
                            ->Attribute(Edit::Attributes::SoftMin, &AreaLightComponentConfig::GetIntensitySoftMin)
                            ->Attribute(Edit::Attributes::SoftMax, &AreaLightComponentConfig::GetIntensitySoftMax)
                            ->Attribute(Edit::Attributes::Suffix, &AreaLightComponentConfig::GetIntensitySuffix)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AreaLightComponentConfig::m_lightEmitsBothDirections, "Both Directions", "Whether light should emit from both sides of the surface or just the front")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &AreaLightComponentConfig::Is2DSurface)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AreaLightComponentConfig::m_useFastApproximation, "Fast Approximation", "Whether the light should use the default high quality linear transformed cosine technique or a faster approximation.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsFastApproximation)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Attenuation Radius")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AreaLightComponentConfig::m_attenuationRadiusMode, "Mode", "Controls whether the attenation radius is calculated automatically or set explicitly.")
                            ->EnumAttribute(LightAttenuationRadiusMode::Automatic, "Automatic")
                            ->EnumAttribute(LightAttenuationRadiusMode::Explicit, "Explicit")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &AreaLightComponentConfig::m_attenuationRadius, "Radius", "The distance at which this light no longer has an affect.")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &AreaLightComponentConfig::IsAttenuationRadiusModeAutomatic)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorAreaLightComponent>()->RequestBus("AreaLightRequestBus");

                behaviorContext->ConstantProperty("EditorAreaLightComponentTypeId", BehaviorConstant(Uuid(EditorAreaLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        void EditorAreaLightComponent::Activate()
        {
            m_controller.SetVisibiliy(IsVisible());

            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            
            // Override the shape component so that this component controls the color.
            LmbrCentral::EditorShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::EditorShapeComponentRequests::SetShapeColorIsEditable, false);
            LmbrCentral::EditorShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::EditorShapeComponentRequests::SetShapeColor, m_controller.m_configuration.m_color);
        }

        void EditorAreaLightComponent::Deactivate()
        {
            LmbrCentral::EditorShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::EditorShapeComponentRequests::SetShapeColorIsEditable, true);
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        AZ::u32 EditorAreaLightComponent::OnConfigurationChanged()
        {
            LmbrCentral::EditorShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::EditorShapeComponentRequests::SetShapeColor, m_controller.m_configuration.m_color);

            // If photometric unit changes, convert the intensities so the actual intensity doesn't change.
            m_controller.ConvertToIntensityMode(m_controller.m_configuration.m_intensityMode);

            BaseClass::OnConfigurationChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        void EditorAreaLightComponent::OnEntityVisibilityChanged(bool visibility)
        {
            m_controller.SetVisibiliy(visibility);
        }

        void EditorAreaLightComponent::DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (IsVisible())
            {
                m_controller.HandleDisplayEntityViewport(viewportInfo, debugDisplay, IsSelected());
            }
        }

        bool EditorAreaLightComponent::ShouldActivateController() const
        {
            // Always true since this component needs to activate even when invisible.
            return true;
        }

    } // namespace Render
} // namespace AZ
