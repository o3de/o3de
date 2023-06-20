/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

namespace AZ
{
    class Capsule;
    class Obb;
    class ReflectContext;
    class Sphere;
} // namespace AZ

namespace Physics
{
    /// Used to identify shape configuration type from base class.
    enum class ShapeType : AZ::u8
    {
        Sphere,
        Box,
        Capsule,
        Cylinder,
        ConvexHull, ///< Not Supported in physx
        TriangleMesh, ///< Not Supported in physx
        Native, ///< Native shape configuration if user wishes to bypass generic shape configurations.
        PhysicsAsset, ///< Shapes configured in the asset.
        CookedMesh, ///< Stores a blob of mesh data cooked for the specific engine.
        Heightfield ///< Interacts with the physics system heightfield
    };

    namespace ShapeConstants
    {
        static constexpr float DefaultCapsuleRadius = 0.25f;
        static constexpr float DefaultCapsuleHeight = 1.0f;
        static constexpr float DefaultSphereRadius = 0.5f;
        static const AZ::Vector3 DefaultBoxDimensions = AZ::Vector3::CreateOne();
        static const AZ::Vector3 DefaultScale = AZ::Vector3::CreateOne();
        static constexpr float DefaultCylinderRadius = 1.0f;
        static constexpr float DefaultCylinderHeight = 1.0f;
        static constexpr AZ::u8 DefaultCylinderSubdivisionCount = 16;
    } // namespace ShapeConstants

    class ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(ShapeConfiguration, "{1FD56C72-6055-4B35-9253-07D432B94E91}");
        static void Reflect(AZ::ReflectContext* context);
        explicit ShapeConfiguration(const AZ::Vector3& scale = ShapeConstants::DefaultScale);
        virtual ~ShapeConfiguration() = default;
        virtual ShapeType GetShapeType() const = 0;

