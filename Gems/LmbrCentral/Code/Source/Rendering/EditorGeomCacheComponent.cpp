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
#include "EditorGeomCacheComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

namespace LmbrCentral
{
    void EditorGeometryCacheCommon::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGeometryCacheCommon, GeometryCacheCommon>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorGeometryCacheCommon>("Editor Geom Cache Common Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                editContext->Class<GeometryCacheCommon>("Geom Cache Common Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_visible, "Visible", "Should the GeomCache be rendered.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnRenderOptionsChanged)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeometryCacheCommon::m_minSpec, "Min Spec", "The minimum graphics spec where this GeomCache will be rendered.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnRenderOptionsChanged)
                    ->EnumAttribute(EngineSpec::Never, "Never")
                    ->EnumAttribute(EngineSpec::VeryHigh, "Very high")
                    ->EnumAttribute(EngineSpec::High, "High")
                    ->EnumAttribute(EngineSpec::Medium, "Medium")
                    ->EnumAttribute(EngineSpec::Low, "Low")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_geomCacheAsset, "Geom Cache", "The Alembic Geometry Cache asset.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnGeomCacheAssetChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_materialOverrideAsset, "Material Override", "Optional material override asset.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnMaterialOverrideChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_loop, "Loop", "Should the animation loop.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnLoopChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_playOnStart, "Play on Start", "Should the alembic animation play when the component activates.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnPlayOnStartChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_startTime, "Start Time", "The time point that the animation should start at.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnStartTimeChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_streamInDistance, "Stream In Distance", "How close does the viewer need to be for the GeomCache to begin streaming into memory.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnStreamInDistanceChanged)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Stand-in Settings")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_firstFrameStandin, "First Frame Stand-in", "The entity that should stand in for this GeomCache before playback begins.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnFirstFrameStandinChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_lastFrameStandin, "Last Frame Stand-in", "The entity that should stand in for this GeomCache after playback has ended.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnLastFrameStandinChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_standin, "Stand-in", "The entity that should stand in for this GeomCache when the viewer is past the Stand-in Distance.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnStandinChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_standinDistance, "Stand-in Distance", "How close does the viewer need to be before the GeomCache replaces the Stand-in.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Options")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_maxViewDistance, "Max View Distance", "That maximum distance that this GeomCache can be viewed from.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnMaxViewDistanceChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_viewDistanceMultiplier, "View Distance Multiplier", "Multiplied to the Max View Distance to get the final max view distance.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnViewDistanceMultiplierChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GeometryCacheCommon::m_lodDistanceRatio, "LOD Distance Ratio", "Controls LOD ratio over distance.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnLODDistanceRatioChanged)
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_castShadows, "Cast Shadows", "Should the GeomCache cast shadows.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnRenderOptionsChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GeometryCacheCommon::m_useVisAreas, "Use Vis Areas", "Should the GeomCache be affected by VisAreas.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GeometryCacheCommon::OnRenderOptionsChanged)
                    ;
            }
        }
    }

    void EditorGeometryCacheCommon::Activate()
    {
        GeometryCacheCommon::Activate();
        EditorGeometryCacheComponentRequestBus::Handler::BusConnect(m_entityId);

        //Store the serialized results here so that we can properly react to stand-in changes after level load
        m_prevFirstFrameStandin = m_firstFrameStandin;
        m_prevLastFrameStandin = m_lastFrameStandin;
        m_prevStandin = m_standin;
    }

    void EditorGeometryCacheCommon::Deactivate()
    {
        EditorGeometryCacheComponentRequestBus::Handler::BusDisconnect(m_entityId);
        GeometryCacheCommon::Deactivate();
    }

    void EditorGeometryCacheCommon::SetMinSpec(EngineSpec minSpec)
    {
        m_minSpec = minSpec;
        OnRenderOptionsChanged();
    }

    void EditorGeometryCacheCommon::SetPlayOnStart(bool playOnStart)
    {
        m_playOnStart = playOnStart;
        OnPlayOnStartChanged();
    }

    void EditorGeometryCacheCommon::SetMaxViewDistance(float maxViewDistance)
    {
        m_maxViewDistance = maxViewDistance;
        OnMaxViewDistanceChanged();
    }

    void EditorGeometryCacheCommon::SetViewDistanceMultiplier(float viewDistanceMultiplier)
    {
        m_viewDistanceMultiplier = viewDistanceMultiplier;
        OnViewDistanceMultiplierChanged();
    }

    void EditorGeometryCacheCommon::SetLODDistanceRatio(AZ::u32 lodDistanceRatio)
    {
        m_lodDistanceRatio = lodDistanceRatio;
        OnLODDistanceRatioChanged();
    }

    void EditorGeometryCacheCommon::SetCastShadows(bool castShadows)
    {
        m_castShadows = castShadows;
        OnRenderOptionsChanged();
    }

    void EditorGeometryCacheCommon::SetUseVisAreas(bool useVisAreas)
    {
        m_useVisAreas = useVisAreas;
        OnRenderOptionsChanged();
    }
    
    void EditorGeometryCacheCommon::OnFirstFrameStandinChanged()
    {
        //We only need to remove the prev stand-in from the geom cache's hierarchy if it's not used anywhere else
        bool removePrevFirstFrameStandinTransform = false;
        if (m_prevFirstFrameStandin != m_prevStandin && m_prevFirstFrameStandin != m_prevLastFrameStandin)
        {
            removePrevFirstFrameStandinTransform = true;
        }
        HandleStandinChanged(m_prevFirstFrameStandin, m_firstFrameStandin, removePrevFirstFrameStandinTransform);

        m_prevFirstFrameStandin = m_firstFrameStandin;
    }

    void EditorGeometryCacheCommon::OnLastFrameStandinChanged()
    {
        //We only need to remove the prev stand-in from the geom cache's hierarchy if it's not used anywhere else
        bool removePrevLastFrameStandinTransform = false;
        if (m_prevLastFrameStandin != m_prevStandin && m_prevLastFrameStandin != m_prevFirstFrameStandin)
        {
            removePrevLastFrameStandinTransform = true;
        }
        HandleStandinChanged(m_prevLastFrameStandin, m_lastFrameStandin, removePrevLastFrameStandinTransform);

        m_prevLastFrameStandin = m_lastFrameStandin;
    }

    void EditorGeometryCacheCommon::OnStandinChanged()
    {
        //We only need to remove the prev stand-in from the geom cache's hierarchy if it's not used anywhere else
        bool removePrevStandinTransform = false;
        if (m_prevStandin != m_prevLastFrameStandin && m_prevStandin != m_prevFirstFrameStandin)
        {
            removePrevStandinTransform = true;
        }
        HandleStandinChanged(m_prevStandin, m_standin, removePrevStandinTransform);
        m_prevStandin = m_standin;
    }

    void EditorGeometryCacheCommon::HandleStandinChanged(const AZ::EntityId& prevStandinId, const AZ::EntityId& newStandinId, bool evictPrevStandinTransform)
    {
        //Undo modifications we've made to the stand-in entity
        if (prevStandinId.IsValid() && evictPrevStandinTransform)
        {
            //If this stand-in isn't currently active it may be set to invisible. If it's not going to be a stand-in anymore we want it to be visible
            MeshComponentRequestBus::Event(prevStandinId, &MeshComponentRequestBus::Events::SetVisibility, true);
            //If it's not going to be a stand-in anymore it also doesn't need to be parented. User can re-parent if needed
            AZ::TransformBus::Event(prevStandinId, &AZ::TransformBus::Events::SetParent, AZ::EntityId()); 
        }
        
        //Parent new stand-in to geometry cache transform
        AZ::TransformBus::Event(newStandinId, &AZ::TransformBus::Events::SetParent, m_entityId);

        //Standin will be made visible if necessary in GeomCacheCommon::OnTick
        MeshComponentRequestBus::Event(m_standin, &MeshComponentRequestBus::Events::SetVisibility, false);
        MeshComponentRequestBus::Event(m_firstFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, false);
        MeshComponentRequestBus::Event(m_lastFrameStandin, &MeshComponentRequestBus::Events::SetVisibility, false);

        //Force stand-in logic to refresh
        m_currentStandinType = StandinType::None;
    }

    void EditorGeometryCacheCommon::SetMaterial(_smart_ptr<IMaterial> material)
    {
        GeometryCacheCommon::SetMaterial(material);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorGeometryCacheComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("GeomCacheService", 0x3d2bc48c));
    }

    void EditorGeometryCacheComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorGeometryCacheComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGeometryCacheComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Common", &EditorGeometryCacheComponent::m_common)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorGeometryCacheComponent>("Geometry Cache", "Controls playback of baked vertex animations.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/GeometryCache.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/GeometryCache.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/geom-cache-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGeometryCacheComponent::m_common, "Common", "No Description")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorGeometryCacheComponent>()
                ->RequestBus("GeometryCacheComponentRequestBus")
                ->RequestBus("GeometryCacheComponentNotificationBus")
                ;
        }

        EditorGeometryCacheCommon::Reflect(context);
    }

    void EditorGeometryCacheComponent::Init()
    {
        m_common.Init(GetEntityId());
    }

    void EditorGeometryCacheComponent::Activate()
    {
        m_common.Activate();

        AzToolsFramework::Components::EditorComponentBase::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        //Get initial world transform for debug rendering
        m_currentWorldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentWorldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
    }

    void EditorGeometryCacheComponent::Deactivate()
    {
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(GetEntityId());

        m_common.Deactivate();
    }

    void EditorGeometryCacheComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        m_common.SetGeomCacheAsset(assetId);
    }

    void EditorGeometryCacheComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<GeometryCacheComponent>(&m_common);
    }

    void EditorGeometryCacheComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // Don't draw extra visualization unless selected.
        if (!IsSelected())
        {
            return;
        }

        debugDisplay.PushMatrix(m_currentWorldTransform);

        debugDisplay.SetColor(AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

        debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), m_common.GetStandInDistance());
        debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), m_common.GetStreamInDistance());

        debugDisplay.PopMatrix();
    }

    void EditorGeometryCacheComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
    }

} //namespace LmbrCentral