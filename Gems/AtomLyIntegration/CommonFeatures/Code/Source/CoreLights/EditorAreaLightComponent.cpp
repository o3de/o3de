/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/EditorAreaLightComponent.h>

#include <Atom/RPI.Edit/Common/ColorUtils.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/EditorShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

#include <AzFramework/Translation/TranslationDef.h>

namespace AZ
{
    namespace Render
    {
        EditorAreaLightComponent::EditorAreaLightComponent(const AreaLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorAreaLightComponent::Reflect(ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EditorAreaLightComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorAreaLightComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Light"),
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "A light which emits from a point or geometric shape."))
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(Edit::Attributes::Icon, "Icons/Components/AreaLight.svg")
                            ->Attribute(Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AreaLight.svg")
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/light/")
                        ;

                    editContext->Class<AreaLightComponentController>(
                        "AreaLightComponentController", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Default, &AreaLightComponentController::m_configuration,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                            ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<AreaLightComponentConfig>(
                        "AreaLightComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(Edit::UIHandlers::ComboBox, &AreaLightComponentConfig::m_lightType,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Light type"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Which type of light this component represents."))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::Unknown,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Choose a light type"))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::Sphere,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Point (sphere)"))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::SimplePoint,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Point (simple punctual)"))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::SpotDisk,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Spot (disk)"))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::SimpleSpot,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Spot (simple punctual)"))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::Capsule,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Capsule"))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::Quad,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Quad"))
                            ->EnumAttribute(AreaLightComponentConfig::LightType::Polygon,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Polygon"))
                        ->DataElement(Edit::UIHandlers::Color, &AreaLightComponentConfig::m_color,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Color"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Color of the light"))
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::LightTypeIsSelected)
                            ->Attribute("ColorEditorConfiguration", RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(Edit::UIHandlers::ComboBox, &AreaLightComponentConfig::m_intensityMode,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity mode"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Allows specifying which photometric unit to work in."))
                            ->Attribute(AZ::Edit::Attributes::EnumValues, &AreaLightComponentConfig::GetValidPhotometricUnits)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::LightTypeIsSelected)
                        ->DataElement(Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_intensity,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity of the light in the set photometric unit."))
                            ->Attribute(Edit::Attributes::Min, &AreaLightComponentConfig::GetIntensityMin)
                            ->Attribute(Edit::Attributes::Max, &AreaLightComponentConfig::GetIntensityMax)
                            ->Attribute(Edit::Attributes::SoftMin, &AreaLightComponentConfig::GetIntensitySoftMin)
                            ->Attribute(Edit::Attributes::SoftMax, &AreaLightComponentConfig::GetIntensitySoftMax)
                            ->Attribute(Edit::Attributes::Suffix, &AreaLightComponentConfig::GetIntensitySuffix)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::LightTypeIsSelected)
                        ->DataElement(Edit::UIHandlers::CheckBox, &AreaLightComponentConfig::m_lightEmitsBothDirections,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Both directions"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Whether light should emit from both sides of the surface or just the front"))
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsBothDirections)
                        ->DataElement(Edit::UIHandlers::CheckBox, &AreaLightComponentConfig::m_useFastApproximation,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fast approximation"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Whether the light should use the default high quality linear transformed cosine technique or a faster approximation."))
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsFastApproximation)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &AreaLightComponentConfig::m_goboImageAsset,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Gobo"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Select a texture to be used as gobo"))
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsGobo)
                        ->ClassElement(Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Attenuation radius"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::LightTypeIsSelected)
                        ->DataElement(Edit::UIHandlers::ComboBox, &AreaLightComponentConfig::m_attenuationRadiusMode,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Mode"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls whether the attenuation radius is calculated automatically or set explicitly."))
                            ->EnumAttribute(LightAttenuationRadiusMode::Automatic,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Automatic"))
                            ->EnumAttribute(LightAttenuationRadiusMode::Explicit,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Explicit"))
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::LightTypeIsSelected)
                        ->DataElement(Edit::UIHandlers::Default, &AreaLightComponentConfig::m_attenuationRadius,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Radius"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "The distance at which this light no longer has an affect."))
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::IsAttenuationRadiusModeAutomatic)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::LightTypeIsSelected)
                        
                        ->DataElement(AZ::Edit::UIHandlers::Default, &AreaLightComponentConfig::m_lightingChannelConfig,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Lighting Channels"), "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)

                        ->ClassElement(Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shutters"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShutters)
                        ->DataElement(Edit::UIHandlers::Default, &AreaLightComponentConfig::m_enableShutters,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable shutters"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Restrict the light to a specific beam angle depending on shape."))
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::ShuttersMustBeEnabled)
                        ->DataElement(Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_innerShutterAngleDegrees,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Inner angle"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "The inner angle of the shutters where the light beam begins to be occluded."))
                            ->Attribute(Edit::Attributes::Min, 0.5f)
                            ->Attribute(Edit::Attributes::Max, 90.0f)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShutters)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::ShuttersDisabled)
                        ->DataElement(Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_outerShutterAngleDegrees,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Outer angle"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "The outer angle of the shutters where the light beam is completely occluded."))
                            ->Attribute(Edit::Attributes::Min, 0.5f)
                            ->Attribute(Edit::Attributes::Max, 90.0f)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShutters)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::ShuttersDisabled)

                        ->ClassElement(Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                        ->DataElement(Edit::UIHandlers::Default, &AreaLightComponentConfig::m_enableShadow,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable shadow"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable shadow for the light"))
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                        ->DataElement(Edit::UIHandlers::ComboBox, &AreaLightComponentConfig::m_shadowmapMaxSize,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadowmap size"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Width and height of shadowmap"))
                            ->EnumAttribute(ShadowmapSize::Size256, " 256")
                            ->EnumAttribute(ShadowmapSize::Size512, " 512")
                            ->EnumAttribute(ShadowmapSize::Size1024, "1024")
                            ->EnumAttribute(ShadowmapSize::Size2048, "2048")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::ShadowsDisabled)
                        ->DataElement(Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_bias,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Bias"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "How deep in shadow a surface must be before being affected by it."))
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)
                            ->Attribute(Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::ShadowsDisabled)
                        ->DataElement(Edit::UIHandlers::ComboBox, &AreaLightComponentConfig::m_shadowFilterMethod,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow filter method"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Filtering method of edge-softening of shadows.\n"
                            "  None: no filtering\n"
                            "  PCF: Percentage-closer Filtering\n"
                            "  ESM: Exponential shadow maps\n"
                            "  ESM+PCF: ESM with a PCF fallback\n"
                            "For BehaviorContext (or track view), None=0, PCF=1, ESM=2, ESM+PCF=3"))
                            ->EnumAttribute(ShadowFilterMethod::None,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "None"))
                            ->EnumAttribute(ShadowFilterMethod::Pcf,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "PCF"))
                            ->EnumAttribute(ShadowFilterMethod::Esm,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "ESM"))
                            ->EnumAttribute(ShadowFilterMethod::EsmPcf,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "ESM+PCF"))
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::ShadowsDisabled)
                        ->DataElement(Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_filteringSampleCount,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Filtering sample count"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "This is only used when the pixel is predicted to be on the boundary. Specific to PCF and ESM+PCF."))
                            ->Attribute(Edit::Attributes::Min, 4)
                            ->Attribute(Edit::Attributes::Max, 64)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::IsShadowPcfDisabled)
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_esmExponent,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "ESM exponent"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Exponent used by ESM shadows. "
                            "Larger values increase the sharpness of the border between lit and unlit areas."))
                            ->Attribute(Edit::Attributes::Min, 1.0f)
                            ->Attribute(Edit::Attributes::Max, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                            ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.05f)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::IsEsmDisabled)
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_normalShadowBias,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Normal shadow bias"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Reduces acne by biasing the shadowmap lookup along the geometric normal.\n"
                            "If this is 0, no biasing is applied."))
                            ->Attribute(Edit::Attributes::Min, 0.f)
                            ->Attribute(Edit::Attributes::Max, 10.0f)
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::ShadowsDisabled)
                        ->DataElement(
                            Edit::UIHandlers::CheckBox, &AreaLightComponentConfig::m_cacheShadows,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Cache shadows"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "When turned on, shadows will only render when something in their field of view changes. This takes more memory "
                            "because the shadow map texture needs to persist, but can be a performance improvement for largely static scenes."))

                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsShadows)
                            ->Attribute(Edit::Attributes::ReadOnly, &AreaLightComponentConfig::ShadowsDisabled)

                        ->ClassElement(Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Global illumination (GI)"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsAffectsGI)
                         ->DataElement(Edit::UIHandlers::CheckBox, &AreaLightComponentConfig::m_affectsGI,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Affects GI"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls whether this light affects diffuse global illumination."))
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsAffectsGI)
                         ->DataElement(Edit::UIHandlers::Slider, &AreaLightComponentConfig::m_affectsGIFactor,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Factor"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Multiplier on the amount of contribution to diffuse global illumination."))
                            ->Attribute(Edit::Attributes::Visibility, &AreaLightComponentConfig::SupportsAffectsGI)
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 2.0f)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorAreaLightComponent>()->RequestBus("AreaLightRequestBus");

                behaviorContext->ConstantProperty("EditorAreaLightComponentTypeId", BehaviorConstant(Uuid(EditorAreaLightComponentTypeId)))
                    ->Attribute(Script::Attributes::Module, "render")
                    ->Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Automation);
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

            AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorAreaLightComponent::Deactivate()
        {
            AzFramework::BoundsRequestBus::Handler::BusDisconnect();

            LmbrCentral::EditorShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::EditorShapeComponentRequests::SetShapeColorIsEditable, true);
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        bool EditorAreaLightComponent::HandleLightTypeChange()
        {
            if (m_lightType == AreaLightComponentConfig::LightType::Unknown)
            {
                // Light type is unknown, see if it can be determined from a shape component.
                Crc32 shapeType = Crc32();
                LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeType, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);
                
                constexpr Crc32 SphereShapeTypeId = AZ_CRC_CE("Sphere");
                constexpr Crc32 DiskShapeTypeId = AZ_CRC_CE("DiskShape");
                constexpr Crc32 CapsuleShapeTypeId = AZ_CRC_CE("Capsule");
                constexpr Crc32 QuadShapeTypeId = AZ_CRC_CE("QuadShape");
                constexpr Crc32 PoylgonShapeTypeId = AZ_CRC_CE("PolygonPrism");

                switch (shapeType)
                {
                case SphereShapeTypeId:
                    m_lightType = AreaLightComponentConfig::LightType::Sphere;
                    break;
                case DiskShapeTypeId:
                    m_lightType = AreaLightComponentConfig::LightType::SpotDisk;
                    break;
                case CapsuleShapeTypeId:
                    m_lightType = AreaLightComponentConfig::LightType::Capsule;
                    break;
                case QuadShapeTypeId:
                    m_lightType = AreaLightComponentConfig::LightType::Quad;
                    break;
                case PoylgonShapeTypeId:
                    m_lightType = AreaLightComponentConfig::LightType::Polygon;
                    break;
                default:
                    break; // Light type can't be deduced. 
                }
            }

            if (m_lightType == m_controller.m_configuration.m_lightType)
            {
                // No change, nothing to do
                return false;
            }

            // Update the cached light type.
            m_lightType = m_controller.m_configuration.m_lightType;
            
            // Check to see if the current photometric type is supported by the light type. If not, convert to lumens before deactivating.
            auto supportedPhotometricUnits = m_controller.m_configuration.GetValidPhotometricUnits();
            auto foundIt = AZStd::find_if(
                supportedPhotometricUnits.begin(),
                supportedPhotometricUnits.end(),
                [&](const Edit::EnumConstant<PhotometricUnit>& entry) -> bool
                {
                    return AZStd::RemoveEnum<PhotometricUnit>::type(m_controller.m_configuration.m_intensityMode) == entry.m_value;
                }
            );

            if (foundIt == supportedPhotometricUnits.end())
            {
                m_controller.ConvertToIntensityMode(PhotometricUnit::Lumen);
            }
            
            // components may be removed or added here, so deactivate now and reactivate the entity when everything is done shifting around.
            GetEntity()->Deactivate();

            // Check if there is already a shape components and remove it.
            for (Component* component : GetEntity()->GetComponents())
            {
                ComponentDescriptor::DependencyArrayType provided;
                ComponentDescriptor* componentDescriptor = nullptr;
                ComponentDescriptorBus::EventResult(
                    componentDescriptor, component->RTTI_GetType(), &ComponentDescriptorBus::Events::GetDescriptor);
                AZ_Assert(componentDescriptor, "Component class %s descriptor is not created! It must be before you can use it!", component->RTTI_GetTypeName());
                componentDescriptor->GetProvidedServices(provided, component);
                auto providedItr = AZStd::find(provided.begin(), provided.end(), AZ_CRC_CE("ShapeService"));
                if (providedItr != provided.end())
                {
                    AZStd::vector<Component*> componentsToRemove = { component };
                    AzToolsFramework::EntityCompositionRequests::RemoveComponentsOutcome outcome =
                        AZ::Failure(AZStd::string("Failed to remove old shape component."));
                    AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(outcome, &AzToolsFramework::EntityCompositionRequests::RemoveComponents, componentsToRemove);
                    break;
                }
            }

            // Add a new shape component for light types that require it.
            
            auto addComponentOfType = [&](AZ::Uuid type)
            {
                AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
                    AZ::Failure(AZStd::string("Failed to add shape component for light type"));

                const AZStd::vector<AZ::EntityId> entityList = { GetEntityId() };

                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
                    entityList, ComponentTypeList({ type }));
            };

            switch (m_lightType)
            {
            case AreaLightComponentConfig::LightType::Sphere:
                addComponentOfType(LmbrCentral::EditorSphereShapeComponentTypeId);
                break;
            case AreaLightComponentConfig::LightType::SpotDisk:
                addComponentOfType(LmbrCentral::EditorDiskShapeComponentTypeId);
                break;
            case AreaLightComponentConfig::LightType::Capsule:
                addComponentOfType(LmbrCentral::EditorCapsuleShapeComponentTypeId);
                break;
            case AreaLightComponentConfig::LightType::Quad:
                addComponentOfType(LmbrCentral::EditorQuadShapeComponentTypeId);
                break;
            case AreaLightComponentConfig::LightType::Polygon:
                addComponentOfType(LmbrCentral::EditorPolygonPrismShapeComponentTypeId);
                break;
            default:
                // Some light types don't require a shape, this is ok. 
                break;
            }
            
            GetEntity()->Activate();

            // Set more reasonable default values for certain shapes.
            switch (m_lightType)
            {
            case AreaLightComponentConfig::LightType::Sphere:
                LmbrCentral::SphereShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::SphereShapeComponentRequests::SetRadius, 0.05f);
                break;
            case AreaLightComponentConfig::LightType::SpotDisk:
                LmbrCentral::DiskShapeComponentRequestBus::Event(GetEntityId(), &LmbrCentral::DiskShapeComponentRequests::SetRadius, 0.05f);
                break;
            }

            return true;
        }

        u32 EditorAreaLightComponent::OnConfigurationChanged()
        {
            bool needsFullRefresh = HandleLightTypeChange();

            LmbrCentral::EditorShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::EditorShapeComponentRequests::SetShapeColor, m_controller.m_configuration.m_color);
            LmbrCentral::EditorShapeComponentRequestsBus::Event(GetEntityId(), &LmbrCentral::EditorShapeComponentRequests::SetShapeWireframeColor, m_controller.m_configuration.m_color);

            // If photometric unit changes, convert the intensities so the actual intensity doesn't change.
            m_controller.ConvertToIntensityMode(m_controller.m_configuration.m_intensityMode);

            BaseClass::OnConfigurationChanged();

            if (needsFullRefresh)
            {
                return Edit::PropertyRefreshLevels::EntireTree;
            }
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

        AZ::Aabb EditorAreaLightComponent::GetWorldBounds() const
        {
            Transform transform = Transform::CreateIdentity();
            TransformBus::EventResult(transform, GetEntityId(), &TransformBus::Events::GetWorldTM);
            return GetLocalBounds().GetTransformedAabb(transform);
        }

        AZ::Aabb EditorAreaLightComponent::GetLocalBounds() const
        {
            return m_controller.GetLocalVisualizationBounds();
        }
    } // namespace Render
} // namespace AZ
