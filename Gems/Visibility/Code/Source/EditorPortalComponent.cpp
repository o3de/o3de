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
#include "EditorPortalComponent.h"
#include "EditorPortalComponentMode.h"

#include <AzCore/RTTI/BehaviorContext.h>

// Include files needed for writing DisplayEntity functions that access the DisplayContext directly.
#include <EditorCoreAPI.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzToolsFramework/Viewport/VertexContainerDisplay.h>
#include <Editor/Objects/BaseObject.h>
#include <MathConversion.h>

namespace Visibility
{
    void EditorPortalComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("EditorPortalService", 0x6ead38f6));
        provided.push_back(AZ_CRC("PortalService", 0x06076210));
        provided.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorPortalComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorPortalComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("QuadShapeService", 0xe449b0fc));
    }

    void EditorPortalComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        incompatible.push_back(AZ_CRC("SplineShapeService", 0x4d4b94a2));
        incompatible.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
        incompatible.push_back(AZ_CRC("FixedVertexContainerService", 0x83f1bbf2));
    }

    void EditorPortalConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPortalConfiguration, PortalConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorPortalConfiguration>("Portal Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<PortalConfiguration>("Portal Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_height, "Height", "How tall the Portal is.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_displayFilled, "DisplayFilled", "Display the Portal as a filled volume.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_affectedBySun, "AffectedBySun", "Allows sunlight to affect objects inside the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_viewDistRatio, "ViewDistRatio", "Specifies how far the Portal is rendered.")
                    ->Attribute(AZ::Edit::Attributes::Max, 100.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_skyOnly, "SkyOnly", "Only the Sky Box will render when looking outside the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_oceanIsVisible, "OceanIsVisible", "Ocean will be visible when looking outside the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_useDeepness, "UseDeepness", "Portal will be treated as an object with volume rather than a plane.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_doubleSide, "DoubleSide", "Cameras will be able to look through the portal from both sides.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_lightBlending, "LightBlending", "Light from neighboring VisAreas will blend into the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_lightBlendValue, "LightBlendValue", "How much to blend lights from neighboring VisAreas.")
                    ->Attribute(AZ::Edit::Attributes::Max, 1.000000)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.000000)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &PortalConfiguration::m_vertices, "Vertices", "Points that make up the floor of the Portal.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PortalConfiguration::OnVerticesChange)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorPortalComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPortalComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("m_config", &EditorPortalComponent::m_config)
                ->Field("ComponentMode", &EditorPortalComponent::m_componentModeDelegate)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorPortalComponent>("Portal", "An area that describes a visibility portal between VisAreas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Portal.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Portal.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/portal-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPortalComponent::m_config, "m_config", "No Description")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPortalComponent::m_componentModeDelegate, "Component Mode", "Portal Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorPortalRequestBus>("EditorPortalRequestBus")
                ->Event("SetHeight", &EditorPortalRequestBus::Events::SetHeight)
                ->Event("GetHeight", &EditorPortalRequestBus::Events::GetHeight)
                ->VirtualProperty("Height", "GetHeight", "SetHeight")

                ->Event("SetDisplayFilled", &EditorPortalRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &EditorPortalRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetAffectedBySun", &EditorPortalRequestBus::Events::SetAffectedBySun)
                ->Event("GetAffectedBySun", &EditorPortalRequestBus::Events::GetAffectedBySun)
                ->VirtualProperty("AffectedBySun", "GetAffectedBySun", "SetAffectedBySun")

                ->Event("SetViewDistRatio", &EditorPortalRequestBus::Events::SetViewDistRatio)
                ->Event("GetViewDistRatio", &EditorPortalRequestBus::Events::GetViewDistRatio)
                ->VirtualProperty("ViewDistRatio", "GetViewDistRatio", "SetViewDistRatio")

                ->Event("SetSkyOnly", &EditorPortalRequestBus::Events::SetSkyOnly)
                ->Event("GetSkyOnly", &EditorPortalRequestBus::Events::GetSkyOnly)
                ->VirtualProperty("SkyOnly", "GetSkyOnly", "SetSkyOnly")

                ->Event("SetOceanIsVisible", &EditorPortalRequestBus::Events::SetOceanIsVisible)
                ->Event("GetOceanIsVisible", &EditorPortalRequestBus::Events::GetOceanIsVisible)
                ->VirtualProperty("OceanIsVisible", "GetOceanIsVisible", "SetOceanIsVisible")

                ->Event("SetUseDeepness", &EditorPortalRequestBus::Events::SetUseDeepness)
                ->Event("GetUseDeepness", &EditorPortalRequestBus::Events::GetUseDeepness)
                ->VirtualProperty("UseDeepness", "GetUseDeepness", "SetUseDeepness")

                ->Event("SetDoubleSide", &EditorPortalRequestBus::Events::SetDoubleSide)
                ->Event("GetDoubleSide", &EditorPortalRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", "SetDoubleSide")

                ->Event("SetLightBlending", &EditorPortalRequestBus::Events::SetLightBlending)
                ->Event("GetLightBlending", &EditorPortalRequestBus::Events::GetLightBlending)
                ->VirtualProperty("LightBlending", "GetLightBlending", "SetLightBlending")

                ->Event("SetLightBlendValue", &EditorPortalRequestBus::Events::SetLightBlendValue)
                ->Event("GetLightBlendValue", &EditorPortalRequestBus::Events::GetLightBlendValue)
                ->VirtualProperty("LightBlendValue", "GetLightBlendValue", "SetLightBlendValue")
                ;

            behaviorContext->Class<EditorPortalComponent>()->RequestBus("EditorPortalRequestBus");
        }

        EditorPortalConfiguration::Reflect(context);
    }

    void EditorPortalConfiguration::OnChange()
    {
        EditorPortalRequestBus::Event(m_entityId, &EditorPortalRequests::UpdatePortalObject);
    }

    void EditorPortalConfiguration::OnVerticesChange()
    {
        EditorPortalRequestBus::Event(m_entityId, &EditorPortalRequests::UpdatePortalObject);
        EditorPortalNotificationBus::Event(m_entityId, &EditorPortalNotifications::OnVerticesChangedInspector);
    }

    void EditorPortalConfiguration::SetEntityId(const AZ::EntityId entityId)
    {
        m_entityId = entityId;
    }

    EditorPortalComponent::~EditorPortalComponent()
    {
        if (m_area && GetIEditor()->Get3DEngine())
        {
            // reset the listener vis area in the unlucky case that we are deleting the
            // vis area where the listener is currently in
            // Audio: do we still need this?
            GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
            m_area = nullptr;
        }
    }

    void EditorPortalComponent::Activate()
    {
        Base::Activate();

        const AZ::EntityId entityId = GetEntityId();
        m_config.SetEntityId(entityId);

        // NOTE: We create the vis-area here at activated, but destroy it in the destructor.
        // We have to do this, otherwise the vis-area is not saved into the level.
        // Unfortunately, at this time we cannot create the vis-areas at game runtime.
        // This means that dynamic slices cannot effectively contain vis-areas until we fix the core rendering system to allow that.

        const auto visGUID = static_cast<AZ::u64>(entityId);
        if(!m_area && GetIEditor() && GetIEditor()->Get3DEngine())
        {
            m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);
        }

        m_AZCachedWorldTransform = AZ::Transform::CreateIdentity();
        m_cryCachedWorldTransform = Matrix34::CreateIdentity();

        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorPortalComponent, EditorPortalComponentMode>(
                AZ::EntityComponentIdPair(entityId, GetId()), this);

        EditorPortalRequestBus::Handler::BusConnect(entityId);
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);

        //Call OnTransformChanged manually to cache current transform since it won't be called
        //automatically for us when the level starts up.
        AZ::Transform worldTM;
        AZ::TransformBus::EventResult(worldTM, entityId, &AZ::TransformBus::Events::GetWorldTM);

        //Use an identity transform for localTM because the
        //OnTransformChanged impl for this class doesn't need it
        OnTransformChanged(AZ::Transform::CreateIdentity(), worldTM);
    }

    void EditorPortalComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        EditorPortalRequestBus::Handler::BusDisconnect();

        Base::Deactivate();
    }

    void EditorPortalComponent::SetHeight(const float height)
    {
        m_config.m_height = height;
        UpdatePortalObject();
    }

    float EditorPortalComponent::GetHeight()
    {
        return m_config.m_height;
    }

    void EditorPortalComponent::SetDisplayFilled(const bool filled)
    {
        m_config.m_displayFilled = filled;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    void EditorPortalComponent::SetAffectedBySun(const bool affectedBySun)
    {
        m_config.m_affectedBySun = affectedBySun;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetAffectedBySun()
    {
        return m_config.m_affectedBySun;
    }

    void EditorPortalComponent::SetViewDistRatio(const float viewDistRatio)
    {
        m_config.m_viewDistRatio = viewDistRatio;
        UpdatePortalObject();
    }

    float EditorPortalComponent::GetViewDistRatio()
    {
        return m_config.m_viewDistRatio;
    }

    void EditorPortalComponent::SetSkyOnly(const bool skyOnly)
    {
        m_config.m_skyOnly = skyOnly;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetSkyOnly()
    {
        return m_config.m_skyOnly;
    }

    void EditorPortalComponent::SetOceanIsVisible(const bool oceanVisible)
    {
        m_config.m_oceanIsVisible = oceanVisible;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetOceanIsVisible()
    {
        return m_config.m_oceanIsVisible;
    }

    void EditorPortalComponent::SetUseDeepness(const bool useDeepness)
    {
        m_config.m_useDeepness = useDeepness;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetUseDeepness()
    {
        return m_config.m_useDeepness;
    }

    void EditorPortalComponent::SetDoubleSide(const bool doubleSided)
    {
        m_config.m_doubleSide = doubleSided;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

    void EditorPortalComponent::SetLightBlending(const bool lightBending)
    {
        m_config.m_lightBlending = lightBending;
        UpdatePortalObject();
    }

    bool EditorPortalComponent::GetLightBlending()
    {
        return m_config.m_lightBlending;
    }

    void EditorPortalComponent::SetLightBlendValue(const float lightBendAmount)
    {
        m_config.m_lightBlendValue = lightBendAmount;
        UpdatePortalObject();
    }

    float EditorPortalComponent::GetLightBlendValue()
    {
        return m_config.m_lightBlendValue;
    }

    bool EditorPortalComponent::GetVertex(const size_t index, AZ::Vector3& vertex) const
    {
        if (index < m_config.m_vertices.size())
        {
            vertex = m_config.m_vertices[index];
            return true;
        }

        return false;
    }

    bool EditorPortalComponent::UpdateVertex(const size_t index, const AZ::Vector3& vertex)
    {
        if (index < m_config.m_vertices.size())
        {
            m_config.m_vertices[index] = vertex;
            return true;
        }

        return false;
    }

    /// Update the object runtime after changes to the Configuration.
    /// Called by the default RequestBus SetXXX implementations,
    /// and used to initially set up the object the first time the
    /// Configuration are set.
    void EditorPortalComponent::UpdatePortalObject()
    {
        if (m_area)
        {
            SVisAreaInfo info;
            info.vAmbientColor = Vec3(ZERO);
            info.bAffectedByOutLights = m_config.m_affectedBySun;
            info.bSkyOnly = m_config.m_skyOnly;
            info.fViewDistRatio = m_config.m_viewDistRatio;
            info.bDoubleSide = m_config.m_doubleSide;
            info.bUseDeepness = m_config.m_useDeepness;
            info.bUseInIndoors = true; //Does not apply to Portals (Portals are only in VisAreas)
            info.bOceanIsVisible = m_config.m_oceanIsVisible;
            info.fPortalBlending = -1.0f;

            if (m_config.m_lightBlending)
            {
                info.fPortalBlending = m_config.m_lightBlendValue;
            }

            AZStd::string name = AZStd::string("Portal_") + GetEntity()->GetName();

            // Calculate scaled height
            // Height exists separate from plane points but we still want to scale it with the transform
            info.fHeight = m_config.m_height;

            /*
               We have to derive at least 3 points and pass them to the vis area system
               For now that means getting the 4 points of the bottom face of the box.

               If we want to send *all* points of a shape the vis system we need to make sure
               that Height is 0; otherwise it'll extend the AABB of the area upwards.
             */

            //Convert to Cry vectors and apply the transform to the given points
            AZStd::fixed_vector<Vec3, 4> verts(4);
            for (AZ::u32 i = 0; i < verts.size(); ++i)
            {
                verts[i] = AZVec3ToLYVec3(m_config.m_vertices[i]);
                verts[i] = m_cryCachedWorldTransform.TransformPoint(verts[i]);
            }

            if (GetIEditor()->Get3DEngine())
            {
                GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &verts[0], verts.size(), name.c_str(), info, true);
            }

            AzFramework::EntityBoundsUnionRequestBus::Broadcast(
                &AzFramework::EntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
        }
    }

    void EditorPortalComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        //Cache the transform so that we don't have to retrieve it every time UpdatePortalObject is called
        m_AZCachedWorldTransform = world;
        m_cryCachedWorldTransform = AZTransformToLYTransform(m_AZCachedWorldTransform);

        UpdatePortalObject();
    }

    struct PortalQuadVertices
    {
        AZ::Vector3 floorLeftFront;
        AZ::Vector3 floorRightFront;
        AZ::Vector3 floorLeftBack;
        AZ::Vector3 floorRightBack;

        AZ::Vector3 quadUpperLeftFront;
        AZ::Vector3 quadUpperRightFront;
        AZ::Vector3 quadUpperLeftBack;
        AZ::Vector3 quadUpperRightBack;

        AZ::Vector3 portalUpperLeftFront;
        AZ::Vector3 portalUpperRightFront;
        AZ::Vector3 portalUpperLeftBack;
        AZ::Vector3 portalUpperRightBack;
    };

    PortalQuadVertices EditorPortalComponent::CalculatePortalQuadVertices(VertTranslation vertTranslation)
    {
        PortalQuadVertices pqv;

        //Untransformed quad corners
        const AZ::Vector3 lowerLeftFront = m_config.m_vertices[0];
        const AZ::Vector3 lowerRightFront = m_config.m_vertices[1];
        const AZ::Vector3 lowerLeftBack = m_config.m_vertices[3];
        const AZ::Vector3 lowerRightBack = m_config.m_vertices[2];

        //Need to calculate the height of the quad after transformation
        const AZ::u32 quadPointCount = 4;
        AZ::Vector3 transformedQuadPoints[quadPointCount];

        transformedQuadPoints[0] = m_AZCachedWorldTransform.TransformPoint(lowerLeftFront);
        transformedQuadPoints[1] = m_AZCachedWorldTransform.TransformPoint(lowerRightFront);
        transformedQuadPoints[2] = m_AZCachedWorldTransform.TransformPoint(lowerLeftBack);
        transformedQuadPoints[3] = m_AZCachedWorldTransform.TransformPoint(lowerRightBack);

        const AZ::Vector3 translation = m_AZCachedWorldTransform.GetTranslation();

        float minHeight = FLT_MAX;
        float maxHeight = FLT_MIN;

        for (auto& transformedQuadPoint : transformedQuadPoints)
        {
            // remove translation from quad points so we can use them with the DisplayContext's usage of the transform
            if (vertTranslation == VertTranslation::Remove)
            {
                transformedQuadPoint -= translation;
            }

            const float height = transformedQuadPoint.GetZ();
            if (height < minHeight)
            {
                minHeight = height;
            }
            if (height > maxHeight)
            {
                maxHeight = height;
            }
        }

        pqv.floorLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), minHeight);
        pqv.floorRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), minHeight);
        pqv.floorLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), minHeight);
        pqv.floorRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), minHeight);

        pqv.quadUpperLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), maxHeight);
        pqv.quadUpperRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), maxHeight);
        pqv.quadUpperLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), maxHeight);
        pqv.quadUpperRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), maxHeight);

        pqv.portalUpperLeftFront = AZ::Vector3(transformedQuadPoints[0].GetX(), transformedQuadPoints[0].GetY(), maxHeight + m_config.m_height);
        pqv.portalUpperRightFront = AZ::Vector3(transformedQuadPoints[1].GetX(), transformedQuadPoints[1].GetY(), maxHeight + m_config.m_height);
        pqv.portalUpperLeftBack = AZ::Vector3(transformedQuadPoints[2].GetX(), transformedQuadPoints[2].GetY(), maxHeight + m_config.m_height);
        pqv.portalUpperRightBack = AZ::Vector3(transformedQuadPoints[3].GetX(), transformedQuadPoints[3].GetY(), maxHeight + m_config.m_height);

        return pqv;
    }

    void EditorPortalComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        /*
           IMPORTANT NOTE: This method may seem very complicated but it is an accurate visualization of
           how portals actually work. The legacy visualization used with the legacy portal entity is
           very misleading!

           Portals always exist as a quad but if the quad becomes non-planar, from rotation or in the legacy
           system from a point being pulled up or down, the volume changes in a non-obvious way. Instead of portal
           existing as the shape defined by 4 points and extruded upwards, the portal actually remains planar.
           Any height difference that you add by making the shape non-planar is just applied to the height of the volume.

           If this is confusing, please actually look at the visualization created by this method. Make sure
           that you rotate the portal in many weird contorted ways and examine how the visualization reacts.
           The portal volume is always going to be a box rotated on only X and Y axes that stretches up along the Z axis.

           Important note on the complexity of this method:
           We cannot directly visualize the OBB of the portal with an AABB that we then transform. The OBB that's mentioned
           here is best imagined as the top plane being all points of the quad pulled up to the height of the highest quad's vert
           and the bottom plane being all points of the quad pulled down to the height of the lowest quad's vert. Trying to
           create an AABB from these points won't produce the correct visualization under complex rotations as the Min and Max
           of the AABB will either only encompass part of the bounding volume or be too large.
         */

        const PortalQuadVertices pqv = CalculatePortalQuadVertices(VertTranslation::Remove);

        //Draw the outline of the OBB of the Portal's quad
        AZ::Color color(0.000f, 1.0f, 0.000f, 1.0f);
        debugDisplay.SetColor(AZ::Vector4(color.GetR(), color.GetG(), color.GetB(), 1.f));

        //Remove all rotation from the transform
        const AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();

        AZ::Transform worldTMOnlyZRot = m_AZCachedWorldTransform;
        worldTMOnlyZRot.SetRotation(rotation);

        debugDisplay.PushMatrix(worldTMOnlyZRot);

        //Draw the outline of the OBB of the portal quad

        //Bottom
        debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
        debugDisplay.DrawLine(pqv.floorRightFront, pqv.floorRightBack);
        debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorLeftBack);
        debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
        //Top
        debugDisplay.DrawLine(pqv.quadUpperLeftFront, pqv.quadUpperRightFront);
        debugDisplay.DrawLine(pqv.quadUpperRightFront, pqv.quadUpperRightBack);
        debugDisplay.DrawLine(pqv.quadUpperRightBack, pqv.quadUpperLeftBack);
        debugDisplay.DrawLine(pqv.quadUpperLeftBack, pqv.quadUpperLeftFront);
        //Left
        debugDisplay.DrawLine(pqv.floorLeftFront, pqv.quadUpperLeftFront);
        debugDisplay.DrawLine(pqv.quadUpperLeftFront, pqv.quadUpperLeftBack);
        debugDisplay.DrawLine(pqv.quadUpperLeftBack, pqv.floorLeftBack);
        debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
        //Right
        debugDisplay.DrawLine(pqv.floorRightFront, pqv.quadUpperRightFront);
        debugDisplay.DrawLine(pqv.quadUpperRightFront, pqv.quadUpperRightBack);
        debugDisplay.DrawLine(pqv.quadUpperRightBack, pqv.floorRightBack);
        debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorRightFront);
        //Front
        debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
        debugDisplay.DrawLine(pqv.floorRightFront, pqv.quadUpperRightFront);
        debugDisplay.DrawLine(pqv.quadUpperRightFront, pqv.quadUpperLeftFront);
        debugDisplay.DrawLine(pqv.quadUpperLeftFront, pqv.floorLeftFront);
        //Back
        debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorRightBack);
        debugDisplay.DrawLine(pqv.floorRightBack, pqv.quadUpperRightBack);
        debugDisplay.DrawLine(pqv.quadUpperRightBack, pqv.quadUpperLeftBack);
        debugDisplay.DrawLine(pqv.quadUpperLeftBack, pqv.floorLeftBack);

        //Now draw the entire portal volume (Previous OBB + extra height)
        if (m_config.m_displayFilled)
        {
            //Draw whole portal with less alpha
            debugDisplay.SetColor(AZ::Vector4(color.GetR(), color.GetG(), color.GetB(), 0.1f));

            //Draw both winding orders for quads so they appear solid from all angles
            //Not drawing boxes because the corners of the quad may not be hit if the bounds are rotated oddly

            //Bottom
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorRightFront, pqv.floorRightBack, pqv.floorLeftBack);
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorLeftBack, pqv.floorRightBack, pqv.floorRightFront);
            //Top
            debugDisplay.DrawQuad(pqv.portalUpperLeftFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawQuad(pqv.portalUpperLeftFront, pqv.portalUpperLeftBack, pqv.portalUpperRightBack, pqv.portalUpperRightFront);
            //Left
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.portalUpperLeftFront, pqv.portalUpperLeftBack, pqv.floorLeftBack);
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorLeftBack, pqv.portalUpperLeftBack, pqv.portalUpperLeftFront);
            //Right
            debugDisplay.DrawQuad(pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.floorRightBack);
            debugDisplay.DrawQuad(pqv.floorRightFront, pqv.floorRightBack, pqv.portalUpperRightBack, pqv.portalUpperRightFront);
            //Front
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperLeftFront);
            debugDisplay.DrawQuad(pqv.floorLeftFront, pqv.portalUpperLeftFront, pqv.portalUpperRightFront, pqv.floorRightFront);
            //Back
            debugDisplay.DrawQuad(pqv.floorLeftBack, pqv.floorRightBack, pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawQuad(pqv.floorLeftBack, pqv.portalUpperLeftBack, pqv.portalUpperRightBack, pqv.floorRightBack);
        }
        else
        {
            //Bottom
            debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
            debugDisplay.DrawLine(pqv.floorRightFront, pqv.floorRightBack);
            debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorLeftBack);
            debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
            //Top
            debugDisplay.DrawLine(pqv.portalUpperLeftFront, pqv.portalUpperRightFront);
            debugDisplay.DrawLine(pqv.portalUpperRightFront, pqv.portalUpperRightBack);
            debugDisplay.DrawLine(pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawLine(pqv.portalUpperLeftBack, pqv.portalUpperLeftFront);
            //Left
            debugDisplay.DrawLine(pqv.floorLeftFront, pqv.portalUpperLeftFront);
            debugDisplay.DrawLine(pqv.portalUpperLeftFront, pqv.portalUpperLeftBack);
            debugDisplay.DrawLine(pqv.portalUpperLeftBack, pqv.floorLeftBack);
            debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorLeftFront);
            //Right
            debugDisplay.DrawLine(pqv.floorRightFront, pqv.portalUpperRightFront);
            debugDisplay.DrawLine(pqv.portalUpperRightFront, pqv.portalUpperRightBack);
            debugDisplay.DrawLine(pqv.portalUpperRightBack, pqv.floorRightBack);
            debugDisplay.DrawLine(pqv.floorRightBack, pqv.floorRightFront);
            //Front
            debugDisplay.DrawLine(pqv.floorLeftFront, pqv.floorRightFront);
            debugDisplay.DrawLine(pqv.floorRightFront, pqv.portalUpperRightFront);
            debugDisplay.DrawLine(pqv.portalUpperRightFront, pqv.portalUpperLeftFront);
            debugDisplay.DrawLine(pqv.portalUpperLeftFront, pqv.floorLeftFront);
            //Back
            debugDisplay.DrawLine(pqv.floorLeftBack, pqv.floorRightBack);
            debugDisplay.DrawLine(pqv.floorRightBack, pqv.portalUpperRightBack);
            debugDisplay.DrawLine(pqv.portalUpperRightBack, pqv.portalUpperLeftBack);
            debugDisplay.DrawLine(pqv.portalUpperLeftBack, pqv.floorLeftBack);
        }

        if (m_componentModeDelegate.AddedToComponentMode())
        {
            AzToolsFramework::VertexContainerDisplay::DisplayVertexContainerIndices(
                debugDisplay, AzToolsFramework::FixedVerticesArray<AZ::Vector3, 4>(m_config.m_vertices),
                GetWorldTM(), AZ::Vector3::CreateOne(), IsSelected());
        }

        debugDisplay.PopMatrix();
    }

    void EditorPortalComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<PortalComponent>(m_config);
    }

    AZ::Aabb EditorPortalComponent::GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        const PortalQuadVertices pqv = CalculatePortalQuadVertices(VertTranslation::Keep);

        AZ::Aabb bbox = AZ::Aabb::CreateNull();
        bbox.AddPoint(pqv.floorLeftFront);
        bbox.AddPoint(pqv.floorRightFront);
        bbox.AddPoint(pqv.floorLeftBack);
        bbox.AddPoint(pqv.floorRightBack);
        bbox.AddPoint(pqv.portalUpperLeftFront);
        return bbox;
    }

    bool EditorPortalComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/, const AZ::Vector3& src,
        const AZ::Vector3& dir, float& distance)
    {
        float t;
        float intermediateT = FLT_MAX;

        const PortalQuadVertices pqv = CalculatePortalQuadVertices(VertTranslation::Keep);

        //Count each quad for intersection hits, two hits implies we are intersecting the prism from outside of it (or from too far)
        AZ::u8 hits = 0;

        //Bottom
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftFront, pqv.floorRightFront, pqv.floorRightBack, pqv.floorLeftBack, t))
        { 
            ++hits; 
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Top
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.portalUpperLeftFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.portalUpperLeftBack, t))
        { 
            ++hits; 
            intermediateT = AZStd::GetMin(t, intermediateT);
        }

        //Left
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftFront, pqv.portalUpperLeftFront, pqv.portalUpperLeftBack, pqv.floorLeftBack, t))
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Right
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperRightBack, pqv.floorRightBack, t))
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Front
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftFront, pqv.floorRightFront, pqv.portalUpperRightFront, pqv.portalUpperLeftFront, t))
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }
        //Back
        if (AZ::Intersect::IntersectRayQuad(src, dir, pqv.floorLeftBack, pqv.floorRightBack, pqv.portalUpperRightBack, pqv.portalUpperLeftBack, t)) 
        { 
            ++hits;
            intermediateT = AZStd::GetMin(t, intermediateT);
        }

        if (hits > 0)
        {
            distance = intermediateT;
        }
        return hits >= 2;
    }

    AZ::Aabb EditorPortalComponent::GetWorldBounds()
    {
        return GetLocalBounds().GetTransformedAabb(m_AZCachedWorldTransform);
    }

    AZ::Aabb EditorPortalComponent::GetLocalBounds()
    {
        AZ::Aabb bbox = AZ::Aabb::CreateNull();
        for (const auto& vertex : m_config.m_vertices)
        {
            bbox.AddPoint(vertex);
        }
        return bbox;
    }
} //namespace Visibility
