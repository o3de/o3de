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
#pragma once

#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/DependencyRequestBus.h>
#include <Vegetation/Ebuses/DescriptorProviderRequestBus.h>
#include <Vegetation/Ebuses/DescriptorSelectorRequestBus.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/ModifierRequestBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <AzCore/Math/Random.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Source/AreaSystemComponent.h>
#include <Source/InstanceSystemComponent.h>
#include <ISerialize.h>
#include <IIndexedMesh.h>

// used for the mock for IStatObj
#ifndef CRYINCLUDE_CRY3DENGINE_STATOBJ_H
struct SPhysGeomArray
{
};
#endif

//////////////////////////////////////////////////////////////////////////
// mock event bus classes for testing vegetation
namespace UnitTest
{
    struct MockAreaManager
        : public Vegetation::AreaSystemRequestBus::Handler
    {
        mutable int m_count = 0;

        MockAreaManager()
        {
            Vegetation::AreaSystemRequestBus::Handler::BusConnect();
        }

        ~MockAreaManager()
        {
            Vegetation::AreaSystemRequestBus::Handler::BusDisconnect();
        }

        void RegisterArea([[maybe_unused]] AZ::EntityId areaId, [[maybe_unused]] AZ::u32 layer, [[maybe_unused]] AZ::u32 priority, [[maybe_unused]] const AZ::Aabb& bounds) override
        {
            ++m_count;
        }

        void UnregisterArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void RefreshArea([[maybe_unused]] AZ::EntityId areaId, [[maybe_unused]] AZ::u32 layer, [[maybe_unused]] AZ::u32 priority, [[maybe_unused]] const AZ::Aabb& bounds) override
        {
            ++m_count;
        }

        void RefreshAllAreas() override
        {
            ++m_count;
        }

        void ClearAllAreas() override
        {
            ++m_count;
        }

        void MuteArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void UnmuteArea([[maybe_unused]] AZ::EntityId areaId) override
        {
            ++m_count;
        }

        AZStd::vector<Vegetation::InstanceData> m_existingInstances;
        void EnumerateInstancesInOverlappingSectors(const AZ::Aabb& bounds, Vegetation::AreaSystemEnumerateCallback callback) const override
        {
            EnumerateInstancesInAabb(bounds, callback);
        }

        void EnumerateInstancesInAabb([[maybe_unused]] const AZ::Aabb& bounds, Vegetation::AreaSystemEnumerateCallback callback) const override
        {
            ++m_count;
            for (const auto& instanceData : m_existingInstances)
            {
                if (callback(instanceData) != Vegetation::AreaSystemEnumerateCallbackResult::KeepEnumerating)
                {
                    return;
                }
            }
        }

        AZStd::size_t GetInstanceCountInAabb([[maybe_unused]] const AZ::Aabb& bounds) const override
        {
            ++m_count;
            return m_existingInstances.size();
        }

        AZStd::vector<Vegetation::InstanceData> GetInstancesInAabb([[maybe_unused]] const AZ::Aabb& bounds) const override
        {
            ++m_count;
            return m_existingInstances;
        }
    };

    struct MockDescriptorBus
        : public Vegetation::InstanceSystemRequestBus::Handler
    {
        AZStd::set<Vegetation::DescriptorPtr> m_descriptorSet;

        MockDescriptorBus()
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusConnect();
        }

