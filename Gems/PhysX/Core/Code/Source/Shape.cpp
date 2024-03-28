/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Shape.h>

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <Common/PhysXSceneQueryHelpers.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <PhysX/Material/PhysXMaterial.h>
#include <Source/Collision.h>
#include <Source/Utils.h>
#include <PhysX/MathConversion.h>

namespace PhysX
{
    namespace ShapeConstants
    {
        // 48 is the number of stacks/slices used when generating mesh geo for spheres in legacy physics
        // we default to these values for consistency
        constexpr size_t NumStacks = 48;
        constexpr size_t NumSlices = 48;
    }

    Shape::Shape(Shape&& shape)
        : m_pxShape(AZStd::move(shape.m_pxShape))
        , m_materials(AZStd::move(shape.m_materials))
        , m_collisionLayer(AZStd::move(shape.m_collisionLayer))
        , m_collisionGroup(AZStd::move(shape.m_collisionGroup))
    {
        if (m_pxShape)
        {
            m_pxShape->userData = this;
        }
    }

    Shape& Shape::operator=(Shape&& shape)
    {
        m_pxShape = AZStd::move(shape.m_pxShape);
        m_materials = AZStd::move(shape.m_materials);
        m_collisionLayer = AZStd::move(shape.m_collisionLayer);
        m_collisionGroup = AZStd::move(shape.m_collisionGroup);

        if (m_pxShape)
        {
            m_pxShape->userData = this;
        }

        return *this;
    }

    void Shape::ReleasePxShape(physx::PxShape* shape)
    {
        if (shape != nullptr)
        {
            PHYSX_SCENE_WRITE_LOCK(GetScene());
            shape->userData = nullptr;
            shape->release();
        }
    }

