/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxColliderComponent.h"
#include "EditorWhiteBoxComponent.h"
#include "WhiteBoxColliderComponent.h"

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <numeric>

namespace WhiteBox
{
    // defined below; forward-declared so the members above can use it
    static bool ConvertToTriangles(
        const WhiteBoxMesh& whiteBox, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices);

    void EditorWhiteBoxColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWhiteBoxColliderComponent, EditorComponentBase>()
                ->Version(2)
                ->Field("Configuration", &EditorWhiteBoxColliderComponent::m_physicsColliderConfiguration)
                ->Field("MeshData", &EditorWhiteBoxColliderComponent::m_meshShapeConfiguration)
                ->Field("BooleanMeshData", &EditorWhiteBoxColliderComponent::m_booleanMeshShapeConfiguration)
                ->Field("HasBooleanMesh", &EditorWhiteBoxColliderComponent::m_hasBooleanMesh)
                ->Field("DrawCollider", &EditorWhiteBoxColliderComponent::m_drawCollider)
                ->Field("WhiteBoxConfiguration", &EditorWhiteBoxColliderComponent::m_whiteBoxColliderConfiguration);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorWhiteBoxColliderComponent>(
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box Collider"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Physics collider for White Box Component"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/WhiteBox_collider.svg")
                    ->Attribute(
                        AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/WhiteBox_collider.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL,
                        "https://o3de.org/docs/user-guide/components/reference/shape/white-box-collider/")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxColliderComponent::m_physicsColliderConfiguration,
                        QT_TRANSLATE_NOOP("WhiteBox", "Configuration"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Collider configuration"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorWhiteBoxColliderComponent::m_whiteBoxColliderConfiguration,
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box Collider Configuration"),
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box collider configuration properties"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxColliderComponent::m_drawCollider,
                        QT_TRANSLATE_NOOP("WhiteBox", "Draw Collider"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Draw the actual cooked collision mesh as a green wireframe in the viewport"));
            }
        }
    }

