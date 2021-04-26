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
#include "LmbrCentral_precompiled.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>

#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <IEntityRenderState.h>

#include "EditorFogVolumeComponent.h"

namespace LmbrCentral
{
    void EditorFogVolumeComponent::Activate()
    {
        Base::Activate();

        const auto entityId = GetEntityId();

        m_configuration.SetEntityId(entityId);
        m_configuration.UpdateSizeFromEntityShape();

        m_fogVolume.SetEntityId(entityId);
        m_fogVolume.CreateFogVolumeRenderNode(m_configuration);

        RefreshFog();

        RenderNodeRequestBus::Handler::BusConnect(entityId);
        FogVolumeComponentRequestBus::Handler::BusConnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void EditorFogVolumeComponent::Deactivate()
    {
        Base::Deactivate();

        m_fogVolume.DestroyRenderNode();
        m_fogVolume.SetEntityId(AZ::EntityId());
        m_configuration.SetEntityId(AZ::EntityId());

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        FogVolumeComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();
    }

    void EditorFogVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        EditorFogVolumeConfiguration::Reflect(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorFogVolumeComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("FogVolumeConfiguration", &EditorFogVolumeComponent::m_configuration);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorFogVolumeComponent>(
                    "Fog Volume", "Allows to specify an area with a fog")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/FogVolume.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/FogVolume.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/fog-volume-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(0, &EditorFogVolumeComponent::m_configuration, "Settings", "Fog configuration")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorFogVolumeComponent>()->RequestBus("EditorFogVolumeComponentRequestBus");
            FogVolumeComponent::ExposeRequestsBusInBehaviorContext(behaviorContext, "EditorFogVolumeComponentRequestBus");
        }
    }

    IRenderNode* EditorFogVolumeComponent::GetRenderNode()
    {
        return m_fogVolume.GetRenderNode();
    }

    float EditorFogVolumeComponent::GetRenderNodeRequestBusOrder() const
    {
        return FogVolumeComponent::s_renderNodeRequestBusOrder;
    }

    void EditorFogVolumeComponent::RefreshFog()
    {
        m_fogVolume.UpdateFogVolumeProperties(m_configuration);
        m_fogVolume.UpdateRenderingFlags(m_configuration);
        m_fogVolume.UpdateFogVolumeTransform();
    }

    void EditorFogVolumeComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        // Entities transform component calls OnTransformChanged before this component is activated
        // This only happens during undo operations so we guard against that in the Editor component
        if (m_fogVolume.GetRenderNode())
        {
            RefreshFog();
        }
    }

    void EditorFogVolumeComponent::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            m_configuration.UpdateSizeFromEntityShape();
            RefreshFog();
        }
    }

    void EditorFogVolumeComponent::OnEditorSpecChange()
    {
        RefreshFog();
    }

    void EditorFogVolumeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        return FogVolumeComponent::GetRequiredServices(required);
    }

    void EditorFogVolumeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        FogVolumeComponent* fogComponent = gameEntity->CreateComponent<FogVolumeComponent>();
        if (fogComponent)
        {
            fogComponent->SetConfiguration(m_configuration);
        }
    }

    void EditorFogVolumeConfiguration::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorFogVolumeConfiguration, FogVolumeConfiguration>()->
                Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<FogVolumeConfiguration>("Configuration", "Fog Volume configuration")
                     
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FogVolumeConfiguration::m_volumeType,
                        "Volume type", "Shape of the fog")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->EnumAttribute(FogVolumeType::RectangularPrism, "Cuboid")
                        ->EnumAttribute(FogVolumeType::Ellipsoid, "Ellipsoid")

                    ->DataElement(AZ::Edit::UIHandlers::Color, &FogVolumeConfiguration::m_color, 
                        "Color", "Fog color")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &FogVolumeConfiguration::m_useGlobalFogColor, 
                        "Use global fog color", "If true, the Color property is ignored. Instead, the current global fog color is used")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_globalDensity,
                        "Fog Density", "Controls the density of the fog. The higher the value the more dense the fog and the less you'll be able to see objects behind or inside the fog volume")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_densityOffset,
                        "Density offset", "Offset fog density, used in conjunction with the GlobalDensity parameter")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, -1000.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_nearCutoff,
                        "Near cutoff", "Stop rendering the object, depending on camera distance to object")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_softEdges,
                        "Soft edges", "Specifies a factor that is used to soften the edges of the fog volume when viewed from outside")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_windInfluence,
                        "Wind influence (Volumetric Fog only)", "Controls the influence of the wind (Volumetric Fog only)")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rendering General")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &FogVolumeConfiguration::m_ignoresVisAreas,
                        "Ignore vis. areas", "Controls whether this entity should respond to visareas")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &FogVolumeConfiguration::m_affectsThisAreaOnly,
                        "Affect this area only", "Set this parameter to false to make this entity affect in multiple visareas")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)

                    ->DataElement(0, &FogVolumeConfiguration::m_viewDistMultiplier,
                        "View distance multiplier", "Adjusts max view distance. If 1.0 then default is used. 1.1 would be 10% further than default.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Suffix, "x")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FogVolumeConfiguration::m_minSpec,
                        "Minimum spec", "Min spec for the fog to be active.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->EnumAttribute(EngineSpec::Never, "Never")
                        ->EnumAttribute(EngineSpec::VeryHigh, "Very high")
                        ->EnumAttribute(EngineSpec::High, "High")
                        ->EnumAttribute(EngineSpec::Medium, "Medium")
                        ->EnumAttribute(EngineSpec::Low, "Low")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_hdrDynamic,
                        "HDR Dynamic (Non-Volumetric Fog)", "Specifies how much brighter than the default 255,255,255 white the fog is (Non-Volumetric Fog only)")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Fall Off Settings")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_fallOffDirLong,
                        "Longitude", "Controls the longitude of the world space fall off direction of the fog. 0 represents East, rotation is counter-clockwise")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_fallOffDirLatitude,
                        "Latitude", "Controls the latitude of the world space fall off direction of the fog. 90 lets the fall off direction point upwards in world space")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_fallOffShift,
                        "Shift", "Controls how much to shift the fog density distribution along the fall off direction in world units")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, -50.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_fallOffScale,
                        "Scale", "Scales the density distribution along the fall off direction. Higher values will make the fog fall off more rapidly and generate thicker fog layers along the negative fall off direction")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, -50.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Ramp (Volumetric Fog only)")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_rampStart,
                        "Start", "Specifies the start distance of fog density ramp in world units")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 30000.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_rampEnd,
                        "End", "Specifies the end distance of fog density ramp in world units")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 30000.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_rampInfluence,
                        "Influence", "Controls the influence of fog density ramp")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Density Noise (Volumetric Fog only)")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_densityNoiseScale,
                        "Scale", "Scales the noise for the density")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_densityNoiseOffset,
                        "Offset", "Offsets the noise for the density")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, -2.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FogVolumeConfiguration::m_densityNoiseTimeFrequency,
                        "Time frequency", "Controls the time frequency of the noise for the density")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &FogVolumeConfiguration::m_densityNoiseFrequency,
                        "Spatial frequency", "Controls the spatial frequency of the noise for the density")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FogVolumeConfiguration::PropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                    ;
            }
        }
    }

    AZ::Crc32 EditorFogVolumeConfiguration::PropertyChanged()
    {
        if (m_entityId.IsValid())
        {
            FogVolumeComponentRequestBus::Event(m_entityId, &FogVolumeComponentRequestBus::Events::RefreshFog);
        }
        return AZ::Edit::PropertyRefreshLevels::None;
    }
}
