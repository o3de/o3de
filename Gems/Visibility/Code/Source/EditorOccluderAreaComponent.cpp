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

#include "Visibility_precompiled.h"
#include "EditorOccluderAreaComponent.h"
#include "EditorOccluderAreaComponentMode.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/Viewport/VertexContainerDisplay.h>
#include <Editor/Objects/BaseObject.h>
#include <Objects/EntityObject.h>

// Include files needed for writing DisplayEntity functions that access the DisplayContext directly.
#include <EditorCoreAPI.h>

#include "MathConversion.h"

namespace Visibility
{
    void EditorOccluderAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("EditorOccluderAreaService", 0xf943e16a));
        provides.push_back(AZ_CRC("OccluderAreaService", 0x2fefad66));
        provides.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorOccluderAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorOccluderAreaComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("QuadShapeService", 0xe449b0fc));
    }

    void EditorOccluderAreaComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("EditorOccluderAreaService", 0xf943e16a));
        incompatible.push_back(AZ_CRC("OccluderAreaService", 0x2fefad66));
        incompatible.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorOccluderAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorOccluderAreaConfiguration, OccluderAreaConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorOccluderAreaConfiguration>("OccluderArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<OccluderAreaConfiguration>("OccluderArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_displayFilled, "DisplayFilled", "Display the Occlude Area as a filled quad.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_cullDistRatio, "CullDistRatio", "The range of the culling effect.")
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_useInIndoors, "UseInIndoors", "Should this occluder work inside VisAreas.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_doubleSide, "DoubleSide", "Should this occlude from both sides.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnChange)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OccluderAreaConfiguration::m_vertices, "Vertices", "Points that make up the OccluderArea.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OccluderAreaConfiguration::OnVerticesChange)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void EditorOccluderAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorOccluderAreaComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("m_config", &EditorOccluderAreaComponent::m_config)
                ->Field("ComponentMode", &EditorOccluderAreaComponent::m_componentModeDelegate)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorOccluderAreaComponent>("OccluderArea", "An area that blocks objects behind it from rendering.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/OccluderArea.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/OccluderArea.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/occluder-area-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorOccluderAreaComponent::m_config, "m_config", "No Description")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorOccluderAreaComponent::m_componentModeDelegate, "Component Mode", "OccluderArea Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorOccluderAreaRequestBus>("EditorOccluderAreaRequestBus")
                ->Event("SetDisplayFilled", &EditorOccluderAreaRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &EditorOccluderAreaRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetCullDistRatio", &EditorOccluderAreaRequestBus::Events::SetCullDistRatio)
                ->Event("GetCullDistRatio", &EditorOccluderAreaRequestBus::Events::GetCullDistRatio)
                ->VirtualProperty("CullDistRatio", "GetCullDistRatio", "SetCullDistRatio")

                ->Event("SetUseInIndoors", &EditorOccluderAreaRequestBus::Events::SetUseInIndoors)
                ->Event("GetUseInIndoors", &EditorOccluderAreaRequestBus::Events::GetUseInIndoors)
                ->VirtualProperty("UseInIndoors", "GetUseInIndoors", "SetUseInIndoors")

                ->Event("SetDoubleSide", &EditorOccluderAreaRequestBus::Events::SetDoubleSide)
                ->Event("GetDoubleSide", &EditorOccluderAreaRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", "SetDoubleSide")
                ;

            behaviorContext->Class<EditorOccluderAreaComponent>()->RequestBus("EditorOccluderAreaRequestBus");
        }

        EditorOccluderAreaConfiguration::Reflect(context);
    }

    void EditorOccluderAreaConfiguration::OnChange()
    {
        EditorOccluderAreaRequestBus::Event(m_entityId, &EditorOccluderAreaRequests::UpdateOccluderAreaObject);
    }

    void EditorOccluderAreaConfiguration::OnVerticesChange()
    {
        EditorOccluderAreaRequestBus::Event(
            m_entityId, &EditorOccluderAreaRequests::UpdateOccluderAreaObject);
        EditorOccluderAreaNotificationBus::Event(
            m_entityId, &EditorOccluderAreaNotifications::OnVerticesChangedInspector);
    }

    void EditorOccluderAreaConfiguration::SetEntityId(const AZ::EntityId entityId)
    {
        m_entityId = entityId;
    }

    EditorOccluderAreaComponent::~EditorOccluderAreaComponent()
    {
        if (m_area)
        {
            GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
            m_area = nullptr;
        }
    }

    void EditorOccluderAreaComponent::Activate()
    {
        Base::Activate();

        const AZ::EntityId entityId = GetEntityId();
        m_config.SetEntityId(entityId);

        // NOTE: We create the vis-area here at activate, but destroy it in the destructor.
        // We have to do this, otherwise the vis-area is not saved into the level.
        // Unfortunately, at this time we cannot create the vis-areas at game runtime.
        // This means that dynamic slices cannot effectively contain vis areas until we fix
        // the core rendering system to allow that.
        const auto visGUID = static_cast<AZ::u64>(entityId);
        if(!m_area && GetIEditor())
        {
            m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);
        }

        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorOccluderAreaComponent, EditorOccluderAreaComponentMode>(
                AZ::EntityComponentIdPair(entityId, GetId()), this);

        EditorOccluderAreaRequestBus::Handler::BusConnect(entityId);
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);

        UpdateOccluderAreaObject();
    }

    void EditorOccluderAreaComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        EditorOccluderAreaRequestBus::Handler::BusDisconnect();

        Base::Deactivate();
    }

    /// Update the object runtime after changes to the Configuration.
    /// Called by the default RequestBus SetXXX implementations,
    /// and used to initially set up the object the first time the
    /// Configuration are set.
    void EditorOccluderAreaComponent::UpdateOccluderAreaObject()
    {
        if (m_area)
        {
            AZStd::array<Vec3, 4> verts;

            const Matrix34& wtm = AZTransformToLYTransform(GetWorldTM());
            for (size_t i = 0; i < m_config.m_vertices.size(); ++i)
            {
                verts[i] = wtm.TransformPoint(AZVec3ToLYVec3(m_config.m_vertices[i]));
            }

            SVisAreaInfo info;
            info.fHeight = 0;
            info.vAmbientColor = Vec3(0, 0, 0);
            info.bAffectedByOutLights = false;
            info.bSkyOnly = false;
            info.fViewDistRatio = m_config.m_cullDistRatio;
            info.bDoubleSide = m_config.m_doubleSide;
            info.bUseDeepness = false;
            info.bUseInIndoors = m_config.m_useInIndoors;
            info.bOceanIsVisible = false;
            info.fPortalBlending = -1.0f;

            const AZStd::string name = AZStd::string("OcclArea_") + GetEntity()->GetName();
            GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &verts[0], verts.size(), name.c_str(), info, false);

            AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(GetEntityId());
        }
    }

    void EditorOccluderAreaComponent::SetDisplayFilled(const bool value)
    {
        m_config.m_displayFilled = value;
        UpdateOccluderAreaObject();
    }

    bool EditorOccluderAreaComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    void EditorOccluderAreaComponent::SetCullDistRatio(const float value)
    {
        m_config.m_cullDistRatio = value;
        UpdateOccluderAreaObject();
    }

    float EditorOccluderAreaComponent::GetCullDistRatio()
    {
        return m_config.m_cullDistRatio;
    }

    void EditorOccluderAreaComponent::SetUseInIndoors(const bool value)
    {
        m_config.m_useInIndoors = value;
        UpdateOccluderAreaObject();
    }

    bool EditorOccluderAreaComponent::GetUseInIndoors()
    {
        return m_config.m_useInIndoors;
    }

    void EditorOccluderAreaComponent::SetDoubleSide(const bool value)
    {
        m_config.m_doubleSide = value;
        UpdateOccluderAreaObject();
    }

    bool EditorOccluderAreaComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

    bool EditorOccluderAreaComponent::GetVertex(const size_t index, AZ::Vector3& vertex) const
    {
        if (index < m_config.m_vertices.size())
        {
            vertex = m_config.m_vertices[index];
            return true;
        }

        return false;
    }

    bool EditorOccluderAreaComponent::UpdateVertex(const size_t index, const AZ::Vector3& vertex)
    {
        if (index < m_config.m_vertices.size())
        {
            m_config.m_vertices[index] = vertex;
            return true;
        }

        return false;
    }

    void EditorOccluderAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, [[maybe_unused]] const AZ::Transform& world)
    {
        UpdateOccluderAreaObject();
    }

    void EditorOccluderAreaComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AZ::Transform worldFromLocal = GetWorldTM();
        const AZ::Vector4 color(0.5f, 0.25f, 0.0f, 1.0f);
        const AZ::Vector4 selectedColor(1.0f, 0.5f, 0.0f, 1.0f);
        const float previousLineWidth = debugDisplay.GetLineWidth();

        debugDisplay.DepthWriteOff();
        debugDisplay.PushMatrix(worldFromLocal);
        debugDisplay.SetColor(IsSelected() ? selectedColor : color);
        debugDisplay.SetLineWidth(5.0f);
        debugDisplay.SetAlpha(0.8f);

        for (size_t i = 2; i < 4; i++)
        {
            // draw the plane
            if (m_config.m_displayFilled)
            {
                debugDisplay.SetAlpha(0.3f);
                debugDisplay.CullOff();
                debugDisplay.DrawTri(m_config.m_vertices[0], m_config.m_vertices[i - 1], m_config.m_vertices[i]);
                debugDisplay.CullOn();
                debugDisplay.SetAlpha(0.8f);
            }

            debugDisplay.DrawLine(m_config.m_vertices[i - 2], m_config.m_vertices[i - 1]);
            debugDisplay.DrawLine(m_config.m_vertices[i - 1], m_config.m_vertices[i]);
        }

        // draw the closing line
        debugDisplay.DrawLine(m_config.m_vertices[3], m_config.m_vertices[0]);

        if (m_componentModeDelegate.AddedToComponentMode())
        {
            AzToolsFramework::VertexContainerDisplay::DisplayVertexContainerIndices(
                debugDisplay, AzToolsFramework::FixedVerticesArray<AZ::Vector3, 4>(m_config.m_vertices),
                GetWorldTM(), AZ::Vector3::CreateOne(), IsSelected());
        }

        debugDisplay.DepthWriteOn();
        debugDisplay.SetLineWidth(previousLineWidth);
        debugDisplay.PopMatrix();
    }

    void EditorOccluderAreaComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<OccluderAreaComponent>(m_config);
    }

    AZ::Aabb EditorOccluderAreaComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        return GetWorldBounds();
    }

    bool EditorOccluderAreaComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/, const AZ::Vector3& src,
        const AZ::Vector3& dir, float& distance)
    {
        const float rayLength = 1000.0f;
        AZ::Vector3 scaledDir = dir * rayLength;
        AZ::Vector3 end = src + scaledDir;
        float t;
        float intermediateT = std::numeric_limits<float>::max();
        bool didHit = false;

        // Transform verts to world space for tris test
        AZStd::array<AZ::Vector3, 4> verts;
        const AZ::Transform& wtm = GetWorldTM();
        for (size_t i = 0; i < m_config.m_vertices.size(); ++i)
        {
            verts[i] = wtm.TransformPoint(m_config.m_vertices[i]);
        }

        AZ::Vector3 normal;
        for (AZ::u8 i = 2; i < 4; ++i)
        {
            if (AZ::Intersect::IntersectSegmentTriangleCCW(src, end, verts[0], verts[i - 1], verts[i], normal, t) > 0)
            {
                intermediateT = AZStd::GetMin(t, intermediateT);
                didHit = true;
            }
            //Else if here as we shouldn't successfully ccw and cw intersect at the same time
            else if (AZ::Intersect::IntersectSegmentTriangle(src, end, verts[0], verts[i - 1], verts[i], normal, t) > 0)
            {
                intermediateT = AZStd::GetMin(t, intermediateT);
                didHit = true;
            }
        }

        if (didHit)
        {
            distance = intermediateT * rayLength;
        }
        return didHit;
    }

    AZ::Aabb EditorOccluderAreaComponent::GetWorldBounds()
    {
        return GetLocalBounds().GetTransformedAabb(GetWorldTM());
    }

    AZ::Aabb EditorOccluderAreaComponent::GetLocalBounds()
    {
        AZ::Aabb bbox = AZ::Aabb::CreateNull();
        for (const auto& vertex : m_config.m_vertices)
        {
            bbox.AddPoint(vertex);
        }
        return bbox;
    }
} // namespace Visibility