    Shape::Shape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration)
        : m_collisionLayer(colliderConfiguration.m_collisionLayer)
    {
        if (physx::PxShape* newShape = Utils::CreatePxShapeFromConfig(colliderConfiguration, shapeConfiguration, m_collisionGroup))
        {
            m_pxShape = PxShapeUniquePtr(newShape, AZStd::bind(&Shape::ReleasePxShape, this, newShape));
            m_pxShape->userData = this;

            ExtractMaterialsFromPxShape();

            m_tag = AZ::Crc32(colliderConfiguration.m_tag);
        }
    }

    Shape::Shape(physx::PxShape* nativeShape)
    {
        m_pxShape = PxShapeUniquePtr(nativeShape, AZStd::bind(&Shape::ReleasePxShape, this, nativeShape));
        m_pxShape->acquireReference();
        m_pxShape->userData = this;

        ExtractMaterialsFromPxShape();
    }

    Shape::~Shape()
    {
        //release the shape here, so when Shape::ReleasePxShape is called to delete the physx::PxShape* we can still acquire the scene lock.
        m_pxShape.reset();
        m_pxShape = nullptr;
        m_attachedActor = nullptr;
    }

    physx::PxShape* Shape::GetPxShape()
    {
        if (m_pxShape)
        {
            return m_pxShape.get();
        }
        return nullptr;
    }

    void Shape::SetMaterial(const AZStd::shared_ptr<Physics::Material>& material)
    {
        if (auto materialWrapper = AZStd::rtti_pointer_cast<PhysX::Material>(material))
        {
            m_materials.clear();
            m_materials.emplace_back(materialWrapper);

            BindMaterialsWithPxShape();
        }
        else
        {
            AZ_Warning("PhysX Shape", false, "Trying to assign material of unknown type");
        }
    }

    AZStd::shared_ptr<Physics::Material> Shape::GetMaterial() const
    {
        if (!m_materials.empty())
        {
            return m_materials[0];
        }
        return nullptr;
    }

    Physics::MaterialId Shape::GetMaterialId() const
    {
        if (!m_materials.empty())
        {
            return m_materials[0]->GetId();
        }

        return {};
    }

    void Shape::SetPhysXMaterials(const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& materials)
    {
        m_materials = materials;

        BindMaterialsWithPxShape();
    }

    const AZStd::vector<AZStd::shared_ptr<PhysX::Material>>& Shape::GetPhysXMaterials()
    {
        return m_materials;
    }

    void Shape::BindMaterialsWithPxShape()
    {
        if (m_pxShape)
        {
            AZStd::vector<const physx::PxMaterial*> pxMaterials;
            pxMaterials.reserve(m_materials.size());

            for (const auto& material : m_materials)
            {
                pxMaterials.emplace_back(material->GetPxMaterial());
            }

            AZ_Warning("PhysX Shape", m_materials.size() < std::numeric_limits<AZ::u16>::max(), "Trying to assign too many materials, cutting down");
            size_t materialsCount = AZStd::GetMin(m_materials.size(), static_cast<size_t>(std::numeric_limits<AZ::u16>::max()));

            {
                PHYSX_SCENE_WRITE_LOCK(GetScene());
                m_pxShape->setMaterials(const_cast<physx::PxMaterial**>(pxMaterials.data()), static_cast<physx::PxU16>(materialsCount));
            }
        }
    }

    void Shape::ExtractMaterialsFromPxShape()
    {
        if (m_pxShape == nullptr)
        {
            return;
        }
        const int BufferSize = 100;

        AZ_Warning("PhysX Shape", m_pxShape->getNbMaterials() < BufferSize, "Shape has too many materials, consider increasing the buffer");

        physx::PxMaterial* assignedMaterials[BufferSize];
        int materialsCount = m_pxShape->getMaterials(assignedMaterials, BufferSize, 0);

        m_materials.clear();
        m_materials.reserve(materialsCount);

        for (int i = 0; i < materialsCount; ++i)
        {
            if (assignedMaterials[i]->userData == nullptr)
            {
                AZ_Error("PhysX Shape", false, "Trying to assign material with no user data. Make sure you are creating materials using MaterialManager");
                continue;
            }

            AZStd::shared_ptr<PhysX::Material> physxMaterial = static_cast<PhysX::Material*>(PhysX::Utils::GetUserData(assignedMaterials[i]))->shared_from_this();
            if (!physxMaterial)
            {
                AZ_Error("PhysX Shape", false, "Invalid user data of a physx material. Make sure you are creating materials using MaterialManager");
                continue;
            }

            m_materials.push_back(physxMaterial);
        }
    }

    void Shape::SetCollisionLayer(const AzPhysics::CollisionLayer& layer)
    {
        m_collisionLayer = layer;

        PHYSX_SCENE_WRITE_LOCK(GetScene());

        physx::PxFilterData filterData = m_pxShape->getSimulationFilterData();
        Collision::SetLayer(layer, filterData);
        m_pxShape->setSimulationFilterData(filterData);
        m_pxShape->setQueryFilterData(filterData);
    }

    AzPhysics::CollisionLayer Shape::GetCollisionLayer() const
    {
        return m_collisionLayer;
    }

    void Shape::SetCollisionGroup(const AzPhysics::CollisionGroup& group)
    {
        m_collisionGroup = group;

        PHYSX_SCENE_WRITE_LOCK(GetScene());

        physx::PxFilterData filterData = m_pxShape->getSimulationFilterData();
        Collision::SetGroup(m_collisionGroup, filterData);
        m_pxShape->setSimulationFilterData(filterData);
        m_pxShape->setQueryFilterData(filterData);
    }

    AzPhysics::CollisionGroup Shape::GetCollisionGroup() const
    {
        return m_collisionGroup;
    }

    void Shape::SetName(const char* name)
    {
        if (m_pxShape)
        {
            m_pxShape->setName(name);
        }
    }

    void Shape::SetLocalPose(const AZ::Vector3& offset, const AZ::Quaternion& rotation)
    {
        PHYSX_SCENE_WRITE_LOCK(GetScene());

        physx::PxTransform pxShapeTransform = PxMathConvert(offset, rotation);
        AZ_Warning("Physics::Shape", m_pxShape->isExclusive(), "Non-exclusive shapes are not mutable after they're attached to a body.");
        if (m_pxShape->getGeometryType() == physx::PxGeometryType::eCAPSULE)
        {
            physx::PxQuat lyToPxRotation(AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
            pxShapeTransform.q *= lyToPxRotation;
        }
        m_pxShape->setLocalPose(pxShapeTransform);
    }

    AZStd::pair<AZ::Vector3, AZ::Quaternion> Shape::GetLocalPose() const
    {
        PHYSX_SCENE_READ_LOCK(GetScene());

        physx::PxTransform pose = m_pxShape->getLocalPose();
        if (m_pxShape->getGeometryType() == physx::PxGeometryType::eCAPSULE)
        {
            physx::PxQuat PxTolyRotation(-AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
            pose.q *= PxTolyRotation;
        }
        return { PxMathConvert(pose.p), PxMathConvert(pose.q) };
    }

    float Shape::GetRestOffset() const
    {
        return m_pxShape->getRestOffset();
    }

    float Shape::GetContactOffset() const
    {
        return m_pxShape->getContactOffset();
    }

    void Shape::SetRestOffset(float restOffset)
    {
        float contactOffset = GetContactOffset();
        if (restOffset >= contactOffset)
        {
            AZ_Error("PhysX Shape", false, "Requested rest offset (%e) must be less than contact offset (%e).",
                restOffset, contactOffset);
            return;
        }
        m_pxShape->setRestOffset(restOffset);
    }

    void Shape::SetContactOffset(float contactOffset)
    {
        if (contactOffset <= 0.0f)
        {
            AZ_Error("PhysX Shape", false, "Requested contact offset (%e) must exceed 0.");
            return;
        }

        float restOffset = GetRestOffset();
        if (contactOffset <= restOffset)
        {
            AZ_Error("PhysX Shape", false, "Requested contact offset (%e) must exceed rest offset (%e).",
                contactOffset, restOffset);
            return;
        }
        m_pxShape->setContactOffset(contactOffset);
    }

    void* Shape::GetNativePointer()
    {
        return m_pxShape.get();
    }

    const void* Shape::GetNativePointer() const
    {
        return m_pxShape.get();
    }

    AZ::Crc32 Shape::GetTag() const
    {
        return m_tag;
    }

    bool Shape::IsTrigger() const
    {
        if (m_pxShape->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE)
        {
            return true;
        }
        return false;
    }

    void Shape::AttachedToActor(void* actor)
    {
        physx::PxActor* pxActor = static_cast<physx::PxActor*>(actor);
        if (pxActor != nullptr)
        {
            m_attachedActor = pxActor;
        }
    }

    void Shape::DetachedFromActor()
    {
        m_attachedActor = nullptr;
    }

    AzPhysics::SceneQueryHit Shape::RayCastInternal(const AzPhysics::RayCastRequest& worldSpaceRequest, const physx::PxTransform& pose)
    {
        if (const bool shouldCollide = worldSpaceRequest.m_collisionGroup.GetMask() & m_collisionLayer.GetMask();
            !shouldCollide)
        {
            return AzPhysics::SceneQueryHit();
        }

        const physx::PxVec3 start = PxMathConvert(worldSpaceRequest.m_start);
        const physx::PxVec3 unitDir = PxMathConvert(worldSpaceRequest.m_direction);
        const physx::PxU32 maxHits = 1;
        const physx::PxHitFlags hitFlags = SceneQueryHelpers::GetPxHitFlags(worldSpaceRequest.m_hitFlags);

        physx::PxRaycastHit hitInfo;
        bool hit;
        {
            PHYSX_SCENE_READ_LOCK(GetScene());
#if (PX_PHYSICS_VERSION_MAJOR == 5)
            hit = physx::PxGeometryQuery::raycast(
                start, unitDir, m_pxShape->getGeometry(), pose, worldSpaceRequest.m_distance, hitFlags, maxHits, &hitInfo);
#else
            hit = physx::PxGeometryQuery::raycast(
                start, unitDir, m_pxShape->getGeometry().any(), pose, worldSpaceRequest.m_distance, hitFlags, maxHits, &hitInfo);
#endif
        }

        if (hit)
        {
            // Fill actor and shape, as they won't be filled from PxGeometryQuery
            hitInfo.actor = static_cast<physx::PxRigidActor*>(m_attachedActor); // This cast is safe since GetHitFromPxHit() only uses PxActor:: functions
            hitInfo.shape = GetPxShape();
            return SceneQueryHelpers::GetHitFromPxHit(hitInfo, hitInfo);
        }
        return AzPhysics::SceneQueryHit();
    }

    AzPhysics::SceneQueryHit Shape::RayCast(const AzPhysics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform)
    {
        physx::PxTransform localPose;
        {
            PHYSX_SCENE_READ_LOCK(GetScene());
            localPose = m_pxShape->getLocalPose();
        }
        return RayCastInternal(worldSpaceRequest, PxMathConvert(worldTransform) * localPose);
    }

    AzPhysics::SceneQueryHit Shape::RayCastLocal(const AzPhysics::RayCastRequest& localSpaceRequest)
    {
        physx::PxTransform localPose;
        {
            PHYSX_SCENE_READ_LOCK(GetScene());
            localPose = m_pxShape->getLocalPose();
        }
        return RayCastInternal(localSpaceRequest, localPose);
    }

    AZ::Aabb Shape::GetAabb(const AZ::Transform& worldTransform) const
    {
        PHYSX_SCENE_READ_LOCK(GetScene());
#if (PX_PHYSICS_VERSION_MAJOR == 5)
        return PxMathConvert(physx::PxGeometryQuery::getWorldBounds(m_pxShape->getGeometry(), PxMathConvert(worldTransform) * m_pxShape->getLocalPose(), 1.0f));
#else
        return PxMathConvert(physx::PxGeometryQuery::getWorldBounds(m_pxShape->getGeometry().any(), PxMathConvert(worldTransform) * m_pxShape->getLocalPose(), 1.0f));
#endif
    }

    AZ::Aabb Shape::GetAabbLocal() const
    {
        PHYSX_SCENE_READ_LOCK(GetScene());
#if (PX_PHYSICS_VERSION_MAJOR == 5)
        return PxMathConvert(physx::PxGeometryQuery::getWorldBounds(m_pxShape->getGeometry(), m_pxShape->getLocalPose(), 1.0f));
#else
        return PxMathConvert(physx::PxGeometryQuery::getWorldBounds(m_pxShape->getGeometry().any(), m_pxShape->getLocalPose(), 1.0f));
#endif
    }

    physx::PxScene* Shape::GetScene() const
    {
        if (m_attachedActor != nullptr)
        {
            return m_attachedActor->getScene();
        }
        return nullptr;
    }

    void Shape::GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices,
        const AZ::Aabb* optionalBounds) const
    {
        if (!m_pxShape)
        {
            return;
        }

        PHYSX_SCENE_READ_LOCK(GetScene());

        if (m_pxShape->getGeometryType() == physx::PxGeometryType::eTRIANGLEMESH)
        {
            physx::PxTriangleMeshGeometry geometry{};
            if (m_pxShape->getTriangleMeshGeometry(geometry) && geometry.triangleMesh && geometry.isValid())
            {
                Utils::Geometry::GetTriangleMeshGeometry(geometry, vertices, indices);
            }
        }
        else if (m_pxShape->getGeometryType() == physx::PxGeometryType::eCONVEXMESH)
        {
            physx::PxConvexMeshGeometry geometry{};
            if (m_pxShape->getConvexMeshGeometry(geometry) && geometry.convexMesh && geometry.isValid())
            {
                Utils::Geometry::GetConvexMeshGeometry(geometry, vertices, indices);
            }
        }
        else if (m_pxShape->getGeometryType() == physx::PxGeometryType::eHEIGHTFIELD)
        { 
            physx::PxHeightFieldGeometry geometry{};
            if (m_pxShape->getHeightFieldGeometry(geometry) && geometry.heightField && geometry.isValid())
            {
                Utils::Geometry::GetHeightFieldGeometry(geometry, vertices, indices, optionalBounds);
            }
        }
        else if (m_pxShape->getGeometryType() == physx::PxGeometryType::eBOX)
        {
            physx::PxBoxGeometry geometry{};
            if (m_pxShape->getBoxGeometry(geometry) && geometry.isValid())
            {
                Utils::Geometry::GetBoxGeometry(geometry, vertices, indices);
            }
        }
        else if (m_pxShape->getGeometryType() == physx::PxGeometryType::eSPHERE)
        {
            physx::PxSphereGeometry geometry{};
            if (m_pxShape->getSphereGeometry(geometry) && geometry.isValid())
            {
                Utils::Geometry::GetSphereGeometry(geometry, vertices, indices, ShapeConstants::NumStacks, ShapeConstants::NumSlices);
            }
        }
        else if (m_pxShape->getGeometryType() == physx::PxGeometryType::eCAPSULE)
        {
            physx::PxCapsuleGeometry geometry{};
            if (m_pxShape->getCapsuleGeometry(geometry) && geometry.isValid())
            {
                Utils::Geometry::GetCapsuleGeometry(geometry, vertices, indices, ShapeConstants::NumStacks, ShapeConstants::NumSlices);
            }
        }
        else
        {
            AZ_TracePrintf("Shape", "GetGeometry for PxGeometryType %d is not supported", static_cast<int>(m_pxShape->getGeometryType()));
        }
    }
}