        AZ::Vector3 m_scale = ShapeConstants::DefaultScale;
    };

    class SphereShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(SphereShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(SphereShapeConfiguration, "{0B9F3D2E-0780-4B0B-BFEE-B41C5FDE774A}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        SphereShapeConfiguration(
            float radius = ShapeConstants::DefaultSphereRadius, const AZ::Vector3& scale = ShapeConstants::DefaultScale);

        ShapeType GetShapeType() const override { return ShapeType::Sphere; }
        AZ::Sphere ToSphere(const AZ::Transform& transform = AZ::Transform::CreateIdentity()) const;

        float m_radius = ShapeConstants::DefaultSphereRadius;
    };

    class BoxShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(BoxShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(BoxShapeConfiguration, "{E58040ED-3E50-4882-B0E9-525E7A548F8D}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        BoxShapeConfiguration(
            const AZ::Vector3& boxDimensions = ShapeConstants::DefaultBoxDimensions,
            const AZ::Vector3& scale = ShapeConstants::DefaultScale);

        ShapeType GetShapeType() const override { return ShapeType::Box; }
        AZ::Obb ToObb(const AZ::Transform& transform = AZ::Transform::CreateIdentity()) const;

        AZ::Vector3 m_dimensions = ShapeConstants::DefaultBoxDimensions;
    };

    class CapsuleShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CapsuleShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(CapsuleShapeConfiguration, "{19C6A07E-5644-46B7-A49E-48703B56ED32}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        CapsuleShapeConfiguration(
            float height = ShapeConstants::DefaultCapsuleHeight,
            float radius = ShapeConstants::DefaultCapsuleRadius,
            const AZ::Vector3& scale = ShapeConstants::DefaultScale);

        ShapeType GetShapeType() const override { return ShapeType::Capsule; }
        AZ::Capsule ToCapsule(const AZ::Transform& transform = AZ::Transform::CreateIdentity()) const;

        float m_height = ShapeConstants::DefaultCapsuleHeight; //!< Total height, including hemispherical caps, oriented along z-axis.
        float m_radius = ShapeConstants::DefaultCapsuleRadius;

    private:
        void OnHeightChanged();
        void OnRadiusChanged();
    };

    class ConvexHullShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(ConvexHullShapeConfiguration, AZ::SystemAllocator);

        ShapeType GetShapeType() const override { return ShapeType::ConvexHull; }

        const void* m_vertexData = nullptr;
        AZ::u32 m_vertexCount = 0;
        AZ::u32 m_vertexStride = 4;

        const void* m_planeData = nullptr;
        AZ::u32 m_planeCount = 0;
        AZ::u32 m_planeStride = 4;

        const void* m_adjacencyData = nullptr;
        AZ::u32 m_adjacencyCount = 0;
        AZ::u32 m_adjacencyStride = 4;

        bool m_copyData = true; ///< If set, vertex buffer will be copied in the native physics implementation,
    };

    class TriangleMeshShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(TriangleMeshShapeConfiguration, AZ::SystemAllocator);

        ShapeType GetShapeType() const override { return ShapeType::TriangleMesh; }

        const void* m_vertexData = nullptr;
        AZ::u32 m_vertexCount = 0;
        AZ::u32 m_vertexStride = 4; ///< Data size of a given vertex, e.g. float * 3 = 12.

        const void* m_indexData = nullptr;
        AZ::u32 m_indexCount = 0;
        AZ::u32 m_indexStride = 12; ///< Data size of indices for a given triangle, e.g. AZ::u32 * 3 = 12.

        bool m_copyData = true; ///< If set, vertex/index buffers will be copied in the native physics implementation,
                                ///< and don't need to be kept alive by the caller;
    };

    class PhysicsAssetShapeConfiguration
        : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysicsAssetShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(PhysicsAssetShapeConfiguration, "{1C0046D9-BC9E-4F93-9F0E-D62654FB18EA}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        ShapeType GetShapeType() const override;

        AZ::Data::Asset<AZ::Data::AssetData> m_asset{ AZ::Data::AssetLoadBehavior::PreLoad };
        AZ::Vector3 m_assetScale = AZ::Vector3::CreateOne();
        bool m_useMaterialsFromAsset = true;
        AZ::u8 m_subdivisionLevel = 4; ///< The level of subdivision if a primitive shape is replaced with a convex mesh due to scaling.
    };

    class NativeShapeConfiguration : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(NativeShapeConfiguration, "{6CB8FE4A-A577-49AF-81F4-4F1AD245859A}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        ShapeType GetShapeType() const override { return ShapeType::Native; }

        void* m_nativeShapePtr = nullptr; ///< Native shape ptr. This will not be serialised
        AZ::Vector3 m_nativeShapeScale = AZ::Vector3::CreateOne(); ///< Native shape scale. This will be serialised
    };

    class CookedMeshShapeConfiguration
        : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CookedMeshShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(CookedMeshShapeConfiguration, "{D9E58241-36BB-4A4F-B50C-1736EB7E841F}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        enum class MeshType : AZ::u8
        {
            TriangleMesh = 0,
            Convex
        };

        CookedMeshShapeConfiguration() = default;
        CookedMeshShapeConfiguration(const CookedMeshShapeConfiguration&);
        CookedMeshShapeConfiguration& operator=(const CookedMeshShapeConfiguration&);
        ~CookedMeshShapeConfiguration();

        ShapeType GetShapeType() const override;

        //! Sets the cooked data. This will release the cached mesh.
        //! Input data has to be in the physics engine specific format.
        //! (e.g. in PhysX: result of cookTriangleMesh or cookConvexMesh).
        void SetCookedMeshData(const AZ::u8* cookedData, size_t cookedDataSize, MeshType type);
        const AZStd::vector<AZ::u8>& GetCookedMeshData() const;

        MeshType GetMeshType() const;

        void* GetCachedNativeMesh();
        const void* GetCachedNativeMesh() const;
        void SetCachedNativeMesh(void* cachedNativeMesh);

    private:
        void ReleaseCachedNativeMesh();

        AZStd::vector<AZ::u8> m_cookedData;
        MeshType m_type = MeshType::TriangleMesh;

        //! Cached native mesh object (e.g. PxConvexMesh or PxTriangleMesh). This data is not serialized.
        void* m_cachedNativeMesh = nullptr;
    };

    class HeightfieldShapeConfiguration
        : public ShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(HeightfieldShapeConfiguration, AZ::SystemAllocator);
        AZ_RTTI(HeightfieldShapeConfiguration, "{8DF47C83-D2A9-4E7C-8620-5E173E43C0B3}", ShapeConfiguration);
        static void Reflect(AZ::ReflectContext* context);
        HeightfieldShapeConfiguration() = default;
        HeightfieldShapeConfiguration(const HeightfieldShapeConfiguration&);
        HeightfieldShapeConfiguration& operator=(const HeightfieldShapeConfiguration&);
        ~HeightfieldShapeConfiguration();

        ShapeType GetShapeType() const override
        {
            return ShapeType::Heightfield;
        }

        const void* GetCachedNativeHeightfield() const;
        void* GetCachedNativeHeightfield();
        void SetCachedNativeHeightfield(void* cachedNativeHeightfield);
        const AZ::Vector2& GetGridResolution() const;
        void SetGridResolution(const AZ::Vector2& gridSpacing);
        size_t GetNumColumnVertices() const;
        size_t GetNumColumnSquares() const;
        void SetNumColumnVertices(size_t numColumns);
        size_t GetNumRowVertices() const;
        size_t GetNumRowSquares() const;
        void SetNumRowVertices(size_t numRows);
        const AZStd::vector<Physics::HeightMaterialPoint>& GetSamples() const;
        void ModifySample(size_t column, size_t row, const Physics::HeightMaterialPoint& point);
        void SetSamples(const AZStd::vector<Physics::HeightMaterialPoint>& samples);
        float GetMinHeightBounds() const;
        void SetMinHeightBounds(float minBounds);
        float GetMaxHeightBounds() const;
        void SetMaxHeightBounds(float maxBounds);

    private:
        //! The number of meters between each heightfield sample in x and y.
        AZ::Vector2 m_gridResolution{ 1.0f };
        //! The number of columns in the heightfield sample grid.
        uint32_t m_numColumns{ 0 };
        //! The number of rows in the heightfield sample grid.
        uint32_t m_numRows{ 0 };
        //! The minimum and maximum heights that can be used by this heightfield.
        //! This can be used by the physics system to choose a more optimal heightfield data type internally (ex: int16, uint8)
        float m_minHeightBounds{AZStd::numeric_limits<float>::lowest()};
        float m_maxHeightBounds{AZStd::numeric_limits<float>::max()};
        //! The grid of sample points for the heightfield.
        AZStd::vector<Physics::HeightMaterialPoint> m_samples;
        //! An optional storage pointer for the physics system to cache its native heightfield representation.
        void* m_cachedNativeHeightfield{ nullptr };
    };
} // namespace Physics