    void EditorWhiteBoxColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("WhiteBoxColliderService"));
    }

    void EditorWhiteBoxColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("WhiteBoxService"));
    }

    void EditorWhiteBoxColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // NonUniformScaleService intentionally NOT listed - the collider bakes the entity's
        // non-uniform scale into its cooked shape scale (PhysX triangle meshes support it).
        incompatible.push_back(AZ_CRC_CE("WhiteBoxColliderService"));
        // Incompatible with other rigid bodies because it handles its own rigid body
        // internally and it would conflict if another rigid body is added to the entity.
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorWhiteBoxColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        EditorWhiteBoxColliderRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());

        // hide collider properties we do not care about for white box
        m_physicsColliderConfiguration.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
        m_physicsColliderConfiguration.SetPropertyVisibility(Physics::ColliderConfiguration::IsTrigger, false);

        m_sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (m_sceneInterface)
        {
            m_editorSceneHandle = m_sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        }

        // can't use buses here as EditorWhiteBoxComponentBus is addressed using component id. How do get component id?
        if (auto whiteBoxComponent = GetEntity()->FindComponent<WhiteBox::EditorWhiteBoxComponent>())
        {
            // The collision-filtered evaluated mesh; empty (collision off) tears the body down.
            if (auto whiteBoxMesh = whiteBoxComponent->GetPhysicsFilteredMesh())
            {
                CreatePhysics(*whiteBoxMesh);
            }
        }
    }

    void EditorWhiteBoxColliderComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxColliderRequestBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        DestroyPhysics();

        m_sceneInterface = nullptr;
        m_editorSceneHandle = AzPhysics::InvalidSceneHandle;
    }

    void EditorWhiteBoxColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // The boolean variant must come from the edit-time cook: its source entity cannot be resolved
        // during the game build. The base is re-cooked here to pick up the latest edits.
        Physics::CookedMeshShapeConfiguration baseConfiguration = m_meshShapeConfiguration;
        bool liveBoolean = false;

        if (auto* whiteBoxComponent = GetEntity()->FindComponent<WhiteBox::EditorWhiteBoxComponent>())
        {
            liveBoolean = whiteBoxComponent->GetLiveBoolean();

            // Re-cook the base from the strict collision-filtered mesh (null rather than unfiltered on a
            // clone). With the live boolean on keep the edit-time base; an empty mesh means no collider.
            if (!liveBoolean)
            {
                if (WhiteBoxMesh* baseMesh = whiteBoxComponent->GetPhysicsCombinedMeshStrict())
                {
                    if (Api::MeshFaceCount(*baseMesh) == 0)
                    {
                        baseConfiguration = Physics::CookedMeshShapeConfiguration{}; // collision disabled
                    }
                    else
                    {
                        Physics::CookedMeshShapeConfiguration freshBase;
                        if (CookToConfiguration(*baseMesh, freshBase, whiteBoxComponent))
                        {
                            baseConfiguration = freshBase;
                        }
                    }
                }
            }
        }

        // CSG cannot run at runtime, so both variants are pre-baked; the runtime component
        // toggles its collider between them via the live-boolean parameter.
        auto* collider = gameEntity->CreateComponent<WhiteBoxColliderComponent>(
            baseConfiguration, m_physicsColliderConfiguration, m_whiteBoxColliderConfiguration,
            m_booleanMeshShapeConfiguration, m_hasBooleanMesh, m_hasBooleanMesh && liveBoolean);

        // carry the editor's "Draw Collider" toggle into game mode
        if (collider != nullptr)
        {
            collider->SetDrawCollider(m_drawCollider);

            // Bake the cooked collider geometry for the runtime "Draw Collider" overlay, from the strict
            // collision-filtered mesh only; without it the runtime uses the serialized physics render data.
            if (auto* whiteBoxComponent = GetEntity()->FindComponent<WhiteBox::EditorWhiteBoxComponent>())
            {
                if (auto* mesh = whiteBoxComponent->GetPhysicsCombinedMeshStrict();
                    mesh != nullptr && Api::MeshFaceCount(*mesh) > 0)
                {
                    AZStd::vector<AZ::Vector3> debugVertices;
                    AZStd::vector<AZ::u32> debugIndices;
                    if (!Api::BuildColliderTriangles(*mesh, debugVertices, debugIndices))
                    {
                        ConvertToTriangles(*mesh, debugVertices, debugIndices);
                    }
                    collider->SetDebugMesh(debugVertices, debugIndices);
                }
            }
        }
    }

    void EditorWhiteBoxColliderComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        // a uniform-scale change requires re-cooking/rebuilding the shape (scale is not part
        // of the rigid body transform); a plain move/rotate just updates the body transform.
        if (!AZ::IsClose(world.GetUniformScale(), m_editorBuiltScale))
        {
            if (auto* whiteBoxComponent = GetEntity()->FindComponent<EditorWhiteBoxComponent>())
            {
                // Re-cook from the collision-filtered mesh; empty tears the body down.
                if (auto* mesh = whiteBoxComponent->GetPhysicsFilteredMesh())
                {
                    CreatePhysics(*mesh);
                    return;
                }
            }
        }

        if (m_sceneInterface)
        {
            if (auto* rigidBody = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_rigidBodyHandle))
            {
                rigidBody->SetTransform(world);
            }
        }
    }

    void EditorWhiteBoxColliderComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_drawCollider || m_debugVertices.empty() || m_debugIndices.size() < 3)
        {
            return;
        }

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(AZ::Color(0.15f, 1.0f, 0.35f, 1.0f)); // green wireframe

        for (size_t i = 0; i + 2 < m_debugIndices.size(); i += 3)
        {
            const AZ::Vector3 a = worldTransform.TransformPoint(m_debugVertices[m_debugIndices[i]]);
            const AZ::Vector3 b = worldTransform.TransformPoint(m_debugVertices[m_debugIndices[i + 1]]);
            const AZ::Vector3 c = worldTransform.TransformPoint(m_debugVertices[m_debugIndices[i + 2]]);
            debugDisplay.DrawLine(a, b);
            debugDisplay.DrawLine(b, c);
            debugDisplay.DrawLine(c, a);
        }
    }

    void EditorWhiteBoxColliderComponent::CookColliderVariants()
    {
        m_meshShapeConfiguration = Physics::CookedMeshShapeConfiguration{};
        m_booleanMeshShapeConfiguration = Physics::CookedMeshShapeConfiguration{};
        m_hasBooleanMesh = false;

        auto* whiteBoxComponent = GetEntity()->FindComponent<EditorWhiteBoxComponent>();
        if (whiteBoxComponent == nullptr)
        {
            return;
        }

        // Base variant: the collision-filtered evaluated mesh (stamps, carves and layers included). With
        // the live boolean on, build the uncut base instead.
        Api::WhiteBoxMeshPtr ownedBase; // keeps a freshly built base alive for the cook below
        WhiteBoxMesh* baseMesh = nullptr;
        if (whiteBoxComponent->GetLiveBoolean())
        {
            ownedBase = whiteBoxComponent->BuildPhysicsFilteredBaseMesh();
            baseMesh = ownedBase ? ownedBase.get() : whiteBoxComponent->GetWhiteBoxMesh();
        }
        else
        {
            baseMesh = whiteBoxComponent->GetPhysicsFilteredMesh();
        }
        // Only cook when there is collidable geometry. If every visible layer has Collision off
        // the filtered mesh is empty, leaving m_meshShapeConfiguration empty -> no base collider.
        if (baseMesh != nullptr && Api::MeshFaceCount(*baseMesh) > 0)
        {
            CookToConfiguration(*baseMesh, m_meshShapeConfiguration, whiteBoxComponent);
        }

        // Boolean variant: the cached collision-filtered display mesh (no source lookup needed).
        if (auto* displayMesh = whiteBoxComponent->GetPhysicsFilteredBooleanMesh())
        {
            // Pass the component so stamped cubes use the greedy voxel collider here too.
            if (Api::MeshFaceCount(*displayMesh) > 0)
            {
                m_hasBooleanMesh =
                    CookToConfiguration(*displayMesh, m_booleanMeshShapeConfiguration, whiteBoxComponent);
            }
        }
    }

    void EditorWhiteBoxColliderComponent::CreatePhysics(const WhiteBoxMesh& whiteBox)
    {
        if (Api::MeshFaceCount(whiteBox) == 0)
        {
            // Nothing collidable: remove the body and the cooked data.
            DestroyPhysics();
            m_debugVertices.clear();
            m_debugIndices.clear();
            m_meshShapeConfiguration = Physics::CookedMeshShapeConfiguration{};
            m_booleanMeshShapeConfiguration = Physics::CookedMeshShapeConfiguration{};
            m_hasBooleanMesh = false;
            return;
        }

        // Cook both variants now (edit time). BuildGameEntity hands these to the runtime,
        // which cannot cook a CSG result itself.
        CookColliderVariants();

        // Wireframe of the mesh actually cooked, for the viewport.
        m_debugVertices.clear();
        m_debugIndices.clear();
        if (!Api::BuildColliderTriangles(whiteBox, m_debugVertices, m_debugIndices))
        {
            ConvertToTriangles(whiteBox, m_debugVertices, m_debugIndices);
        }

        bool live = false;
        if (auto* whiteBoxComponent = GetEntity()->FindComponent<EditorWhiteBoxComponent>())
        {
            live = whiteBoxComponent->GetLiveBoolean();
        }
        const Physics::CookedMeshShapeConfiguration& viewportConfiguration =
            (m_hasBooleanMesh && live) ? m_booleanMeshShapeConfiguration : m_meshShapeConfiguration;

        if (viewportConfiguration.GetCookedMeshData().empty())
        {
            // Nothing cooked - either the physics backend was not ready, or the selected variant
            // has no collidable geometry. Remove any stale body so the collider does not linger.
            DestroyPhysics();
            return;
        }

        // apply the entity's uniform scale to the cooked shape (the mesh is cooked unscaled
        // and the body carries only translation + rotation).
        Physics::CookedMeshShapeConfiguration scaledConfiguration = viewportConfiguration;
        m_editorBuiltScale = GetTransform()->GetWorldUniformScale();
        // Only uniform scale goes on the shape; non-uniform scale is baked into the mesh (cooked shapes ignore it).
        scaledConfiguration.m_scale = AZ::Vector3(m_editorBuiltScale);

        AzPhysics::StaticRigidBodyConfiguration bodyConfiguration;
        bodyConfiguration.m_debugName = GetEntity()->GetName().c_str();
        bodyConfiguration.m_entityId = GetEntityId();
        bodyConfiguration.m_orientation = GetTransform()->GetWorldRotationQuaternion();
        bodyConfiguration.m_position = GetTransform()->GetWorldTranslation();
        bodyConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(
            AZStd::make_shared<Physics::ColliderConfiguration>(m_physicsColliderConfiguration),
            AZStd::make_shared<Physics::CookedMeshShapeConfiguration>(scaledConfiguration));

        if (m_sceneInterface)
        {
            DestroyPhysics();
            m_rigidBodyHandle = m_sceneInterface->AddSimulatedBody(m_editorSceneHandle, &bodyConfiguration);
        }
    }

    void EditorWhiteBoxColliderComponent::DestroyPhysics()
    {
        if (m_sceneInterface && m_rigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_rigidBodyHandle);
            m_rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        }
    }

    static bool ConvertToTriangles(
        const WhiteBoxMesh& whiteBox, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices)
    {
        // Build both buffers per face, fanning n-gons: CSG results can have n-gons and boundary
        // half-edges, so 3 vertices per half-edge does not hold.
        vertices.clear();
        indices.clear();

        // Weld by vertex handle and drop degenerate triangles: PhysX rejects unwelded triangle soup.
        AZStd::unordered_map<int, AZ::u32> vertexRemap;
        const auto indexForVertex = [&](const Api::VertexHandle& vertexHandle) -> AZ::u32
        {
            const int key = vertexHandle.Index();
            const auto it = vertexRemap.find(key);
            if (it != vertexRemap.end())
            {
                return it->second;
            }
            const AZ::u32 newIndex = static_cast<AZ::u32>(vertices.size());
            vertices.push_back(Api::VertexPosition(whiteBox, vertexHandle));
            vertexRemap.emplace(key, newIndex);
            return newIndex;
        };

        const auto faceHandles = Api::MeshFaceHandles(whiteBox);
        for (const auto& faceHandle : faceHandles)
        {
            const auto faceHalfedgeHandles = Api::FaceHalfedgeHandles(whiteBox, faceHandle);
            if (faceHalfedgeHandles.size() < 3)
            {
                continue; // degenerate face, skip
            }

            AZStd::vector<AZ::u32> faceIndices;
            faceIndices.reserve(faceHalfedgeHandles.size());
            for (const auto& halfEdgeHandle : faceHalfedgeHandles)
            {
                faceIndices.push_back(indexForVertex(Api::HalfedgeVertexHandleAtTip(whiteBox, halfEdgeHandle)));
            }

            // fan the face into triangles (0, i, i+1), skipping degenerate ones
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                const AZ::u32 a = faceIndices[0];
                const AZ::u32 b = faceIndices[i];
                const AZ::u32 c = faceIndices[i + 1];
                if (a == b || b == c || a == c)
                {
                    continue;
                }
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);
            }
        }

        return !indices.empty();
    }

    bool EditorWhiteBoxColliderComponent::CookToConfiguration(
        const WhiteBoxMesh& whiteBox, Physics::CookedMeshShapeConfiguration& outConfiguration,
        [[maybe_unused]] EditorWhiteBoxComponent* voxelComponent)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;

        // Coplanar-merge the actual mesh into a light collider; fall back to per-face triangles.
        const bool merged = Api::BuildColliderTriangles(whiteBox, vertices, indices);
        if (!merged && !ConvertToTriangles(whiteBox, vertices, indices))
        {
            // if there are no valid triangles then do not attempt to create a physics mesh
            return false;
        }

        auto* physicsSystem = AZ::Interface<Physics::System>::Get();
        if (physicsSystem == nullptr)
        {
            AZ_Warning(
                "EditorWhiteBoxColliderComponent", false, "No physics backend enabled - please ensure one is provided");
            return false;
        }

        const auto cook = [&](AZStd::vector<AZ::u8>& bytes)
        {
            return physicsSystem->CookTriangleMeshToMemory(
                vertices.data(), (AZ::u32)vertices.size(), indices.data(), (AZ::u32)indices.size(), bytes);
        };

        AZStd::vector<AZ::u8> bytes;
        bool result = cook(bytes);

        // If the merged mesh was rejected by the cooker, fall back to the always-valid per-face
        // triangulation so the collider is not silently dropped.
        if (!result && merged)
        {
            vertices.clear();
            indices.clear();
            bytes.clear();
            if (ConvertToTriangles(whiteBox, vertices, indices))
            {
                result = cook(bytes);
            }
        }

        AZ_Warning(
            "EditorWhiteBoxColliderComponent", result, "Failed to cook mesh data (verts=%zu, indices=%zu)",
            vertices.size(), indices.size());

        if (result)
        {
            outConfiguration.SetCookedMeshData(
                bytes.data(), bytes.size(), Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);
            return true;
        }

        return false;
    }

    void EditorWhiteBoxColliderComponent::ConvertToPhysicsMesh(const WhiteBoxMesh& whiteBox)
    {
        CookToConfiguration(whiteBox, m_meshShapeConfiguration);
    }
} // namespace WhiteBox
