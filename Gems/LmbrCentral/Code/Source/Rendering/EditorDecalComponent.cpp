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
#include "EditorDecalComponent.h"
#include <IEditor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <IEditor.h>
#include <I3DEngine.h>

namespace LmbrCentral
{
    void EditorDecalComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        EditorDecalConfiguration::Reflect(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorDecalComponent, EditorComponentBase>()->
                Version(1)->
                Field("EditorDecalConfiguration", &EditorDecalComponent::m_configuration)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorDecalComponent>(
                    "Decal", "The Decal component allows an entity to project a texture or material onto a mesh")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(AZ::Edit::Attributes::Category, "Rendering")->
                    Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Decal.svg")->
                    Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<LmbrCentral::MaterialAsset>::Uuid())->
                    Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Decal.png")->
                    Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))->
                    Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                    Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-decal.html")->
                    DataElement(0, &EditorDecalComponent::m_configuration, "Settings", "Decal configuration")->
                    Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void EditorDecalConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorDecalConfiguration, DecalConfiguration>()->
                Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<DecalConfiguration>(
                    "Render Settings", "Rendering options for the decal.")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                    Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))->

                    ClassElement(AZ::Edit::ClassElements::Group, "Decal Settings")->
                    Attribute(AZ::Edit::Attributes::AutoExpand, true)->

                    DataElement(0, &DecalConfiguration::m_position, "Offset", "")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->

                    DataElement(0, &DecalConfiguration::m_visible, "Visible", "")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->

                    DataElement(AZ::Edit::UIHandlers::ComboBox, &DecalConfiguration::m_projectionType, "Projection type", "")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MajorPropertyChanged)->
                    EnumAttribute(SDecalProperties::EProjectionType::ePlanar, "Planar")->
                    EnumAttribute(SDecalProperties::EProjectionType::eProjectOnTerrain, "On Terrain")->
                    EnumAttribute(SDecalProperties::EProjectionType::eProjectOnTerrainAndStaticObjects, "On Terrain and Static Objects")->

                    DataElement(AZ::Edit::UIHandlers::Default, &DecalConfiguration::m_deferredString, "Deferred", "")->
                    Attribute(AZ::Edit::Attributes::ReadOnly, true)->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MajorPropertyChanged)->

                    DataElement(AZ::Edit::UIHandlers::SpinBox, &DecalConfiguration::m_sortPriority, "Sort priority", "Higher priority renders decals on top.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->
                    Attribute(AZ::Edit::Attributes::Min, 0)->
                    Attribute(AZ::Edit::Attributes::Max, 255)->
                    Attribute(AZ::Edit::Attributes::Step, 1)->

                    DataElement(AZ::Edit::UIHandlers::SpinBox, &DecalConfiguration::m_depth, "Depth", "")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->
                    Attribute(AZ::Edit::Attributes::Min, 0.0001f)->
                    Attribute(AZ::Edit::Attributes::Max, 10.f)->
                    Attribute(AZ::Edit::Attributes::Step, 1.f / 255.f)->

                    DataElement(0, &DecalConfiguration::m_material, "Material", "")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MajorPropertyChanged)->

                    DataElement(AZ::Edit::UIHandlers::Slider, &DecalConfiguration::m_opacity, "Opacity", "Additional opacity setting on top of the distance from the decal to the surface")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, 1.f)->
                    Attribute(AZ::Edit::Attributes::Visibility, &DecalConfiguration::m_deferred)->

                    DataElement(AZ::Edit::UIHandlers::Slider, &DecalConfiguration::m_angleAttenuation, "Angle Attenuation", "amount of angle attenuation computation taken into account")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, 1.f)->
                    Attribute(AZ::Edit::Attributes::Visibility, &DecalConfiguration::m_deferred)->

                    ClassElement(AZ::Edit::ClassElements::Group, "Options")->

                    DataElement(AZ::Edit::UIHandlers::Default, &DecalConfiguration::m_maxViewDist, "Max view distance", "The furthest distance this decal can be seen from")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->
                    Attribute(AZ::Edit::Attributes::Suffix, " m")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, 8000.0f)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(AZ::Edit::UIHandlers::SpinBox, &DecalConfiguration::m_viewDistanceMultiplier, "View distance multiplier", "")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX)->

                    DataElement(AZ::Edit::UIHandlers::ComboBox, &DecalConfiguration::m_minSpec, "Minimum spec", "Min spec for light to be active.")->
                    Attribute(AZ::Edit::Attributes::ChangeNotify, &DecalConfiguration::MinorPropertyChanged)->
                    EnumAttribute(EngineSpec::Never, "Never")->
                    EnumAttribute(EngineSpec::VeryHigh, "Very high")->
                    EnumAttribute(EngineSpec::High, "High")->
                    EnumAttribute(EngineSpec::Medium, "Medium")->
                    EnumAttribute(EngineSpec::Low, "Low")
                ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    AZ::u32 EditorDecalConfiguration::MajorPropertyChanged()
    {
        if (m_editorEntityId.IsValid())
        {
            EBUS_EVENT_ID(m_editorEntityId, DecalComponentEditorRequests::Bus, RefreshDecal);
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::u32 EditorDecalConfiguration::MinorPropertyChanged()
    {
        if (m_editorEntityId.IsValid())
        {
            EBUS_EVENT_ID(m_editorEntityId, DecalComponentEditorRequests::Bus, RefreshDecal);
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    //////////////////////////////////////////////////////////////////////////

    EditorDecalComponent::EditorDecalComponent()
        : m_materialLayersMask(0)
    {
    }

    EditorDecalComponent::~EditorDecalComponent()
    {
    }

    void EditorDecalComponent::Activate()
    {
        Base::Activate();

        AZ::EntityId entityId = GetEntityId();

        IEditor* editor = nullptr;
        EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);

        m_configuration.m_editorEntityId = entityId;
        m_decalRenderNode = static_cast<IDecalRenderNode*>(editor->Get3DEngine()->CreateRenderNode(eERType_Decal));
        RefreshDecal();

        MaterialOwnerRequestBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        DecalComponentEditorRequests::Bus::Handler::BusConnect(entityId);
        RenderNodeRequestBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);
    }

    void EditorDecalComponent::Deactivate()
    {
        MaterialOwnerRequestBus::Handler::BusDisconnect();
        DecalComponentEditorRequests::Bus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();

        m_configuration.m_editorEntityId.SetInvalid();

        if (m_decalRenderNode)
        {
            IEditor* editor = nullptr;
            EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
            editor->Get3DEngine()->DeleteRenderNode(m_decalRenderNode);

            m_decalRenderNode = nullptr;
        }

        Base::Deactivate();
    }

    void EditorDecalComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        DecalComponent* decalComponent = gameEntity->CreateComponent<DecalComponent>();

        if (decalComponent)
        {
            decalComponent->SetConfiguration(m_configuration);
        }
    }

    void EditorDecalComponent::OnEditorSpecChange()
    {
        RefreshDecal();
    }

    void EditorDecalComponent::RefreshDecal()
    {
        if (!m_decalRenderNode)
        {
            return;
        }

        if (m_configuration.m_projectionType == SDecalProperties::EProjectionType::eProjectOnTerrainAndStaticObjects)
        {
            m_configuration.m_deferred = true;
            m_configuration.m_deferredString = "Yes";
        }
        else
        {
            m_configuration.m_deferred = false;
            m_configuration.m_deferredString = "No";
        }

        m_renderFlags = 0;

        if (IsSelected())
        {
            m_renderFlags |= ERF_SELECTED;
        }

        // take the entity's visibility into account
        bool visible = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            visible, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        const int configSpec = gEnv->pSystem->GetConfigSpec(true);
        if (!visible || !m_configuration.m_visible || static_cast<AZ::u32>(configSpec) < static_cast<AZ::u32>(m_configuration.m_minSpec))
        {
            m_renderFlags |= ERF_HIDDEN;
        }

        //Set default material
        if (m_configuration.m_material.GetAssetPath().empty())
        {
            m_configuration.m_material.SetAssetPath("engineassets/materials/decals/default.mtl");
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        SDecalProperties decalProperties = m_configuration.GetDecalProperties(transform);
        m_decalRenderNode->SetDecalProperties(decalProperties);

        m_renderFlags |= ERF_COMPONENT_ENTITY;
        m_decalRenderNode->SetRndFlags(m_renderFlags);
        m_decalRenderNode->SetMatrix(AZTransformToLYTransform(transform));
        m_decalRenderNode->SetMinSpec(static_cast<int>(decalProperties.m_minSpec));
        m_decalRenderNode->SetMaterialLayers(m_materialLayersMask);
        m_decalRenderNode->SetViewDistanceMultiplier(m_configuration.m_viewDistanceMultiplier);
    }

    void EditorDecalComponent::SetPrimaryAsset(const AZ::Data::AssetId& id)
    {
        AZStd::string assetPath;
        EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, id);
        m_configuration.m_material.SetAssetPath(assetPath.c_str());
        RefreshDecal();
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, AddDirtyEntity, GetEntityId());
    }

    void EditorDecalComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // Displays a small grid over the area where the decal will be applied.
        if (!IsSelected())
        {
            return;
        }

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        AZ::Vector3 x1 = transform.TransformPoint(AZ::Vector3(-1, 0, 0));
        AZ::Vector3 x2 = transform.TransformPoint(AZ::Vector3(1, 0, 0));
        AZ::Vector3 y1 = transform.TransformPoint(AZ::Vector3(0, -1, 0));
        AZ::Vector3 y2 = transform.TransformPoint(AZ::Vector3(0, 1, 0));
        AZ::Vector3 p = transform.TransformPoint(AZ::Vector3(0, 0, 0));
        AZ::Vector3 n = transform.TransformPoint(AZ::Vector3(0, 0, 1));

        debugDisplay.SetColor(AZ::Vector4(1, 0, 0, 1));

        debugDisplay.DrawLine(p, n);
        debugDisplay.DrawLine(x1, x2);
        debugDisplay.DrawLine(y1, y2);

        AZ::Vector3 p0 = transform.TransformPoint(AZ::Vector3(-1, -1, 0));
        AZ::Vector3 p1 = transform.TransformPoint(AZ::Vector3(-1, 1, 0));
        AZ::Vector3 p2 = transform.TransformPoint(AZ::Vector3(1, 1, 0));
        AZ::Vector3 p3 = transform.TransformPoint(AZ::Vector3(1, -1, 0));

        debugDisplay.DrawLine(p0, p1);
        debugDisplay.DrawLine(p1, p2);
        debugDisplay.DrawLine(p2, p3);
        debugDisplay.DrawLine(p3, p0);
        debugDisplay.DrawLine(p0, p2);
        debugDisplay.DrawLine(p1, p3);
    }

    void EditorDecalComponent::OnEntityVisibilityChanged(bool /*visibility*/)
    {
        RefreshDecal();
    }

    IRenderNode* EditorDecalComponent::GetRenderNode()
    {
        return m_decalRenderNode;
    }

    float EditorDecalComponent::GetRenderNodeRequestBusOrder() const
    {
        return DecalComponent::s_renderNodeRequestBusOrder;
    }

    void EditorDecalComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        RefreshDecal();
    }

    void EditorDecalComponent::SetMaterial(_smart_ptr<IMaterial> material)
    {
        if (material)
        {
            m_configuration.m_material.SetAssetPath(material->GetName());
        }
        else
        {
            m_configuration.m_material.SetAssetPath("");
        }

        RefreshDecal();

        EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    _smart_ptr<IMaterial> EditorDecalComponent::GetMaterial()
    {
        return m_decalRenderNode->GetMaterial();
    }

    AZ::Aabb EditorDecalComponent::GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        return GetWorldBounds();
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

    bool EditorDecalComponent::EditorSelectionIntersectRayViewport(const AzFramework::ViewportInfo& /*viewportInfo*/, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 p0 = transform.TransformPoint(AZ::Vector3(-1, -1, 0));
        AZ::Vector3 p1 = transform.TransformPoint(AZ::Vector3(-1, 1, 0));
        AZ::Vector3 p2 = transform.TransformPoint(AZ::Vector3(1, 1, 0));
        AZ::Vector3 p3 = transform.TransformPoint(AZ::Vector3(1, -1, 0));
        float t{ 0.0f };

        bool hitResult = AZ::Intersect::IntersectRayQuad(src, dir, p0, p1, p2, p3, t) != 0;
        distance = t;

        return hitResult;
    }
} // namespace LmbrCentral
