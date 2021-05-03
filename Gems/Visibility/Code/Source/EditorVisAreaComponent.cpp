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
#include "EditorVisAreaComponent.h"
#include "EditorVisAreaComponentMode.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/Viewport/VertexContainerDisplay.h>
#include <Editor/Objects/BaseObject.h>

// Include files needed for writing DisplayEntity functions that access the DisplayContext directly.
#include <EditorCoreAPI.h>

#include "MathConversion.h"

namespace Visibility
{
    /*static*/ AZ::Color EditorVisAreaComponent::s_visAreaColor = AZ::Color(1.0f, 0.5f, 0.0f, 1.0f);

    void EditorVisAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("EditorVisAreaService", 0x4507d2ae));
        provided.push_back(AZ_CRC("VisAreaService", 0x0c063fb9));
        provided.push_back(AZ_CRC("VariableVertexContainerService", 0x70c58740));
        provided.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorVisAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorVisAreaComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("VariableVertexContainerService", 0x70c58740));
        incompatible.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorVisAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorVisAreaConfiguration, VisAreaConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorVisAreaConfiguration>("VisArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<VisAreaConfiguration>("VisArea Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_height, "Height", "How tall the VisArea is.")
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeHeight)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_displayFilled, "DisplayFilled", "Display the VisArea as a filled volume.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeDisplayFilled)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_affectedBySun, "AffectedBySun", "Allows sunlight to affect objects inside the VisArea.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeAffectedBySun)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_viewDistRatio, "ViewDistRatio", "Specifies how far the VisArea is rendered.")
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeViewDistRatio)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_oceanIsVisible, "OceanIsVisible", "Ocean will be visible when looking outside the VisArea.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeOceanIsVisible)
                    // note: This will not work as expected. See Activate where we set the callbacks on the vertex container directly
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisAreaConfiguration::m_vertexContainer, "Vertices", "Points that make up the floor of the VisArea.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VisAreaConfiguration::ChangeVertexContainer)
                        ;
            }
        }
    }

    void EditorVisAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorVisAreaComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("m_config", &EditorVisAreaComponent::m_config)
                ->Field("ComponentMode", &EditorVisAreaComponent::m_componentModeDelegate)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorVisAreaComponent>("VisArea", "An area where only objects inside the area will be visible.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/VisArea.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/VisArea.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/vis-area-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorVisAreaComponent::m_config, "m_config", "No Description")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorVisAreaComponent::m_componentModeDelegate, "Component Mode", "VisArea Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorVisAreaComponentRequestBus>("EditorVisAreaComponentRequestBus")
                ->Event("SetHeight", &EditorVisAreaComponentRequestBus::Events::SetHeight)
                ->Event("GetHeight", &EditorVisAreaComponentRequestBus::Events::GetHeight)
                ->VirtualProperty("Height", "GetHeight", "SetHeight")

                ->Event("SetDisplayFilled", &EditorVisAreaComponentRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &EditorVisAreaComponentRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetAffectedBySun", &EditorVisAreaComponentRequestBus::Events::SetAffectedBySun)
                ->Event("GetAffectedBySun", &EditorVisAreaComponentRequestBus::Events::GetAffectedBySun)
                ->VirtualProperty("AffectedBySun", "GetAffectedBySun", "SetAffectedBySun")

                ->Event("SetViewDistRatio", &EditorVisAreaComponentRequestBus::Events::SetViewDistRatio)
                ->Event("GetViewDistRatio", &EditorVisAreaComponentRequestBus::Events::GetViewDistRatio)
                ->VirtualProperty("ViewDistRatio", "GetViewDistRatio", "SetViewDistRatio")

                ->Event("SetOceanIsVisible", &EditorVisAreaComponentRequestBus::Events::SetOceanIsVisible)
                ->Event("GetOceanIsVisible", &EditorVisAreaComponentRequestBus::Events::GetOceanIsVisible)
                ->VirtualProperty("OceanIsVisible", "GetOceanIsVisible", "SetOceanIsVisible")
                ;

            behaviorContext->Class<EditorVisAreaComponent>()->RequestBus("EditorVisAreaComponentRequestBus");
        }

        EditorVisAreaConfiguration::Reflect(context);
    }

    void EditorVisAreaConfiguration::ChangeHeight()
    {
        EditorVisAreaComponentRequestBus::Event(
            m_entityId, &EditorVisAreaComponentRequests::UpdateVisAreaObject);
    }

    void EditorVisAreaConfiguration::ChangeDisplayFilled()
    {
        EditorVisAreaComponentRequestBus::Event(
            m_entityId, &EditorVisAreaComponentRequests::UpdateVisAreaObject);
    }

    void EditorVisAreaConfiguration::ChangeAffectedBySun()
    {
        EditorVisAreaComponentRequestBus::Event(
            m_entityId, &EditorVisAreaComponentRequests::UpdateVisAreaObject);
    }

    void EditorVisAreaConfiguration::ChangeViewDistRatio()
    {
        EditorVisAreaComponentRequestBus::Event(
            m_entityId, &EditorVisAreaComponentRequests::UpdateVisAreaObject);
    }

    void EditorVisAreaConfiguration::ChangeOceanIsVisible()
    {
        EditorVisAreaComponentRequestBus::Event(
            m_entityId, &EditorVisAreaComponentRequests::UpdateVisAreaObject);
    }

    void EditorVisAreaConfiguration::ChangeVertexContainer()
    {
        EditorVisAreaComponentRequestBus::Event(
            m_entityId, &EditorVisAreaComponentRequests::UpdateVisAreaObject);
    }

    void EditorVisAreaConfiguration::SetEntityId(const AZ::EntityId entityId)
    {
        m_entityId = entityId;
    }

    EditorVisAreaComponent::~EditorVisAreaComponent()
    {
        if (m_area && GetIEditor()->Get3DEngine())
        {
            // Reset the listener vis area in the unlucky case that we are deleting the
            // vis area where the listener is currently in
            GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
            m_area = nullptr;
        }
    }

    void EditorVisAreaComponent::Activate()
    {
        Base::Activate();

        const AZ::EntityId entityId = GetEntityId();

        // NOTE: We create the vis-area here at activated, but destroy it in the destructor.
        // We have to do this, otherwise the vis-area is not saved into the level.
        // Unfortunately, at this time we cannot create the vis-areas at game runtime.
        // This means that dynamic slices cannot effectively contain vis areas until we fix the core rendering system to allow that.

        const auto visGUID = AZ::u64(entityId);
        if(!m_area && GetIEditor() && GetIEditor()->Get3DEngine())
        {
            m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);
        }

        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorVisAreaComponent, EditorVisAreaComponentMode>(
                AZ::EntityComponentIdPair(entityId, GetId()), this);

        // give default values to the vertices if needed
        if (m_config.m_vertexContainer.Size() == 0)
        {
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(-1.0f, -1.0f, 0.0f));
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(+1.0f, -1.0f, 0.0f));
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(+1.0f, +1.0f, 0.0f));
            m_config.m_vertexContainer.AddVertex(AZ::Vector3(-1.0f, +1.0f, 0.0f));
        }

        const auto vertexAdded = [this](size_t vertIndex)
        {
            EditorVisAreaComponentNotificationBus::Event(
                GetEntityId(), &EditorVisAreaComponentNotifications::OnVertexAdded, vertIndex);

            UpdateVisAreaObject();
        };

        const auto vertexRemoved = [this](size_t vertIndex)
        {
            EditorVisAreaComponentNotificationBus::Event(
                GetEntityId(), &EditorVisAreaComponentNotifications::OnVertexRemoved, vertIndex);

            UpdateVisAreaObject();
        };

        const auto vertexChanged = [this](size_t vertIndex)
        {
            EditorVisAreaComponentNotificationBus::Event(
                GetEntityId(), &EditorVisAreaComponentNotifications::OnVertexUpdated, vertIndex);

            UpdateVisAreaObject();
        };

        const auto verticesSet = [this]()
        {
            EditorVisAreaComponentNotificationBus::Event(
                GetEntityId(), &EditorVisAreaComponentNotifications::OnVerticesSet,
                m_config.m_vertexContainer.GetVertices());

            UpdateVisAreaObject();
        };

        const auto verticesCleared = [this]()
        {
            EditorVisAreaComponentNotificationBus::Event(
                GetEntityId(), &EditorVisAreaComponentNotifications::OnVerticesCleared);

            UpdateVisAreaObject();
        };

        m_config.m_vertexContainer.SetCallbacks(
            vertexAdded, vertexRemoved, vertexChanged,
            verticesSet, verticesCleared);

        m_config.SetEntityId(entityId);

        AZ::TransformBus::EventResult(
            m_currentWorldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        // make the initial vis area with the data we just parsed
        UpdateVisAreaObject();

        EditorVisAreaComponentRequestBus::Handler::BusConnect(entityId);
        AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusConnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);
    }

    void EditorVisAreaComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        EditorVisAreaComponentRequestBus::Handler::BusDisconnect();

        Base::Deactivate();
    }

    void EditorVisAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        UpdateVisAreaObject();
    }

    /// Apply the component's settings to the underlying vis area
    void EditorVisAreaComponent::UpdateVisAreaObject()
    {
        if (m_area)
        {
            std::vector<Vec3> points;

            const AZStd::vector<AZ::Vector3>& vertices = m_config.m_vertexContainer.GetVertices();
            const size_t vertexCount = vertices.size();

            if (vertexCount > 3)
            {
                const Matrix34& wtm = AZTransformToLYTransform(GetWorldTM());
                points.resize(vertexCount);
                for (size_t i = 0; i < vertexCount; i++)
                {
                    points[i] = wtm.TransformPoint(AZVec3ToLYVec3(vertices[i]));
                }

                SVisAreaInfo info;
                info.fHeight = GetHeight();
                info.bAffectedByOutLights = m_config.m_affectedBySun;
                info.fViewDistRatio = m_config.m_viewDistRatio;
                info.bOceanIsVisible = m_config.m_oceanIsVisible;

                //Unconfigurable; these values are used by other area types
                //We set them just so that debugging later it's clear that these
                //aren't being used because this is a VisArea.
                info.fPortalBlending = -1;
                info.bDoubleSide = true;
                info.bUseDeepness = false;
                info.bUseInIndoors = false;

                const AZStd::string name = AZStd::string("vis-area_") + GetEntity()->GetName();

                if (GetIEditor()->Get3DEngine())
                {
                    GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &points[0], points.size(), name.c_str(), info, true);
                }

                AzFramework::EntityBoundsUnionRequestBus::Broadcast(
                    &AzFramework::EntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
            }
        }
    }

    void EditorVisAreaComponent::SetHeight(const float value)
    {
        m_config.m_height = value;
        UpdateVisAreaObject();
    }

    float EditorVisAreaComponent::GetHeight()
    {
        return m_config.m_height;
    }

    void EditorVisAreaComponent::SetDisplayFilled(const bool value)
    {
        m_config.m_displayFilled = value;
        UpdateVisAreaObject();
    }

    bool EditorVisAreaComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    void EditorVisAreaComponent::SetAffectedBySun(const bool value)
    {
        m_config.m_affectedBySun = value;
        UpdateVisAreaObject();
    }

    bool EditorVisAreaComponent::GetAffectedBySun()
    {
        return m_config.m_affectedBySun;
    }

    void EditorVisAreaComponent::SetViewDistRatio(const float value)
    {
        m_config.m_viewDistRatio = value;
        UpdateVisAreaObject();
    }

    float EditorVisAreaComponent::GetViewDistRatio()
    {
        return m_config.m_viewDistRatio;
    }

    void EditorVisAreaComponent::SetOceanIsVisible(const bool value)
    {
        m_config.m_oceanIsVisible = value;
        UpdateVisAreaObject();
    }

    bool EditorVisAreaComponent::GetOceanIsVisible()
    {
        return m_config.m_oceanIsVisible;
    }

    bool EditorVisAreaComponent::GetVertex(size_t index, AZ::Vector3& vertex) const
    {
        return m_config.m_vertexContainer.GetVertex(index, vertex);
    }

    void EditorVisAreaComponent::AddVertex(const AZ::Vector3& vertex)
    {
        m_config.m_vertexContainer.AddVertex(vertex);
        UpdateVisAreaObject();
    }

    bool EditorVisAreaComponent::UpdateVertex(size_t index, const AZ::Vector3& vertex)
    {
        if (m_config.m_vertexContainer.UpdateVertex(index, vertex))
        {
            UpdateVisAreaObject();
            return true;
        }

        return false;
    }

    bool EditorVisAreaComponent::InsertVertex(size_t index, const AZ::Vector3& vertex)
    {
        if (m_config.m_vertexContainer.InsertVertex(index, vertex))
        {
            UpdateVisAreaObject();
            return true;
        }

        return false;
    }

    bool EditorVisAreaComponent::RemoveVertex(size_t index)
    {
        if (m_config.m_vertexContainer.RemoveVertex(index))
        {
            UpdateVisAreaObject();
            return true;
        }

        return false;
    }

    void EditorVisAreaComponent::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_config.m_vertexContainer.SetVertices(vertices);
        UpdateVisAreaObject();
    }

    void EditorVisAreaComponent::ClearVertices()
    {
        m_config.m_vertexContainer.Clear();
        UpdateVisAreaObject();
    }

    size_t EditorVisAreaComponent::Size() const
    {
        return m_config.m_vertexContainer.Size();
    }

    bool EditorVisAreaComponent::Empty() const
    {
        return m_config.m_vertexContainer.Empty();
    }

    void EditorVisAreaComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        /*
            The VisArea and Portal share a common strangeness with how they are displayed.
            The Legacy visualization is actually incorrect! It's important to know that
            the vis volumes essentially act as a points on an XY plane with a known Z
            position and a height. The volumes that actually affect rendition are planar
            quads with a height. The height is calculated by the largest height value on
            a local point + the given height value on the component.

            Also note that this visualization does not display the floors or ceilings of the vis area
            volumes. There is currently no way with the display context to easily draw a filled polygon.
            We could try to draw some triangles but it would take up a great deal of rendering
            time and could potentially slow down the editor more than we want if there are a lot
            of volumes.
        */

        const AZStd::vector<AZ::Vector3>& vertices = m_config.m_vertexContainer.GetVertices();
        const size_t vertexCount = vertices.size();

        //We do not want to push a transform with scale or rotation as the
        //vis area is always snapped to the XY plane with a height.
        //Scale will be applied during flattening
        AZ::Transform translation = AZ::Transform::CreateIdentity();
        translation.SetTranslation(m_currentWorldTransform.GetTranslation());

        debugDisplay.PushMatrix(translation);
        debugDisplay.SetColor(s_visAreaColor.GetAsVector4());

        AZStd::vector<AZ::Vector3> transformedPoints;
        transformedPoints.resize(vertexCount);

        //Min and max Z value (in local space)
        float minZ = FLT_MAX;
        float maxZ = FLT_MIN;

        //Apply rotation and scale before removing translation.
        //We want translation to apply with the matrix to make things easier
        //but we need to calculate a difference in Z after rotation and scaling.
        //During the next loop we'll flatten all these points down to a common XY plane
        for (size_t i = 0; i < vertexCount; ++i)
        {
            AZ::Vector3 transformedPoint = m_currentWorldTransform.TransformPoint(vertices[i]);

            //Back into local space
            transformedPoints[i] = transformedPoint - m_currentWorldTransform.GetTranslation();

            const float transformedZ = transformedPoints[i].GetZ();

            minZ = AZ::GetMin(transformedZ, minZ);
            maxZ = AZ::GetMax(transformedZ, maxZ);
        }

        //The height of the vis area + the max local height
        const float actualHeight = m_config.m_height + maxZ;

        //Draw walls for every line segment
        size_t transformedPointCount = transformedPoints.size();
        for (size_t i = 0; i < transformedPointCount; ++i)
        {
            AZ::Vector3 lowerLeft   = AZ::Vector3();
            AZ::Vector3 lowerRight  = AZ::Vector3();
            AZ::Vector3 upperRight  = AZ::Vector3();
            AZ::Vector3 upperLeft   = AZ::Vector3();

            lowerLeft = transformedPoints[i];
            lowerRight = transformedPoints[(i + 1) % transformedPointCount];
            //The mod with transformedPointCount ensures that for the last vert it will connect back with vert 0
            //If vert count is 10, the last vert will be i = 9 and we want that to create a surface with vert 0

            //Make all lower points planar
            lowerLeft.SetZ(minZ);
            lowerRight.SetZ(minZ);

            upperRight = AZ::Vector3(lowerRight.GetX(), lowerRight.GetY(), actualHeight);
            upperLeft = AZ::Vector3(lowerLeft.GetX(), lowerLeft.GetY(), actualHeight);

            if (m_config.m_displayFilled)
            {
                debugDisplay.SetAlpha(0.3f);
                //Draw filled quad with two winding orders to make it double sided
                debugDisplay.DrawQuad(lowerLeft, lowerRight, upperRight, upperLeft);
                debugDisplay.DrawQuad(lowerLeft, upperLeft, upperRight, lowerRight);
            }

            debugDisplay.SetAlpha(1.0f);
            debugDisplay.DrawLine(lowerLeft, lowerRight);
            debugDisplay.DrawLine(lowerRight, upperRight);
            debugDisplay.DrawLine(upperRight, upperLeft);
            debugDisplay.DrawLine(upperLeft, lowerLeft);
        }

        if (m_componentModeDelegate.AddedToComponentMode())
        {
            AzToolsFramework::VertexContainerDisplay::DisplayVertexContainerIndices(
                debugDisplay, AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector3>(m_config.m_vertexContainer),
                GetWorldTM(), AZ::Vector3::CreateOne(), IsSelected());
        }

        debugDisplay.PopMatrix();
    }

    void EditorVisAreaComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<VisAreaComponent>(m_config);
    }

    AZ::Aabb EditorVisAreaComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        return GetWorldBounds();
    }

    bool EditorVisAreaComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src,
        const AZ::Vector3& dir, float& distance)
    {
        AZ::Aabb bbox = GetEditorSelectionBoundsViewport(viewportInfo);
        float end;
        float t;

        const bool intersection = AZ::Intersect::IntersectRayAABB2(
            src, dir.GetReciprocal(),
            bbox, t, end) > 0;

        if (intersection)
        {
            distance = t;
        }

        return intersection;
    }

    AZ::Aabb EditorVisAreaComponent::GetWorldBounds()
    {
        return GetLocalBounds().GetTransformedAabb(GetWorldTM());
    }

    AZ::Aabb EditorVisAreaComponent::GetLocalBounds()
    {
        AZ::Aabb bbox = AZ::Aabb::CreateNull();
        for (const auto& vertex : m_config.m_vertexContainer.GetVertices())
        {
            bbox.AddPoint(vertex);
        }
        bbox.AddPoint(bbox.GetMax() + AZ::Vector3::CreateAxisZ(m_config.m_height));
        return bbox;
    }
} //namespace Visibility
