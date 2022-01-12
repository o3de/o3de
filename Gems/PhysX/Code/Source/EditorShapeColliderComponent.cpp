/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorShapeColliderComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/ColliderComponentBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <LyViewPaneNames.h>
#include <ShapeColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <Editor/ConfigurationWindowBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <PhysX/SystemComponentBus.h>
#include <Source/Utils.h>
#include <AzCore/Math/Geometry2DUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <cmath>

#include <RigidBodyStatic.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    EditorShapeColliderComponent::EditorShapeColliderComponent()
        : m_physXConfigChangedHandler(
            []([[maybe_unused]] const AzPhysics::SystemConfiguration* config)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                    AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
            })
        , m_onMaterialLibraryChangedEventHandler(
            [this](const AZ::Data::AssetId& defaultMaterialLibrary)
            {
                m_colliderConfig.m_materialSelection.OnMaterialLibraryChanged(defaultMaterialLibrary);
                Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);

                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                    AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
            })
        , m_nonUniformScaleChangedHandler([this](const AZ::Vector3& scale) {OnNonUniformScaleChanged(scale);})
    {
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
    }

    AZ::Crc32 EditorShapeColliderComponent::SubdivisionCountVisibility()
    {
        if (m_shapeType == ShapeType::Cylinder)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }
        
        return AZ::Edit::PropertyVisibility::Hide;
    }

    void EditorShapeColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorShapeColliderComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("ColliderConfiguration", &EditorShapeColliderComponent::m_colliderConfig)
                ->Field("DebugDrawSettings", &EditorShapeColliderComponent::m_colliderDebugDraw)
                ->Field("ShapeConfigs", &EditorShapeColliderComponent::m_shapeConfigs)
                ->Field("SubdivisionCount", &EditorShapeColliderComponent::m_subdivisionCount)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorShapeColliderComponent>(
                    "PhysX Shape Collider", "Create a PhysX collider using a shape provided by a Shape component.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/shape-collider/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorShapeColliderComponent::m_colliderConfig,
                        "Collider configuration", "Configuration of the collider.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorShapeColliderComponent::OnConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorShapeColliderComponent::m_colliderDebugDraw,
                        "Debug draw settings", "Debug draw settings.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorShapeColliderComponent::m_subdivisionCount, "Subdivision count",
                        "Number of angular subdivisions in the PhysX cylinder.")
                        ->Attribute(AZ::Edit::Attributes::Min, Utils::MinFrustumSubdivisions)
                        ->Attribute(AZ::Edit::Attributes::Max, Utils::MaxFrustumSubdivisions)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorShapeColliderComponent::OnSubdivisionCountChange)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorShapeColliderComponent::SubdivisionCountVisibility)
                    ;
            }
        }
    }

    void EditorShapeColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysicsWorldBodyService", 0x944da0cc));
        provided.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        provided.push_back(AZ_CRC("PhysXTriggerService", 0x3a117d7b));
        provided.push_back(AZ_CRC("PhysXShapeColliderService", 0x98a7e779));
    }

    void EditorShapeColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void EditorShapeColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LegacyCryPhysicsService"));
        incompatible.push_back(AZ_CRC_CE("PhysXShapeColliderService"));
        incompatible.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        incompatible.push_back(AZ_CRC_CE("CompoundShapeService"));
        incompatible.push_back(AZ_CRC_CE("DiskShapeService"));
        incompatible.push_back(AZ_CRC_CE("QuadShapeService"));
        incompatible.push_back(AZ_CRC_CE("TubeShapeService"));
        incompatible.push_back(AZ_CRC_CE("ReferenceShapeService"));
    }

    const AZStd::vector<AZ::Vector3>& EditorShapeColliderComponent::GetSamplePoints() const
    {
        if (m_geometryCache.m_cachedSamplePointsDirty)
        {
            UpdateCachedSamplePoints();
            m_geometryCache.m_cachedSamplePointsDirty = false;
        }

        return m_geometryCache.m_cachedSamplePoints;
    }

    void EditorShapeColliderComponent::UpdateCachedSamplePoints() const
    {
        m_geometryCache.m_cachedSamplePoints.clear();

        switch (m_shapeType)
        {
        case ShapeType::Box:
        {
            const AZ::Vector3 boxMax = 0.5f * m_geometryCache.m_boxDimensions;
            const AZ::Vector3 boxMin = -boxMax;
            m_geometryCache.m_cachedSamplePoints = Utils::Geometry::GenerateBoxPoints(boxMin, boxMax);
            break;
        }
        case ShapeType::Sphere:
        {
            m_geometryCache.m_cachedSamplePoints = Utils::Geometry::GenerateSpherePoints(m_geometryCache.m_radius);
            break;
        }
        case ShapeType::Capsule:
        {
            const float cylinderHeight = m_geometryCache.m_height - 2 * m_geometryCache.m_radius;
            if (cylinderHeight > 0.0f)
            {
                m_geometryCache.m_cachedSamplePoints = Utils::Geometry::GenerateCylinderPoints(cylinderHeight,
                    m_geometryCache.m_radius);
            }
            break;
        }
        case ShapeType::Cylinder:
        {
            if (m_geometryCache.m_height > 0.0f && m_geometryCache.m_radius > 0.0f)
            {
                m_geometryCache.m_cachedSamplePoints = Utils::Geometry::GenerateCylinderPoints(
                    m_geometryCache.m_height,
                    m_geometryCache.m_radius);
            }
            break;
        }
        case ShapeType::PolygonPrism:
        {
            if (!m_shapeConfigs.empty())
            {
                AZ::PolygonPrismPtr polygonPrismPtr;
                LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(),
                    &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);

                if (polygonPrismPtr)
                {
                    const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
                    const AZ::Vector3 overallScale = uniformScale * m_currentNonUniformScale;

                    const AZStd::vector<AZ::Vector2>& vertices = polygonPrismPtr->m_vertexContainer.GetVertices();
                    for (const auto& vertex : vertices)
                    {
                        const float scaledX = overallScale.GetX() * vertex.GetX();
                        const float scaledY = overallScale.GetY() * vertex.GetY();
                        m_geometryCache.m_cachedSamplePoints.emplace_back(AZ::Vector3(scaledX, scaledY, 0.0f));
                        m_geometryCache.m_cachedSamplePoints.emplace_back(AZ::Vector3(scaledX, scaledY, m_geometryCache.m_height));
                    }
                }
            }

            break;
        }
        default:
            AZ_WarningOnce("PhysX Shape Collider Component", false, "Unsupported shape type in UpdateCachedSamplePoints");
            break;
        }

        AZ::Transform transform = GetWorldTM();
        transform.ExtractUniformScale();
        const size_t numPoints = m_geometryCache.m_cachedSamplePoints.size();
        for (size_t pointIndex = 0; pointIndex < numPoints; ++pointIndex)
        {
            m_geometryCache.m_cachedSamplePoints[pointIndex] = transform.TransformPoint(m_geometryCache.m_cachedSamplePoints[pointIndex]);
        }
    }

    const Physics::ColliderConfiguration& EditorShapeColliderComponent::GetColliderConfiguration() const
    {
        return m_colliderConfig;
    }

    const AZStd::vector<AZStd::shared_ptr<Physics::ShapeConfiguration>>& EditorShapeColliderComponent::GetShapeConfigurations() const
    {
        return m_shapeConfigs;
    }

    void EditorShapeColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* shapeColliderComponent = gameEntity->CreateComponent<ShapeColliderComponent>();
        AzPhysics::ShapeColliderPairList shapeConfigurationList;
        shapeConfigurationList.reserve(m_shapeConfigs.size());
        for (const auto& shapeConfig : m_shapeConfigs)
        {
            shapeConfigurationList.emplace_back(
                AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig),
                shapeConfig);
        }

        shapeColliderComponent->SetShapeConfigurationList(shapeConfigurationList);

        StaticRigidBodyUtils::TryCreateRuntimeComponent(*GetEntity(), *gameEntity);
    }

    void EditorShapeColliderComponent::CreateStaticEditorCollider()
    {
        // Don't create static rigid body in the editor if current entity components
        // don't allow creation of runtime static rigid body component
        if (!StaticRigidBodyUtils::CanCreateRuntimeComponent(*GetEntity()))
        {
            return;
        }

        const AZ::Transform colliderTransform = GetWorldTM();

        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = colliderTransform.GetRotation();
        configuration.m_position = colliderTransform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        AzPhysics::ShapeColliderPairList colliderShapePairs;
        colliderShapePairs.reserve(m_shapeConfigs.size());
        for (const auto& shapeConfig : m_shapeConfigs)
        {
            colliderShapePairs.emplace_back(
                AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig), shapeConfig);
        }
        configuration.m_colliderAndShapeData = colliderShapePairs;

        if (m_sceneInterface)
        {
            //remove the previous body if any
            if (m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
            {
                m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
            }

            m_editorBodyHandle = m_sceneInterface->AddSimulatedBody(m_editorSceneHandle, &configuration);
        }

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    AZ::u32 EditorShapeColliderComponent::OnConfigurationChanged()
    {
        m_colliderConfig.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        CreateStaticEditorCollider();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorShapeColliderComponent::UpdateShapeConfigs()
    {
        m_geometryCache.m_cachedSamplePointsDirty = true;

        AZ::Crc32 shapeCrc;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeCrc, GetEntityId(),
            &LmbrCentral::ShapeComponentRequests::GetShapeType);

        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
        const AZ::Vector3 overallScale = uniformScale * m_currentNonUniformScale;

        // using if blocks because switch statements aren't supported for values generated by the crc macro
        if (shapeCrc == ShapeConstants::Box)
        {
            UpdateBoxConfig(overallScale);
        }

        else if (shapeCrc == ShapeConstants::Capsule)
        {
            UpdateCapsuleConfig(uniformScale);
        }

        else if (shapeCrc == ShapeConstants::Sphere)
        {
            UpdateSphereConfig(uniformScale);
        }

        else if (shapeCrc == ShapeConstants::PolygonPrism)
        {
            UpdatePolygonPrismDecomposition();
        }

        else if (shapeCrc == ShapeConstants::Cylinder)
        {
            UpdateCylinderConfig(uniformScale);
        }

        else
        {
            m_shapeType = !shapeCrc ? ShapeType::None : ShapeType::Unsupported;
            m_shapeConfigs.clear();
            AZ_Warning("PhysX Shape Collider Component", m_shapeTypeWarningIssued, "Unsupported shape type for "
                "entity \"%s\". The following shapes are currently supported - box, capsule, sphere, polygon prism.",
                GetEntity()->GetName().c_str());
            m_shapeTypeWarningIssued = true;
        }
    }

    void EditorShapeColliderComponent::UpdateBoxConfig(const AZ::Vector3& scale)
    {
        AZ::Vector3 boxDimensions = AZ::Vector3::CreateOne();
        LmbrCentral::BoxShapeComponentRequestsBus::EventResult(boxDimensions, GetEntityId(),
            &LmbrCentral::BoxShapeComponentRequests::GetBoxDimensions);

        SetShapeConfig(ShapeType::Box, Physics::BoxShapeConfiguration(boxDimensions));

        m_shapeConfigs.back()->m_scale = scale;
        m_geometryCache.m_boxDimensions = scale * boxDimensions;
    }

    void EditorShapeColliderComponent::UpdateCapsuleConfig(const AZ::Vector3& scale)
    {
        LmbrCentral::CapsuleShapeConfig lmbrCentralCapsuleShapeConfig;
        LmbrCentral::CapsuleShapeComponentRequestsBus::EventResult(lmbrCentralCapsuleShapeConfig, GetEntityId(),
            &LmbrCentral::CapsuleShapeComponentRequests::GetCapsuleConfiguration);
        const Physics::CapsuleShapeConfiguration& capsuleShapeConfig =
            Utils::ConvertFromLmbrCentralCapsuleConfig(lmbrCentralCapsuleShapeConfig);

        SetShapeConfig(ShapeType::Capsule, capsuleShapeConfig);

        m_shapeConfigs.back()->m_scale = scale;
        const float scalarScale = scale.GetMaxElement();
        m_geometryCache.m_radius = scalarScale * capsuleShapeConfig.m_radius;
        m_geometryCache.m_height = scalarScale * capsuleShapeConfig.m_height;
    }

    void EditorShapeColliderComponent::UpdateSphereConfig(const AZ::Vector3& scale)
    {
        float radius = 0.0f;
        LmbrCentral::SphereShapeComponentRequestsBus::EventResult(radius, GetEntityId(),
            &LmbrCentral::SphereShapeComponentRequests::GetRadius);

        SetShapeConfig(ShapeType::Sphere, Physics::SphereShapeConfiguration(radius));
        
        m_shapeConfigs.back()->m_scale = scale;
        m_geometryCache.m_radius = scale.GetMaxElement() * radius;
    }

    void EditorShapeColliderComponent::UpdateCylinderConfig(const AZ::Vector3& scale)
    {
        float height = 1.0f;
        float radius = 1.0f;

        LmbrCentral::CylinderShapeComponentRequestsBus::EventResult(radius, GetEntityId(),
            &LmbrCentral::CylinderShapeComponentRequests::GetRadius);

        LmbrCentral::CylinderShapeComponentRequestsBus::EventResult(height, GetEntityId(),
            &LmbrCentral::CylinderShapeComponentRequests::GetHeight);

        const float scalarScale = scale.GetMaxElement();
        m_geometryCache.m_height = scalarScale * height;
        m_geometryCache.m_radius = scalarScale * radius;

        if (radius <= 0.0f || height <= 0.0f)
        {
            m_shapeConfigs.clear();

            AZ_Warning("PhysX", false, "%s: Negative or zero cylinder dimensions are invalid (radius: '%f', height: '%f).", GetEntity()->GetName().c_str(), radius, height);
            return;
        }

        AZStd::optional<AZStd::vector<AZ::Vector3>> points = Utils::CreatePointsAtFrustumExtents(height, radius, radius, m_subdivisionCount);

        if (!points.has_value())
        {
            m_shapeConfigs.clear();

            AZ_Warning("PhysX", false, "Could not generate cylinder shape collider.");
            return;
        }

        AZStd::optional<Physics::CookedMeshShapeConfiguration> shapeConfig = Utils::CreatePxCookedMeshConfiguration(points.value(), scale);

        if (shapeConfig.has_value())
        {
            SetShapeConfig(ShapeType::Cylinder, shapeConfig.value());

            CreateStaticEditorCollider();
        }
    }

    AZ::u32 EditorShapeColliderComponent::OnSubdivisionCountChange()
    {
        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
        UpdateCylinderConfig(uniformScale);
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void EditorShapeColliderComponent::UpdatePolygonPrismDecomposition()
    {
        m_mesh.Clear();

        AZ::PolygonPrismPtr polygonPrismPtr;
        LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(),
            &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);

        if (polygonPrismPtr)
        {
            UpdatePolygonPrismDecomposition(polygonPrismPtr);
        }

        CreateStaticEditorCollider();
        m_mesh.SetDebugDrawDirty();

        m_shapeType = ShapeType::PolygonPrism;
    }

    void EditorShapeColliderComponent::UpdatePolygonPrismDecomposition(const AZ::PolygonPrismPtr polygonPrismPtr)
    {
        const AZStd::vector<AZ::Vector2>& vertices = polygonPrismPtr->m_vertexContainer.GetVertices();

        // if the polygon prism vertices do not form a simple polygon, we cannot perform the decomposition
        if (!AZ::Geometry2DUtils::IsSimplePolygon(vertices))
        {
            if (!m_simplePolygonErrorIssued)
            {
                AZ_Error("PhysX Shape Collider Component", false, "Invalid polygon prism for entity \"%s\""
                    " - must be a simple polygon (no self intersection or duplicate vertices) to be represented in PhysX.",
                    GetEntity()->GetName().c_str());
                m_simplePolygonErrorIssued = true;
            }

            m_mesh.Clear();
            m_shapeConfigs.clear();

            RefreshUiProperties();

            return;
        }

        m_simplePolygonErrorIssued = false;
        size_t numFacesRemoved = 0;

        // If the polygon prism is already convex and meets the PhysX limit on convex mesh vertices/faces,
        // then we don't need to do any complicated decomposition
        if (vertices.size() <= PolygonPrismMeshUtils::MaxPolygonPrismEdges && AZ::Geometry2DUtils::IsConvex(vertices))
        {
            m_mesh.CreateFromSimpleConvexPolygon(vertices);
        }
        else
        {
            // Compute the constrained Delaunay triangulation using poly2tri
            AZStd::vector<p2t::Point> p2tVertices;
            std::vector<p2t::Point*> polyline;
            p2tVertices.reserve(vertices.size());
            polyline.reserve(vertices.size());

            int vertexIndex = 0;
            for (const AZ::Vector2& vert : vertices)
            {
                p2tVertices.push_back(p2t::Point(vert.GetX(), vert.GetY()));
                polyline.push_back(&(p2tVertices.data()[vertexIndex++]));
            }

            p2t::CDT constrainedDelaunayTriangulation(polyline);
            constrainedDelaunayTriangulation.Triangulate();
            const std::vector<p2t::Triangle*>& triangles = constrainedDelaunayTriangulation.GetTriangles();

            // Iteratively merge faces if it's possible to do so while maintaining convexity
            m_mesh.CreateFromPoly2Tri(triangles);
            numFacesRemoved = m_mesh.ConvexMerge();
        }

        // Create the cooked convex mesh configurations
        const AZStd::vector<PolygonPrismMeshUtils::Face>& faces = m_mesh.GetFaces();
        size_t numFacesTotal = faces.size();

        if (m_shapeType != ShapeType::PolygonPrism)
        {
            m_shapeConfigs.clear();
            m_shapeType = ShapeType::PolygonPrism;
        }

        if (numFacesRemoved <= numFacesTotal)
        {
            m_shapeConfigs.reserve(numFacesTotal - numFacesRemoved);
        }

        const float unscaledPrismHeight = polygonPrismPtr->GetHeight();

        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
        const AZ::Vector3 overallScale = uniformScale * m_currentNonUniformScale;
        m_geometryCache.m_height = overallScale.GetZ() * unscaledPrismHeight;

        int shapeConfigsCount = 0;
        bool refreshPropertyTree = false;

        for (int faceIndex = 0; faceIndex < numFacesTotal; faceIndex++)
        {
            if (faces[faceIndex].m_removed)
            {
                continue;
            }

            AZStd::vector<AZ::Vector3> points;
            points.reserve(2 * faces[faceIndex].m_numEdges);
            PolygonPrismMeshUtils::HalfEdge* currentEdge = faces[faceIndex].m_edge;

            for (int edgeIndex = 0; edgeIndex < faces[faceIndex].m_numEdges; edgeIndex++)
            {
                points.emplace_back(currentEdge->m_origin.GetX(), currentEdge->m_origin.GetY(), 0.0f);
                points.emplace_back(currentEdge->m_origin.GetX(), currentEdge->m_origin.GetY(), unscaledPrismHeight);
                currentEdge = currentEdge->m_next;
            }

            AZStd::optional<Physics::CookedMeshShapeConfiguration> shapeConfig = Utils::CreatePxCookedMeshConfiguration(points, overallScale);

            if (shapeConfig.has_value())
            {
                if (shapeConfigsCount >= m_shapeConfigs.size())
                {
                    m_shapeConfigs.push_back(AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(shapeConfig.value()));
                    refreshPropertyTree = true;
                }
                else
                {
                    Physics::CookedMeshShapeConfiguration& configuration =
                        static_cast<Physics::CookedMeshShapeConfiguration&>(*m_shapeConfigs[shapeConfigsCount]);
                    configuration = Physics::CookedMeshShapeConfiguration(shapeConfig.value());
                }

                ++shapeConfigsCount;
            }
        }

        if (m_shapeConfigs.size() != shapeConfigsCount)
        {
            refreshPropertyTree = true;
            m_shapeConfigs.resize(shapeConfigsCount);
        }

        if (refreshPropertyTree)
        {
            RefreshUiProperties();
        }
    }

    void EditorShapeColliderComponent::RefreshUiProperties()
    {
        // Note: This is a workaround for a crash in InstanceDataHierarchy when a polygon prism shape collider is stored in a slice.
        // m_shapeConfigs vector is reflected in the component and resizing it without invalidating property tree leads to dangling pointers
        // in the hierarchy comparison system.

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
    }

    // AZ::Component
    void EditorShapeColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        PhysX::ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        AZ::NonUniformScaleRequestBus::Event(GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
            m_nonUniformScaleChangedHandler);

        AZ::TransformBus::EventResult(m_cachedWorldTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_currentNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_currentNonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        m_sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (m_sceneInterface)
        {
            m_editorSceneHandle = m_sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        }

        UpdateShapeConfigs();

        // Debug drawing
        m_colliderDebugDraw.Connect(GetEntityId());
        m_colliderDebugDraw.SetDisplayCallback(this);
        CreateStaticEditorCollider();

        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorShapeColliderComponent::Deactivate()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        m_colliderDebugDraw.Disconnect();

        m_nonUniformScaleChangedHandler.Disconnect();
        PhysX::ColliderShapeRequestBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorShapeColliderComponent::OnSelected()
    {
        if (auto* physXSystem = GetPhysXSystem())
        {
            if (!m_physXConfigChangedHandler.IsConnected())
            {
                physXSystem->RegisterSystemConfigurationChangedEvent(m_physXConfigChangedHandler);
            }
            if (!m_onMaterialLibraryChangedEventHandler.IsConnected())
            {
                physXSystem->RegisterOnMaterialLibraryChangedEventHandler(m_onMaterialLibraryChangedEventHandler);
            }
        }
    }

    void EditorShapeColliderComponent::OnDeselected()
    {
        m_onMaterialLibraryChangedEventHandler.Disconnect();
        m_physXConfigChangedHandler.Disconnect();
    }

    // TransformBus
    void EditorShapeColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (world.IsClose(m_cachedWorldTransform))
        {
            return;
        }
        m_cachedWorldTransform = world;

        UpdateShapeConfigs();

        CreateStaticEditorCollider();
        m_geometryCache.m_cachedSamplePointsDirty = true;
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorShapeColliderComponent::OnNonUniformScaleChanged(const AZ::Vector3& scale)
    {
        m_currentNonUniformScale = scale;

        UpdateShapeConfigs();

        CreateStaticEditorCollider();
        m_geometryCache.m_cachedSamplePointsDirty = true;
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorShapeColliderComponent::EnablePhysics()
    {
        if (!IsPhysicsEnabled() && m_sceneInterface)
        {
            m_sceneInterface->EnableSimulationOfBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    void EditorShapeColliderComponent::DisablePhysics()
    {
        if (m_sceneInterface)
        {
            m_sceneInterface->DisableSimulationOfBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    bool EditorShapeColliderComponent::IsPhysicsEnabled() const
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->m_simulating;
            }
        }
        return false;
    }

    AZ::Aabb EditorShapeColliderComponent::GetAabb() const
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBody* EditorShapeColliderComponent::GetSimulatedBody()
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body;
            }
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyHandle EditorShapeColliderComponent::GetSimulatedBodyHandle() const
    {
        return m_editorBodyHandle;
    }

    AzPhysics::SceneQueryHit EditorShapeColliderComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->RayCast(request);
            }
        }
        return AzPhysics::SceneQueryHit();
    }

    // LmbrCentral::ShapeComponentNotificationBus
    void EditorShapeColliderComponent::OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            UpdateShapeConfigs();

            CreateStaticEditorCollider();
            Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
        }
    }

    // DisplayCallback
    void EditorShapeColliderComponent::Display(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        // polygon prism is a special case
        if (m_shapeType == ShapeType::PolygonPrism)
        {
            AZ::PolygonPrismPtr polygonPrismPtr;
            LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(),
                &LmbrCentral::PolygonPrismShapeComponentRequests::GetPolygonPrism);
            if (polygonPrismPtr)
            {
                const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
                const AZ::Vector3 overallScale = uniformScale * m_currentNonUniformScale;
                const float height = polygonPrismPtr->GetHeight();
                m_colliderDebugDraw.DrawPolygonPrism(debugDisplay, m_colliderConfig, m_mesh.GetDebugDrawPoints(height,
                    overallScale));
            }
        }

        else if (m_shapeType == ShapeType::Cylinder)
        {
            if (!m_shapeConfigs.empty())
            {
                const AZ::u32 shapeIndex = 0;
                const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
                Physics::ShapeConfiguration* shapeConfig = m_shapeConfigs[0].get();
                m_colliderDebugDraw.BuildMeshes(*shapeConfig, shapeIndex);
                m_colliderDebugDraw.DrawMesh(debugDisplay, m_colliderConfig, *static_cast<Physics::CookedMeshShapeConfiguration*>(shapeConfig), uniformScale, shapeIndex);
            }            
        }

        // for primitive shapes just display the shape configs
        else
        {
            for (const auto& shapeConfig : m_shapeConfigs)
            {
                switch (shapeConfig->GetShapeType())
                {
                case Physics::ShapeType::Box:
                {
                    const auto& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawBox(debugDisplay, m_colliderConfig, boxConfig, m_currentNonUniformScale, false);
                    break;
                }
                case Physics::ShapeType::Capsule:
                {
                    const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawCapsule(debugDisplay, m_colliderConfig, capsuleConfig, AZ::Vector3::CreateOne(), true);
                    break;
                }
                case Physics::ShapeType::Sphere:
                {
                    const auto& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(*shapeConfig);
                    m_colliderDebugDraw.DrawSphere(debugDisplay, m_colliderConfig, sphereConfig);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    // ColliderShapeRequestBus
    AZ::Aabb EditorShapeColliderComponent::GetColliderShapeAabb()
    {
        AZ::Aabb aabb = AZ::Aabb::CreateFromPoint(GetWorldTM().GetTranslation());
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, GetEntityId(),
            &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);
        return aabb;
    }

    bool EditorShapeColliderComponent::IsTrigger()
    {
        return m_colliderConfig.m_isTrigger;
    }
} // namespace PhysX