        ~MockDescriptorBus() override
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusDisconnect();
        }

        Vegetation::DescriptorPtr RegisterUniqueDescriptor(const Vegetation::Descriptor& descriptor) override
        {
            Vegetation::DescriptorPtr sharedPtr = AZStd::make_shared<Vegetation::Descriptor>(descriptor);
            m_descriptorSet.insert(sharedPtr);
            return sharedPtr;
        }

        void ReleaseUniqueDescriptor(Vegetation::DescriptorPtr descriptorPtr) override
        {
            m_descriptorSet.erase(descriptorPtr);
        }

        void CreateInstance(Vegetation::InstanceData& instanceData) override
        {
            instanceData.m_instanceId = Vegetation::InstanceId();
        }

        void DestroyInstance([[maybe_unused]] Vegetation::InstanceId instanceId) override {}

        void DestroyAllInstances() override {}

        void Cleanup() override {}

        void RegisterMergedMeshInstance([[maybe_unused]] Vegetation::InstancePtr instance, [[maybe_unused]] IRenderNode* mergedMeshNode) override {}
        void ReleaseMergedMeshInstance([[maybe_unused]] Vegetation::InstancePtr instance) override {}

    };

    struct MockGradientRequestHandler
        : public GradientSignal::GradientRequestBus::Handler
    {
        mutable int m_count = 0;
        AZStd::function<float()> m_valueGetter;
        float m_defaultValue = -AZ::Constants::FloatMax;
        AZ::Entity m_entity;

        MockGradientRequestHandler()
        {
            GradientSignal::GradientRequestBus::Handler::BusConnect(m_entity.GetId());
        }

        ~MockGradientRequestHandler()
        {
            GradientSignal::GradientRequestBus::Handler::BusDisconnect();
        }

        float GetValue([[maybe_unused]] const GradientSignal::GradientSampleParams& sampleParams) const override
        {
            ++m_count;

            if (m_valueGetter)
            {
                return m_valueGetter();
            }
            return m_defaultValue;
        }

        bool IsEntityInHierarchy(const AZ::EntityId &) const override
        {
            return false;
        }
    };

    struct MockShapeComponentNotificationsBus
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
        AZ::Aabb m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ::Constants::FloatMax);

        MockShapeComponentNotificationsBus()
        {
        }

        ~MockShapeComponentNotificationsBus() override
        {
        }

        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override
        {
            transform = AZ::Transform::CreateTranslation(m_aabb.GetCenter());
            bounds = m_aabb;
        }

        AZ::Crc32 GetShapeType() override
        {
            return AZ::Crc32();
        }

        AZ::Aabb GetEncompassingAabb() override
        {
            return m_aabb;
        }

        bool IsPointInside(const AZ::Vector3& point) override
        {
            return m_aabb.Contains(point);
        }

        float DistanceSquaredFromPoint(const AZ::Vector3& point) override
        {
            return m_aabb.GetDistanceSq(point);
        }
    };

    struct MockSystemConfigurationRequestBus
        : public Vegetation::SystemConfigurationRequestBus::Handler
    {
        AZ::ComponentConfig* m_lastUpdated = nullptr;
        mutable const AZ::ComponentConfig* m_lastRead = nullptr;
        Vegetation::AreaSystemConfig m_areaSystemConfig;
        Vegetation::InstanceSystemConfig m_instanceSystemConfig;

        MockSystemConfigurationRequestBus()
        {
            Vegetation::SystemConfigurationRequestBus::Handler::BusConnect();
        }

        ~MockSystemConfigurationRequestBus() override
        {
            Vegetation::SystemConfigurationRequestBus::Handler::BusDisconnect();
        }

        void UpdateSystemConfig(const AZ::ComponentConfig* config) override
        {
            if (azrtti_typeid(m_areaSystemConfig) == azrtti_typeid(*config))
            {
                m_areaSystemConfig = *azrtti_cast<const Vegetation::AreaSystemConfig*>(config);
                m_lastUpdated = &m_areaSystemConfig;
            }
            else if (azrtti_typeid(m_instanceSystemConfig) == azrtti_typeid(*config))
            {
                m_instanceSystemConfig = *azrtti_cast<const Vegetation::InstanceSystemConfig*>(config);
                m_lastUpdated = &m_instanceSystemConfig;
            }
            else
            {
                m_lastUpdated = nullptr;
                AZ_Error("vegetation", false, "Invalid AZ::ComponentConfig type updated");
            }
        }

        void GetSystemConfig(AZ::ComponentConfig* config) const
        {
            if (azrtti_typeid(m_areaSystemConfig) == azrtti_typeid(*config))
            {
                *azrtti_cast<Vegetation::AreaSystemConfig*>(config) = m_areaSystemConfig;
                m_lastRead = azrtti_cast<const Vegetation::AreaSystemConfig*>(&m_areaSystemConfig);
            }
            else if (azrtti_typeid(m_instanceSystemConfig) == azrtti_typeid(*config))
            {
                *azrtti_cast<Vegetation::InstanceSystemConfig*>(config) = m_instanceSystemConfig;
                m_lastRead = azrtti_cast<const Vegetation::InstanceSystemConfig*>(&m_instanceSystemConfig);
            }
            else
            {
                m_lastRead = nullptr;
                AZ_Error("vegetation", false, "Invalid AZ::ComponentConfig type read");
            }
        }
    };

    template<class T>
    struct MockAsset : public AZ::Data::Asset<T>
    {
        void ClearData()
        {
            this->m_assetData = nullptr;
        }
    };

    struct MockAssetData
        : public AZ::Data::AssetData
    {
        void SetId(const AZ::Data::AssetId& assetId)
        {
            m_assetId = assetId;
            m_status.store(AZ::Data::AssetData::AssetStatus::Ready);
        }

        void SetStatus(AZ::Data::AssetData::AssetStatus status)
        {
            m_status.store(status);
        }
    };

    class MockShape
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        AZ::Entity m_entity;
        mutable int m_count = 0;

        MockShape()
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(m_entity.GetId());
        }

        ~MockShape()
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        AZ::Crc32 GetShapeType() override
        {
            ++m_count;
            return AZ_CRC("TestShape", 0x856ca50c);
        }

        AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
        AZ::Aabb GetEncompassingAabb() override
        {
            ++m_count;
            return m_aabb;
        }

        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity();
        AZ::Aabb m_localBounds = AZ::Aabb::CreateNull();
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override
        {
            ++m_count;
            transform = m_localTransform;
            bounds = m_localBounds;
        }

        bool m_pointInside = true;
        bool IsPointInside([[maybe_unused]] const AZ::Vector3& point) override
        {
            ++m_count;
            return m_pointInside;
        }

        float m_distanceSquaredFromPoint = 0.0f;
        float DistanceSquaredFromPoint([[maybe_unused]] const AZ::Vector3& point) override
        {
            ++m_count;
            return m_distanceSquaredFromPoint;
        }

        AZ::Vector3 m_randomPointInside = AZ::Vector3::CreateZero();
        AZ::Vector3 GenerateRandomPointInside([[maybe_unused]] AZ::RandomDistributionType randomDistribution) override
        {
            ++m_count;
            return m_randomPointInside;
        }

        bool m_intersectRay = false;
        bool IntersectRay([[maybe_unused]] const AZ::Vector3& src, [[maybe_unused]] const AZ::Vector3& dir, [[maybe_unused]] float& distance) override
        {
            ++m_count;
            return m_intersectRay;
        }
    };

    struct MockSurfaceHandler
        : public SurfaceData::SurfaceDataSystemRequestBus::Handler
    {
    public:
        mutable int m_count = 0;

        MockSurfaceHandler()
        {
            SurfaceData::SurfaceDataSystemRequestBus::Handler::BusConnect();
        }

        ~MockSurfaceHandler()
        {
            SurfaceData::SurfaceDataSystemRequestBus::Handler::BusDisconnect();
        }

        AZ::Vector3 m_outPosition = {};
        AZ::Vector3 m_outNormal = {};
        SurfaceData::SurfaceTagWeightMap m_outMasks;
        void GetSurfacePoints([[maybe_unused]] const AZ::Vector3& inPosition, [[maybe_unused]] const SurfaceData::SurfaceTagVector& masks, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            ++m_count;
            SurfaceData::SurfacePoint outPoint;
            outPoint.m_position = m_outPosition;
            outPoint.m_normal = m_outNormal;
            SurfaceData::AddMaxValueForMasks(outPoint.m_masks, m_outMasks);
            surfacePointList.push_back(outPoint);
        }

        void GetSurfacePointsFromRegion([[maybe_unused]] const AZ::Aabb& inRegion, [[maybe_unused]] const AZ::Vector2 stepSize, [[maybe_unused]] const SurfaceData::SurfaceTagVector& desiredTags,
            [[maybe_unused]] SurfaceData::SurfacePointListPerPosition& surfacePointListPerPosition) const override
        {
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataProvider([[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
            return SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        void UnregisterSurfaceDataProvider([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            ++m_count;
        }

        void UpdateSurfaceDataProvider([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle, [[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataModifier([[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
            return SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        void UnregisterSurfaceDataModifier([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            ++m_count;
        }

        void UpdateSurfaceDataModifier([[maybe_unused]] const SurfaceData::SurfaceDataRegistryHandle& handle, [[maybe_unused]] const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
        }

        void RefreshSurfaceData([[maybe_unused]] const AZ::Aabb& dirtyBounds) override
        {
            ++m_count;
        }
    };

    // wow IStatObj has a large interface
    struct MockStatObj
        : public IStatObj
    {
        int GetStreamableContentMemoryUsage([[maybe_unused]] bool bJustForDebug = false) override
        {
            return 0;
        }
        void ReleaseStreamableContent() override
        {
        }
        void GetStreamableName([[maybe_unused]] string & sName) override
        {
        }
        uint32 GetLastDrawMainFrameId() override
        {
            return uint32();
        }
        int AddRef() override
        {
            return 0;
        }
        int Release() override
        {
            return 0;
        }
        void SetFlags([[maybe_unused]] int nFlags) override
        {
        }
        int GetFlags() const override
        {
            return 0;
        }
        void SetDefaultObject([[maybe_unused]] bool state) override
        {
        }
        unsigned int GetVehicleOnlyPhysics() override
        {
            return 0;
        }
        int GetIDMatBreakable() override
        {
            return 0;
        }
        unsigned int GetBreakableByGame() override
        {
            return 0;
        }
        IIndexedMesh * GetIndexedMesh([[maybe_unused]] bool bCreateIfNone = false) override
        {
            return nullptr;
        }
        IIndexedMesh * CreateIndexedMesh() override
        {
            return nullptr;
        }
        IRenderMesh * GetRenderMesh() override
        {
            return nullptr;
        }
        phys_geometry * GetPhysGeom([[maybe_unused]] int nType = PHYS_GEOM_TYPE_DEFAULT) override
        {
            return nullptr;
        }
        IStatObj * UpdateVertices([[maybe_unused]] strided_pointer<Vec3> pVtx, [[maybe_unused]] strided_pointer<Vec3> pNormals, [[maybe_unused]] int iVtx0, [[maybe_unused]] int nVtx, [[maybe_unused]] int * pVtxMap = 0, [[maybe_unused]] float rscale = 1.f) override
        {
            return nullptr;
        }
        IStatObj * SkinVertices([[maybe_unused]] strided_pointer<Vec3> pSkelVtx, [[maybe_unused]] const Matrix34 & mtxSkelToMesh) override
        {
            return nullptr;
        }
        void SetPhysGeom([[maybe_unused]] phys_geometry * pPhysGeom, [[maybe_unused]] int nType = 0) override
        {
        }
        ITetrLattice * GetTetrLattice() override
        {
            return nullptr;
        }
        float GetAIVegetationRadius() const override
        {
            return 0.0f;
        }
        void SetAIVegetationRadius([[maybe_unused]] float radius) override
        {
        }
        void SetMaterial(_smart_ptr<IMaterial> pMaterial) override
        {
        }
        _smart_ptr<IMaterial> GetMaterial() override
        {
            return _smart_ptr<IMaterial>();
        }
        const _smart_ptr<IMaterial> GetMaterial() const override
        {
            return _smart_ptr<IMaterial>();
        }
        Vec3 GetBoxMin() override
        {
            return Vec3();
        }
        Vec3 GetBoxMax() override
        {
            return Vec3();
        }
        const Vec3 GetVegCenter() override
        {
            return Vec3();
        }
        void SetBBoxMin([[maybe_unused]] const Vec3 & vBBoxMin) override
        {
        }
        void SetBBoxMax([[maybe_unused]] const Vec3 & vBBoxMax) override
        {
        }
        float GetRadius() override
        {
            return 0.0f;
        }
        void Refresh([[maybe_unused]] int nFlags) override
        {
        }
        void Render([[maybe_unused]] const SRendParams & rParams, [[maybe_unused]] const SRenderingPassInfo & passInfo) override
        {
        }
        AABB GetAABB() override
        {
            return AABB();
        }
        float GetExtent([[maybe_unused]] EGeomForm eForm) override
        {
            return 0.0f;
        }
        void GetRandomPos([[maybe_unused]] PosNorm & ran, [[maybe_unused]] EGeomForm eForm) const override
        {
        }
        IStatObj * GetLodObject([[maybe_unused]] int nLodLevel, [[maybe_unused]] bool bReturnNearest = false) override
        {
            return nullptr;
        }
        IStatObj * GetLowestLod() override
        {
            return nullptr;
        }
        int FindNearesLoadedLOD([[maybe_unused]] int nLodIn, [[maybe_unused]] bool bSearchUp = false) override
        {
            return 0;
        }
        int FindHighestLOD([[maybe_unused]] int nBias) override
        {
            return 0;
        }
        bool LoadCGF([[maybe_unused]] const char * filename, [[maybe_unused]] bool bLod, [[maybe_unused]] unsigned long nLoadingFlags, [[maybe_unused]] const void * pData, [[maybe_unused]] const int nDataSize) override
        {
            return false;
        }
        void DisableStreaming() override
        {
        }
        void TryMergeSubObjects([[maybe_unused]] bool bFromStreaming) override
        {
        }
        bool IsUnloadable() const override
        {
            return false;
        }
        void SetCanUnload([[maybe_unused]] bool value) override
        {
        }
        string m_GetFileName;
        string & GetFileName() override
        {
            return m_GetFileName;
        }
        const string & GetFileName() const override
        {
            return m_GetFileName;
        }
        string m_cgfNodeName;
        const string& GetCGFNodeName() const override
        {
            return m_cgfNodeName;
        }
        const char * GetFilePath() const override
        {
            return m_GetFileName.c_str();
        }
        void SetFilePath([[maybe_unused]] const char * szFileName) override
        {
        }
        const char * GetGeoName() override
        {
            return nullptr;
        }
        void SetGeoName([[maybe_unused]] const char * szGeoName) override
        {
        }
        bool IsSameObject([[maybe_unused]] const char * szFileName, [[maybe_unused]] const char * szGeomName) override
        {
            return false;
        }
        Vec3 GetHelperPos([[maybe_unused]] const char * szHelperName) override
        {
            return Vec3();
        }
        Matrix34 m_GetHelperTM;
        const Matrix34 & GetHelperTM([[maybe_unused]] const char * szHelperName) override
        {
            return m_GetHelperTM;
        }
        bool IsDefaultObject() override
        {
            return false;
        }
        void FreeIndexedMesh() override
        {
        }
        void GetMemoryUsage([[maybe_unused]] ICrySizer * pSizer) const override
        {
        }
        float m_GetRadiusVert;
        float & GetRadiusVert() override
        {
            return m_GetRadiusVert;
        }
        float m_GetRadiusHors;
        float & GetRadiusHors() override
        {
            return m_GetRadiusHors;
        }
        bool IsPhysicsExist() override
        {
            return false;
        }
        void Invalidate([[maybe_unused]] bool bPhysics = false, [[maybe_unused]] float tolerance = 0.05f) override
        {
        }
        int GetSubObjectCount() const override
        {
            return 0;
        }
        void SetSubObjectCount([[maybe_unused]] int nCount) override
        {
        }
        IStatObj::SSubObject * GetSubObject([[maybe_unused]] int nIndex) override
        {
            return nullptr;
        }
        bool IsSubObject() const override
        {
            return false;
        }
        IStatObj * GetParentObject() const override
        {
            return nullptr;
        }
        IStatObj * GetCloneSourceObject() const override
        {
            return nullptr;
        }
        IStatObj::SSubObject * FindSubObject([[maybe_unused]] const char * sNodeName) override
        {
            return nullptr;
        }
        IStatObj::SSubObject * FindSubObject_CGA([[maybe_unused]] const char * sNodeName) override
        {
            return nullptr;
        }
        IStatObj::SSubObject * FindSubObject_StrStr([[maybe_unused]] const char * sNodeName) override
        {
            return nullptr;
        }
        bool RemoveSubObject([[maybe_unused]] int nIndex) override
        {
            return false;
        }
        bool CopySubObject([[maybe_unused]] int nToIndex, [[maybe_unused]] IStatObj * pFromObj, [[maybe_unused]] int nFromIndex) override
        {
            return false;
        }
        IStatObj::SSubObject m_AddSubObject;
        IStatObj::SSubObject& AddSubObject([[maybe_unused]] IStatObj * pStatObj) override
        {
            return m_AddSubObject;
        }
        int PhysicalizeSubobjects([[maybe_unused]] IPhysicalEntity * pent, [[maybe_unused]] const Matrix34 * pMtx, [[maybe_unused]] float mass, [[maybe_unused]] float density = 0.0f, [[maybe_unused]] int id0 = 0, [[maybe_unused]] strided_pointer<int> pJointsIdMap = 0, [[maybe_unused]] const char * szPropsOverride = 0) override
        {
            return 0;
        }
        int Physicalize([[maybe_unused]] IPhysicalEntity * pent, [[maybe_unused]] pe_geomparams * pgp, [[maybe_unused]] int id = 0, [[maybe_unused]] const char * szPropsOverride = 0) override
        {
            return 0;
        }
        bool IsDeformable() override
        {
            return false;
        }
        bool SaveToCGF([[maybe_unused]] const char * sFilename, [[maybe_unused]] IChunkFile ** pOutChunkFile = NULL, [[maybe_unused]] bool bHavePhysicalProxy = false) override
        {
            return false;
        }
        IStatObj * Clone([[maybe_unused]] bool bCloneGeometry, [[maybe_unused]] bool bCloneChildren, [[maybe_unused]] bool bMeshesOnly) override
        {
            return nullptr;
        }
        int SetDeformationMorphTarget([[maybe_unused]] IStatObj * pDeformed) override
        {
            return 0;
        }
        IStatObj * DeformMorph([[maybe_unused]] const Vec3 & pt, [[maybe_unused]] float r, [[maybe_unused]] float strength, [[maybe_unused]] IRenderMesh * pWeights = 0) override
        {
            return nullptr;
        }
        IStatObj * HideFoliage() override
        {
            return nullptr;
        }
        int Serialize([[maybe_unused]] TSerialize ser) override
        {
            return 0;
        }
        const char * GetProperties() override
        {
            return nullptr;
        }
        bool GetPhysicalProperties([[maybe_unused]] float & mass, [[maybe_unused]] float & density) override
        {
            return false;
        }
        IStatObj * GetLastBooleanOp([[maybe_unused]] float & scale) override
        {
            return nullptr;
        }
        bool m_RayIntersectionOutput = true;
        bool RayIntersection(SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl = 0) override
        {
            hitInfo.fDistance = 0.1f;
            return m_RayIntersectionOutput;
        }
        bool m_LineSegIntersection = true;
        bool LineSegIntersection([[maybe_unused]] const Lineseg & lineSeg, [[maybe_unused]] Vec3 & hitPos, [[maybe_unused]] int & surfaceTypeId) override
        {
            return m_LineSegIntersection;
        }
        void DebugDraw([[maybe_unused]] const SGeometryDebugDrawInfo & info, [[maybe_unused]] float fExtrdueScale = 0.01f) override
        {
        }
        void GetStatistics([[maybe_unused]] SStatistics & stats) override
        {
        }
        uint64 GetInitialHideMask() override
        {
            return uint64();
        }
        uint64 UpdateInitialHideMask([[maybe_unused]] uint64 maskAND = 0ul - 1ul, [[maybe_unused]] uint64 maskOR = 0) override
        {
            return uint64();
        }
        void SetStreamingDependencyFilePath([[maybe_unused]] const char * szFileName) override
        {
        }
        void ComputeGeometricMean([[maybe_unused]] SMeshLodInfo & lodInfo) override
        {
        }
        float GetLodDistance() const override
        {
            return 0.0f;
        }
        bool IsMeshStrippedCGF() const override
        {
            return false;
        }
        void LoadLowLODs([[maybe_unused]] bool bUseStreaming, [[maybe_unused]] unsigned long nLoadingFlags) override
        {
        }
        bool AreLodsLoaded() const override
        {
            return false;
        }
        bool CheckGarbage() const override
        {
            return false;
        }
        void SetCheckGarbage([[maybe_unused]] bool val) override
        {
        }
        int CountChildReferences() const override
        {
            return 0;
        }
        int GetUserCount() const override
        {
            return 0;
        }
        void ShutDown() override
        {
        }
        int GetMaxUsableLod() const override
        {
            return 0;
        }
        int GetMinUsableLod() const override
        {
            return 0;
        }
        SMeshBoneMapping_uint8 * GetBoneMapping() const override
        {
            return nullptr;
        }
        int GetSpineCount() const override
        {
            return 0;
        }
        SSpine * GetSpines() const override
        {
            return nullptr;
        }
        IStatObj * GetLodLevel0() override
        {
            return nullptr;
        }
        void SetLodLevel0([[maybe_unused]] IStatObj * lod) override
        {
        }
        _smart_ptr<CStatObj>* GetLods() override
        {
            return nullptr;
        }
        int GetLoadedLodsNum() override
        {
            return 0;
        }
        bool UpdateStreamableComponents([[maybe_unused]] float fImportance, [[maybe_unused]] const Matrix34A & objMatrix, [[maybe_unused]] bool bFullUpdate, [[maybe_unused]] int nNewLod) override
        {
            return false;
        }
        void RenderInternal([[maybe_unused]] CRenderObject * pRenderObject, [[maybe_unused]] uint64 nSubObjectHideMask, [[maybe_unused]] const CLodValue & lodValue, [[maybe_unused]] const SRenderingPassInfo & passInfo, [[maybe_unused]] const SRendItemSorter & rendItemSorter, [[maybe_unused]] bool forceStaticDraw) override
        {
        }
        void RenderObjectInternal([[maybe_unused]] CRenderObject * pRenderObject, [[maybe_unused]] int nLod, [[maybe_unused]] uint8 uLodDissolveRef, [[maybe_unused]] bool dissolveOut, [[maybe_unused]] const SRenderingPassInfo & passInfo, [[maybe_unused]] const SRendItemSorter & rendItemSorter, [[maybe_unused]] bool forceStaticDraw) override
        {
        }
        void RenderSubObject([[maybe_unused]] CRenderObject * pRenderObject, [[maybe_unused]] int nLod, [[maybe_unused]] int nSubObjId, [[maybe_unused]] const Matrix34A & renderTM, [[maybe_unused]] const SRenderingPassInfo & passInfo, [[maybe_unused]] const SRendItemSorter & rendItemSorter, [[maybe_unused]] bool forceStaticDraw) override
        {
        }
        void RenderSubObjectInternal([[maybe_unused]] CRenderObject * pRenderObject, [[maybe_unused]] int nLod, [[maybe_unused]] const SRenderingPassInfo & passInfo, [[maybe_unused]] const SRendItemSorter & rendItemSorter, [[maybe_unused]] bool forceStaticDraw) override
        {
        }
        void RenderRenderMesh([[maybe_unused]] CRenderObject * pObj, [[maybe_unused]] SInstancingInfo * pInstInfo, [[maybe_unused]] const SRenderingPassInfo & passInfo, [[maybe_unused]] const SRendItemSorter & rendItemSorter) override
        {
        }
        SPhysGeomArray m_GetArrPhysGeomInfo = {};
        SPhysGeomArray & GetArrPhysGeomInfo() override
        {
            return m_GetArrPhysGeomInfo;
        }
        bool IsLodsAreLoadedFromSeparateFile() override
        {
            return false;
        }
        void StartStreaming([[maybe_unused]] bool bFinishNow, [[maybe_unused]] IReadStream_AutoPtr * ppStream) override
        {
        }
        void UpdateStreamingPrioriryInternal([[maybe_unused]] const Matrix34A & objMatrix, [[maybe_unused]] float fDistance, [[maybe_unused]] bool bFullUpdate) override
        {
        }
        void SetMerged([[maybe_unused]] bool state) override
        {
        }
        int GetRenderMeshMemoryUsage() const override
        {
            return 0;
        }
        void SetLodObject([[maybe_unused]] int nLod, [[maybe_unused]] IStatObj * pLod) override
        {
        }
        int GetLoadedTrisCount() const override
        {
            return 0;
        }
        int GetRenderTrisCount() const override
        {
            return 0;
        }
        int GetRenderMatIds() const override
        {
            return 0;
        }
        bool IsUnmergable() const override
        {
            return false;
        }
        void SetUnmergable([[maybe_unused]] bool state) override
        {
        }
        int GetSubObjectMeshCount() const override
        {
            return 0;
        }
        void SetSubObjectMeshCount([[maybe_unused]] int count) override
        {
        }
        void CleanUnusedLods() override
        {
        }
        AZStd::vector<SMeshColor> m_clothData;
        AZStd::vector<SMeshColor>& GetClothData() override
        {
            return m_clothData;
        }
    };

    struct MockMeshAsset
        : public LmbrCentral::MeshAsset
    {
        AZ_RTTI(MockMeshAsset, "{C314B960-9B54-468D-B37C-065738E7487C}", LmbrCentral::MeshAsset);
        AZ_CLASS_ALLOCATOR(MeshAsset, AZ::SystemAllocator, 0);

        MockMeshAsset()
        {
            m_assetId = AZ::Uuid::CreateRandom();
            m_status.store(AZ::Data::AssetData::AssetStatus::Ready);
            m_statObj.reset(&m_mockStatObj);
            m_useCount.fetch_add(1);
            m_weakUseCount.fetch_add(1);
        }

        void Release()
        {
            m_statObj.reset(nullptr);
        }

        ~MockMeshAsset() override
        {
            Release();
        }

        MockStatObj m_mockStatObj;
    };

    struct MockMeshRequestBus
        : public LmbrCentral::MeshComponentRequestBus::Handler
    {
        AZ::Aabb m_GetWorldBoundsOutput;
        AZ::Aabb GetWorldBounds() override
        {
            return m_GetWorldBoundsOutput;
        }

        AZ::Aabb m_GetLocalBoundsOutput;
        AZ::Aabb GetLocalBounds() override
        {
            return m_GetLocalBoundsOutput;
        }

        void SetMeshAsset([[maybe_unused]] const AZ::Data::AssetId & id) override
        {
        }

        AZ::Data::Asset<LmbrCentral::MeshAsset> m_GetMeshAssetOutput;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override
        {
            return m_GetMeshAssetOutput;
        }

        bool m_GetVisibilityOutput = true;
        bool GetVisibility()
        {
            return m_GetVisibilityOutput;
        }
    };

    struct MockTransformBus
        : public AZ::TransformBus::Handler
    {
        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler&) override
        {
            ;
        }
        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler&) override
        {
            ;
        }
        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler&) override
        {
            ;
        }
        void NotifyChildChangedEvent(AZ::ChildChangeType, AZ::EntityId) override
        {
            ;
        }
        AZ::Transform m_GetLocalTMOutput;
        const AZ::Transform & GetLocalTM() override
        {
            return m_GetLocalTMOutput;
        }
        AZ::Transform m_GetWorldTMOutput;
        const AZ::Transform & GetWorldTM() override
        {
            return m_GetWorldTMOutput;
        }
        bool IsStaticTransform() override
        {
            return false;
        }
    };

    class MockShapeServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockShapeServiceComponent, "{E1687D77-F43F-439B-BB6D-B1457E94812A}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("VegetationDescriptorProviderService", 0x62e51209));
        }
    };

    class MockVegetationAreaServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockVegetationAreaServiceComponent, "{EF5292D8-411E-4660-9B31-EFAEED34B1EE}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("VegetationAreaService", 0x6a859504));
        }
    };

    class MockMeshServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockMeshServiceComponent, "{69547762-7EAB-4AC4-86FA-7486F1BBB115}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
        }
    };
}
