/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/any.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <Vegetation/InstanceSpawner.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace Vegetation
{
    static constexpr float s_defaultLowerSurfaceDistanceInMeters = -1000.0f;
    static constexpr float s_defaultUpperSurfaceDistanceInMeters = 1000.0f;
    struct SurfaceTagDistance final
    {
        AZ_CLASS_ALLOCATOR(SurfaceTagDistance, AZ::SystemAllocator);
        AZ_TYPE_INFO(SurfaceTagDistance, "{2AB6096D-C7C0-4C5E-AA84-7CA804A9680C}");
        static void Reflect(AZ::ReflectContext* context);

        bool operator==(const SurfaceTagDistance& rhs) const;

        SurfaceData::SurfaceTagVector m_tags;
        float m_upperDistanceInMeters = s_defaultUpperSurfaceDistanceInMeters;
        float m_lowerDistanceInMeters = s_defaultLowerSurfaceDistanceInMeters;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(const AZStd::string& tag);
    };

    enum class BoundMode : AZ::u8
    {
        Radius = 0,
        MeshRadius
    };

    enum class OverrideMode : AZ::u8
    {
        Disable = 0, // Ignore descriptor level values
        Replace, // Replace component level values with descriptor level values
        Extend, // Combine component level values with descriptor level values
    };

    inline constexpr AZ::TypeId VegetationDescriptorTypeId{ "{A5A5E7F7-FC36-4BD1-8A93-21362574B9DA}" };

    /**
    * Details used to create vegetation instances
    */
    struct Descriptor final
    {
        AZ_CLASS_ALLOCATOR(Descriptor, AZ::SystemAllocator);
        AZ_TYPE_INFO(Descriptor, VegetationDescriptorTypeId);
        static void Reflect(AZ::ReflectContext* context);

        Descriptor();
        ~Descriptor();

        bool operator==(const Descriptor& rhs) const;
        bool HasEquivalentInstanceSpawners(const Descriptor& rhs) const;

        // Pass-throughs to the specific type of instance spawner that we have for this descriptor.
        AZ_INLINE AZStd::string GetDescriptorName() { return m_instanceSpawner ? m_instanceSpawner->GetName() : "<unknown>"; }
        AZ_INLINE void LoadAssets() { if (m_instanceSpawner) { m_instanceSpawner->LoadAssets(); } }
        AZ_INLINE void UnloadAssets() { if (m_instanceSpawner) { m_instanceSpawner->UnloadAssets(); } }
        AZ_INLINE void OnRegisterUniqueDescriptor() { if (m_instanceSpawner) { m_instanceSpawner->OnRegisterUniqueDescriptor(); } }
        AZ_INLINE void OnReleaseUniqueDescriptor() { if (m_instanceSpawner) { m_instanceSpawner->OnReleaseUniqueDescriptor(); } }
        AZ_INLINE float GetInstanceRadius() const { return (m_instanceSpawner && m_instanceSpawner->HasRadiusData()) ? m_instanceSpawner->GetRadius() : m_radiusMin; }
        AZ_INLINE bool HasEmptyAssetReferences() const { return m_instanceSpawner ? m_instanceSpawner->HasEmptyAssetReferences() : true; }
        AZ_INLINE bool IsLoaded() const { return m_instanceSpawner ? m_instanceSpawner->IsLoaded() : false; }
        AZ_INLINE bool IsSpawnable() const { return m_instanceSpawner ? m_instanceSpawner->IsSpawnable() : false; }
        AZ_INLINE InstancePtr CreateInstance(const InstanceData& instanceData)
        {
            return m_instanceSpawner ? m_instanceSpawner->CreateInstance(instanceData) : nullptr;
        }
        AZ_INLINE void DestroyInstance(InstanceId id, InstancePtr instance)
        {
            if (m_instanceSpawner)
            {
                m_instanceSpawner->DestroyInstance(id, instance);
            }
        }

        // We use the InstanceSpawner pointer as the notification bus ID since the InstanceSpawner is
        // the one that will actually broadcast out the notifications.  Multiple Descriptors can point to
        // the same InstanceSpawner, but we want its notifications to go to the consumer of every Descriptor
        // pointing to it.
        void* GetDescriptorNotificationBusId() const { return m_instanceSpawner.get(); }

        // Use with caution, changing the InstanceSpawner will change the DescriptorNotificationBusId.
        AZ_INLINE AZStd::shared_ptr<InstanceSpawner> GetInstanceSpawner() const { return m_instanceSpawner; }
        AZ_INLINE void SetInstanceSpawner(const AZStd::shared_ptr<InstanceSpawner>& spawner) { m_instanceSpawner = spawner; }



        // (basic)

        AZ::TypeId m_spawnerType;
        float m_weight = 1.0f;
        bool  m_advanced = false;

        // (advanced)

        // surface tag settings
        SurfaceTagDistance m_surfaceTagDistance;

        // surface tag filter settings
        OverrideMode m_surfaceFilterOverrideMode = OverrideMode::Disable;
        SurfaceData::SurfaceTagVector m_inclusiveSurfaceFilterTags;
        SurfaceData::SurfaceTagVector m_exclusiveSurfaceFilterTags;

        // radius
        bool m_radiusOverrideEnabled = false;
        BoundMode m_boundMode = BoundMode::Radius;
        float m_radiusMin = 0.0f;
        AZ_INLINE float GetRadius() const { return (m_boundMode == BoundMode::MeshRadius) ? GetInstanceRadius() : m_radiusMin; }

        // surface alignment
        bool m_surfaceAlignmentOverrideEnabled = false;
        float m_surfaceAlignmentMin = 0.0f;
        float m_surfaceAlignmentMax = 1.0f;

        // position
        bool m_positionOverrideEnabled = false;
        float m_positionMinX = -0.3f;
        float m_positionMaxX = 0.3f;
        float m_positionMinY = -0.3f;
        float m_positionMaxY = 0.3f;
        float m_positionMinZ = 0.0f;
        float m_positionMaxZ = 0.0f;
        AZ_INLINE AZ::Vector3 GetPositionMin() const { return AZ::Vector3(m_positionMinX, m_positionMinY, m_positionMinZ); }
        AZ_INLINE AZ::Vector3 GetPositionMax() const { return AZ::Vector3(m_positionMaxX, m_positionMaxY, m_positionMaxZ); }

        // rotation
        bool m_rotationOverrideEnabled = false;
        float m_rotationMinX = 0.0f;
        float m_rotationMaxX = 0.0f;
        float m_rotationMinY = 0.0f;
        float m_rotationMaxY = 0.0f;
        float m_rotationMinZ = -180.0f;
        float m_rotationMaxZ = 180.0f;
        AZ_INLINE AZ::Vector3 GetRotationMin() const { return AZ::Vector3(m_rotationMinX, m_rotationMinY, m_rotationMinZ); }
        AZ_INLINE AZ::Vector3 GetRotationMax() const { return AZ::Vector3(m_rotationMaxX, m_rotationMaxY, m_rotationMaxZ); }

        // scale
        bool m_scaleOverrideEnabled = false;
        float m_scaleMin = 0.1f;
        float m_scaleMax = 1.0f;

        // altitude filter
        bool m_altitudeFilterOverrideEnabled = false;
        float m_altitudeFilterMin = 0.0f;
        float m_altitudeFilterMax = 128.0f;

        // slope filter
        bool m_slopeFilterOverrideEnabled = false;
        float m_slopeFilterMin = 0.0f;
        float m_slopeFilterMax = 20.0f;

        size_t GetNumInclusiveSurfaceFilterTags() const;
        AZ::Crc32 GetInclusiveSurfaceFilterTag(int tagIndex) const;
        void RemoveInclusiveSurfaceFilterTag(int tagIndex);
        void AddInclusiveSurfaceFilterTag(const AZStd::string& tag);

        size_t GetNumExclusiveSurfaceFilterTags() const;
        AZ::Crc32 GetExclusiveSurfaceFilterTag(int tagIndex) const;
        void RemoveExclusiveSurfaceFilterTag(int tagIndex);
        void AddExclusiveSurfaceFilterTag(const AZStd::string& tag);

        AZStd::vector<AZStd::pair<AZ::TypeId, AZStd::string>> GetSpawnerTypeList() const;
        AZ::u32 SpawnerTypeChanged();
        void RefreshSpawnerTypeList() const;
    private:
        AZ::TypeId GetSpawnerType() const;
        void SetSpawnerType(const AZ::TypeId& spawnerType);

        AZStd::any GetSpawner() const;
        void SetSpawner(const AZStd::any& spawner);

        bool CreateInstanceSpawner(AZ::TypeId spawnerType, InstanceSpawner* spawnerToClone = nullptr);

        AZ::u32 GetAdvancedGroupVisibility() const;
        AZ::u32 GetBoundModeVisibility() const;

        AZ_INLINE bool IsSurfaceTagFilterReadOnly() const { return m_surfaceFilterOverrideMode == OverrideMode::Disable; }
        AZ_INLINE bool IsRadiusReadOnly() const { return (!m_radiusOverrideEnabled) || (m_boundMode != BoundMode::Radius); }
        AZ_INLINE bool IsDistanceBetweenFilterReadOnly() const { return !m_radiusOverrideEnabled; }
        AZ_INLINE bool IsSurfaceAlignmentFilterReadOnly() const { return !m_surfaceAlignmentOverrideEnabled; }
        AZ_INLINE bool IsPositionFilterReadOnly() const { return !m_positionOverrideEnabled; }
        AZ_INLINE bool IsRotationFilterReadOnly() const { return !m_rotationOverrideEnabled; }
        AZ_INLINE bool IsScaleFilterReadOnly() const { return !m_scaleOverrideEnabled; }
        AZ_INLINE bool IsAltitudeFilterReadOnly() const { return !m_altitudeFilterOverrideEnabled; }
        AZ_INLINE bool IsSlopeFilterReadOnly() const { return !m_slopeFilterOverrideEnabled; }

        AZStd::shared_ptr<InstanceSpawner> m_instanceSpawner{ nullptr };

        // We cache off our list of spawner types and only build it once, because it's a non-trivial
        // list to calculate for every Descriptor every time the Vegetation Aset List component is
        // refreshed.  The entries shouldn't be able to change dynamically, so there isn't a clear
        // need to ever recompute this list, other than for unit tests that change the set of registered
        // entries between tests.
        // We use a fixed vector with an arbitrary size that's expected to be plenty large, but can
        // be expanded if necessary.  It's just important for the static variable not to rely on
        // system allocators which might not exist at the point that the static variable is destroyed.
        static constexpr int m_maxSpawnerTypesExpected{ 16 };
        static AZStd::fixed_vector<AZStd::pair<AZ::TypeId, AZStd::string_view>, m_maxSpawnerTypesExpected> m_spawnerTypes;
    };

    using DescriptorPtr = AZStd::shared_ptr<Descriptor>;
    using DescriptorPtrVec = AZStd::vector<DescriptorPtr>;

} // namespace Vegetation
