/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Serializer/ParticleAssetData.h>


namespace OpenParticle
{
    class ParticleSourceData final
    {
    public:
        AZ_TYPE_INFO(OpenParticle::ParticleSourceData, "{adf33c31-0d1f-a9ab-a796-e64e26122c3a}");

        using ModuleType = AZStd::unordered_map<AZStd::string, AZStd::pair<bool, AZStd::any*>>;
        using TypeId = AZStd::vector<AZStd::tuple<AZStd::string, AZ::TypeId, AZ::u8>>;
        using ModuleClassTypes = const AZStd::unordered_map<AZStd::string, TypeId>;
        using UnusedListType = AZStd::vector<AZStd::tuple<AZStd::string, AZStd::string, AZ::u8>>;
        using UnusedListNodeType = UnusedListType::node_type;

        static void Reflect(AZ::ReflectContext* context);

        ParticleSourceData();
        ~ParticleSourceData();

        ParticleSourceData(const ParticleSourceData& other);
        ParticleSourceData& operator=(const ParticleSourceData& other);

        struct EmitterInfo
        {
            AZ_CLASS_ALLOCATOR(ParticleSourceData::EmitterInfo, AZ::SystemAllocator, 0);

            void* AddEmitModule(const AZStd::any& val);
            void* AddSpawnModule(const AZStd::any& val);
            void* AddUpdateModule(const AZStd::any& val);
            void* AddEventModule(const AZStd::any& val);
            void* SetRender(const AZStd::any& val);

            bool RemoveEmitModule(const void*);
            bool RemoveSpawnModule(const void*);
            bool RemoveUpdateModule(const void*);
            bool RemoveEventModule(const void*);
            void ResetRender();

            AZStd::string m_name;
            AZStd::any m_config;
            AZStd::any m_renderConfig;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_material;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_model;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_skeletonModel;
            AZStd::list<AZStd::any> m_emitModules;
            AZStd::list<AZStd::any> m_spawnModules;
            AZStd::list<AZStd::any> m_updateModules;
            AZStd::list<AZStd::any> m_eventModules;
        };

        struct Lod
        {
            AZ_CLASS_ALLOCATOR(ParticleSourceData::Lod, AZ::SystemAllocator, 0);
            AZ::u32 m_level = 0;
            float m_distance = 0.0f;
            AZStd::vector<AZ::u32> m_emitters;
        };

        struct DetailConstant
        {
            enum ClassNameIndex : AZ::u8
            {
                EMITTER_INDEX = 0,
                SPAWN_INDEX = 1,
                PARTICLES_INDEX = 2,
                SHAPE_INDEX = 3,
                VELOCITY_INDEX = 4,
                COLOR_INDEX = 5,
                SIZE_INDEX = 6,
                FORCE_INDEX = 7,
                LIGHT_INDEX = 8,
                SUBUV_INDEX = 9,
                EVENT_INDEX = 10,
                RENDERER_INDEX = 11,
                CLASSES_COUNT
            };
            const AZStd::string classNames[CLASSES_COUNT] = { "Emitter", "Spawn", "Particles", "Shape", "Velocity", "Color",
                                                          "Size", "Force", "Light", "SubUV", "Event", "Renderer" };

            ModuleClassTypes moduleClasses = {
                { classNames[EMITTER_INDEX], { { "Emitter", azrtti_typeid<OpenParticle::EmitterConfig>(), EMITTER_CONFIG } } },
                { classNames[SPAWN_INDEX],
                  { { "Spawn Rate", azrtti_typeid<OpenParticle::EmitSpawn>(), EMITTER_EMIT },
                    { "Burst List", azrtti_typeid<OpenParticle::EmitBurstList>(), EMITTER_EMIT },
                    { "Spawn Per Unit", azrtti_typeid<OpenParticle::EmitSpawnOverMoving>(), EMITTER_EMIT } } },
                { classNames[PARTICLES_INDEX],
                  { { "Particle Lifetime", azrtti_typeid<OpenParticle::SpawnLifetime>(), EMITTER_SPAWN },
                    { "Start Color", azrtti_typeid<OpenParticle::SpawnColor>(), EMITTER_SPAWN },
                    { "Start Size", azrtti_typeid<OpenParticle::SpawnSize>(), EMITTER_SPAWN },
                    { "Start Velocity", azrtti_typeid<OpenParticle::SpawnVelDirection>(), EMITTER_SPAWN },
                    { "Start Rotation", azrtti_typeid<OpenParticle::SpawnRotation>(), EMITTER_SPAWN } } },
                { classNames[SHAPE_INDEX],
                  { { "Point", azrtti_typeid<OpenParticle::SpawnLocPoint>(), EMITTER_SPAWN },
                    { "Box", azrtti_typeid<OpenParticle::SpawnLocBox>(), EMITTER_SPAWN },
                    { "Sphere", azrtti_typeid<OpenParticle::SpawnLocSphere>(), EMITTER_SPAWN },
                    { "Cylinder", azrtti_typeid<OpenParticle::SpawnLocCylinder>(), EMITTER_SPAWN },
                    { "Mesh", azrtti_typeid<OpenParticle::SpawnLocSkeleton>(), EMITTER_SPAWN },
                    { "Torus", azrtti_typeid<OpenParticle::SpawnLocTorus>(), EMITTER_SPAWN } } },
                { classNames[VELOCITY_INDEX],
                  { { "Velocity Over Lifetime", azrtti_typeid<OpenParticle::UpdateVelocity>(), EMITTER_UPDATE },
                    { "Velocity Sector", azrtti_typeid<OpenParticle::SpawnVelSector>(), EMITTER_SPAWN },
                    { "Velocity Cone", azrtti_typeid<OpenParticle::SpawnVelCone>(), EMITTER_SPAWN },
                    { "Velocity Sphere", azrtti_typeid<OpenParticle::SpawnVelSphere>(), EMITTER_SPAWN },
                    { "Velocity Concentrate", azrtti_typeid<OpenParticle::SpawnVelConcentrate>(), EMITTER_SPAWN },
                    { "Velocity Rotate Around Point", azrtti_typeid<OpenParticle::UpdateRotateAroundPoint>(), EMITTER_UPDATE } } },
                { classNames[COLOR_INDEX], { { "Color Over Lifetime", azrtti_typeid<OpenParticle::UpdateColor>(), EMITTER_UPDATE } } },
                { classNames[SIZE_INDEX],
                  { { "Size Over Lifetime", azrtti_typeid<OpenParticle::UpdateSizeLinear>(), EMITTER_UPDATE },
                    { "Size By Speed", azrtti_typeid<OpenParticle::UpdateSizeByVelocity>(), EMITTER_UPDATE },
                    { "Size Scale", azrtti_typeid<OpenParticle::SizeScale>(), EMITTER_UPDATE } } },
                { classNames[FORCE_INDEX],
                  { { "Acceleration", azrtti_typeid<OpenParticle::UpdateConstForce>(), EMITTER_UPDATE },
                    { "Drag", azrtti_typeid<OpenParticle::UpdateDragForce>(), EMITTER_UPDATE },
                    { "Vortex", azrtti_typeid<OpenParticle::UpdateVortexForce>(), EMITTER_UPDATE },
                    { "Noise", azrtti_typeid<OpenParticle::UpdateCurlNoiseForce>(), EMITTER_UPDATE },
                    { "Collision", azrtti_typeid<OpenParticle::ParticleCollision>(), EMITTER_UPDATE } } },
                { classNames[LIGHT_INDEX], { { "Light", azrtti_typeid<OpenParticle::SpawnLightEffect>(), EMITTER_SPAWN } } },
                { classNames[SUBUV_INDEX], { { "SubUV", azrtti_typeid<OpenParticle::UpdateSubUv>(), EMITTER_UPDATE } } },
                { classNames[EVENT_INDEX],
                  { { "Send Spawn Event", azrtti_typeid<OpenParticle::SpawnLocationEvent>(), EMITTER_SPAWN },
                    { "Send Location Event", azrtti_typeid<OpenParticle::UpdateLocationEvent>(), EMITTER_EVENT },
                    { "Send Death Event", azrtti_typeid<OpenParticle::UpdateDeathEvent>(), EMITTER_EVENT },
                    { "Send Collision Event", azrtti_typeid<OpenParticle::UpdateCollisionEvent>(), EMITTER_EVENT },
                    { "Send Inheritance Event", azrtti_typeid<OpenParticle::UpdateInheritanceEvent>(), EMITTER_EVENT },
                    { "Event Handler", azrtti_typeid<OpenParticle::ParticleEventHandler>(), EMITTER_EMIT },
                    { "Inheritance Handler", azrtti_typeid<OpenParticle::InheritanceHandler>(), EMITTER_EMIT } } },
                { classNames[RENDERER_INDEX],
                  { { "Sprite Renderer", azrtti_typeid<OpenParticle::SpriteConfig>(), RENDER_TYPE },
                    { "Mesh Renderer", azrtti_typeid<OpenParticle::MeshConfig>(), RENDER_TYPE },
                    { "Ribbon Renderer", azrtti_typeid<OpenParticle::RibbonConfig>(), RENDER_TYPE } } }
            };

            enum EmitterClassIndex : AZ::u8
            {
                EMITTER_CONFIG,
                EMITTER_EMIT,
                EMITTER_SPAWN,
                EMITTER_UPDATE,
                EMITTER_EVENT,
                RENDER_TYPE
            };

            enum TypeIdIndex : AZ::u8
            {
                MODULE_NAME,
                TYPE_ID,
                LIST_INDEX
            };

            enum ParticlesIndex : AZ::u8
            {
                PARTICLE_LIFETIME,
                PARTICLE_START_COLOR,
                PARTICLE_START_SIZE,
                PARTICLE_START_VELOCITY,
                PARTICLE_START_ROTATION
            };

            enum SizeIndex : AZ::u8
            {
                SIZE_LINEAR,
                SIZE_BY_VELOCITY,
                SIZE_SCALE
            };

            enum VelocityIndex : AZ::u8
            {
                VELOCITY_OVER_LIFETIME,
                VELOCITY_SECTOR,
                VELOCITY_CONE,
                VELOCITY_SPHERE,
                VELOCITY_CONCENTRATE,
                VELOCITY_ROTATEAROUNDPOINT
            };

            enum ForceIndex : AZ::u8
            {
                FORCE_ACCELERATION,
                FORCE_DRAG,
                FORCE_VORTEX,
                FORCE_NOISE,
                FORCE_COLLISION
            };

            enum EventIndex : AZ::u8
            {
                SPAWN_EVENT,
                LOCATION_EVENT,
                DEATH_EVENT,
                COLLISION_EVENT,
                INHERITANCE_EVENT,
                EVENT_HANDLER,
                INHERITANCE_HANDLER
            };

            enum AssetIndex : AZ::u8
            {
                ASSET_MATERIAL,
                ASSET_MODEL,
                ASSET_SKELETON_MODEL,
            };
        };

        struct DetailInfo
        {
            AZ_CLASS_ALLOCATOR(ParticleSourceData::DetailInfo, AZ::SystemAllocator, 0);

            bool m_isUse = true;
            bool m_solo = false;
            AZStd::string m_name;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_material;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_model;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_skeletonModel;
            AZStd::unordered_map<AZStd::string, ModuleType> m_modules;

        private:
            friend ParticleSourceData;
            AZStd::list<AZStd::any> m_unusedModules;

            UnusedListType m_unusedListForDetail;
            UnusedListType m_unusedListForClass;

            AZStd::unordered_map<AZ::TypeId, AZ::u8> m_itemNumber;

            enum UnusedListTypeIndex : AZ::u8
            {
                CLASS_NAME,
                MODULE_NAME,
                LIST_INDEX
            };
        };

        EmitterInfo* AddEmitter(const AZStd::string&);
        void RemoveEmitter(EmitterInfo*);

        void AddCurve(const Curve& curve);
        void AddCurve(Curve* curve);
        void RemoveCurve(Curve* curve);

        void AddRandom(const Random& random);
        void AddRandom(Random* random);
        void RemoveRandom(Random* random);

        void Normalize();

        void ToRuntime();
        void EmitterToRuntime();
        void DistributionToRuntime();
        void ToEditor();

        ParticleAssetData::CreateParticleResult CreateParticleAsset(
            AZ::Data::AssetId assetId, AZStd::string_view sourceFilePath, bool elevateWarnings = true) const;

        bool CheckDistributionIndex();

        bool CheckEmitterNames() const;

        // Convert emitter data to detail data
        void EmittersToDetails();

        // copy the currently selected assets in the detail object (being edited) to the emitter.
        void UpdateEmitterAsset(const AZStd::string& emitterName, AZ::u8 index);

        // copy the emitter name (being changed) into the actual emitter and details to keep in sync with ui.
        void UpdateEmitterName(const AZStd::string& oldEmitterName, const AZStd::string& newEmitterName);

        // Add/Remove Emitter in the Particle Editor
        DetailInfo* AddDetail(const AZStd::string&);
        DetailInfo* CopyDetail(AZStd::string&, const AZStd::string&);
        EmitterInfo* CopyEmitter();
        DetailInfo* SetDestItem(EmitterInfo* destEmitter, DetailInfo* destDetail, AZStd::string destName);
        void CopyDetailFromEmitter(EmitterInfo* sourceEmitter, EmitterInfo* destEmitter, DetailInfo* destDetail);
        void CopyDetailFromDetail(DetailInfo* sourceDetail, EmitterInfo* destEmitter, DetailInfo* destDetail);
        void CopyDistributions(AZStd::vector<AZStd::any*>& modules);
        void RemoveDetail(DetailInfo*);

        // Select/Unselect Emitter in the Particle Editor
        void SelectDetail(AZStd::string emitterName);
        void UnselectDetail(AZStd::string emitterName);

        // Add/Remove module to/from emitter
        void SelectModule(DetailInfo* detailInfo, AZStd::string className, AZStd::string moduleName);
        void UnselectModule(DetailInfo* detailInfo, AZStd::string className, AZStd::string moduleName);

        // Select/Unselect Class in the Particle Editor
        void SelectClass(AZStd::string emitterName, AZStd::string className);
        void UnselectClass(AZStd::string emitterName, AZStd::string className);

        void AddEventHandler(DetailInfo* detail, AZ::u8 index);
        void DeleteEventHandler(DetailInfo* detail, AZStd::string& name);

        bool CheckModuleState(const AZ::TypeId& id) const;
        void SortEmitters(AZStd::vector<AZStd::string>& emitterNames);

        void UpdateDetailSoloState(AZStd::string& name, bool solo);
        bool SoloChecked(AZStd::string& name);

        void ClearCopyCache();

        void UpdateDistributionIndexes();
        void UpdateRandomIndexes();
        void UpdateCurveIndexes();

        AZStd::string GetModuleKey(const AZ::TypeId& id, const AZ::TypeId& paramId) const;

        AZStd::string m_name = "ParticleSystem";
        AZStd::any m_config;
        AZStd::any m_preWarm;
        AZStd::vector<EmitterInfo*> m_emitters;
        AZStd::vector<DetailInfo*> m_details;
        EmitterInfo* m_destEmitter = nullptr;
        DetailInfo* m_destDetail = nullptr;
        DetailInfo* m_currentDetailInfo = nullptr;
        DetailConstant m_detailConstant;
        AZStd::vector<Lod> m_lods;
        Distribution m_distribution;

        enum DistributionIndex : AZ::u8
        {
            PTR_INDEX,
            PARAM_INDEX,
            CURVE_INDEX
        };

        OpenParticle::ParticleAssetData m_assetData;

        AZ::SerializeContext* m_serializeContext = nullptr;

    private:
        void CheckParam(AZStd::any& module);
        void ConvertOldVersionDistribution(AZStd::any& module);
        void Reset();

        void RebuildLODs();
        void UpdateLODIndex(AZ::u32 index);

        void EmitterInfoToDetailInfo(DetailInfo* detailInfo, EmitterInfo* emitterInfo);
        template<
            class T,
            class = AZStd::is_pointer<T>,
            class = AZStd::enable_if_t<AZStd::is_same_v<T, EmitterInfo*> || AZStd::is_same_v<T, DetailInfo*>>>
        T GetPointerFromEmitterName(const AZStd::string& emitterName, const AZStd::vector<T>& list) const
        {
            auto iter = AZStd::find_if(
                list.begin(), list.end(),
                [&emitterName](const T& iter)
                {
                    if constexpr (AZStd::is_same_v<T, EmitterInfo*>)
                    {
                        return iter->m_name == emitterName;
                    }
                    else if constexpr (AZStd::is_same_v<T, DetailInfo*>)
                    {
                        return iter->m_name == emitterName;
                    }
                });
            return (iter != list.end()) ? *iter : nullptr;
        };
        void RemoveDetailModule(
            DetailInfo* detailInfo, AZStd::string& className, AZStd::string& moduleName, AZStd::list<AZStd::any>& list);
        void ReserveRemovedModules(const AZStd::string& className, const ModuleType* moduleType, UnusedListType* unusedList);
        void RemovedModulesSort(DetailInfo* detailInfo, EmitterInfo* emitterInfo);
        void RemovedModulesModuleSort(DetailInfo* detailInfo, EmitterInfo* emitterInfo) const;
        const TypeId::value_type& GetTypeIdItem(const AZStd::string& className, const AZStd::string& moduleName) const;
        void RemoveUnusedModule(DetailInfo* detailInfo, AZStd::any& any) const;
        void SetDefaultModules(DetailInfo* detailInfo);

        void StashDistribution(AZStd::any& any, const void* detailInfo, const AZStd::string& className, const AZStd::string& moduleName);
        bool PopDistribution(AZStd::any& any, const void* detailInfo, const AZStd::string& className, const AZStd::string& moduleName, bool stash = false);

        void SortSpawnModules(
            DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string className, const AZStd::string moduleName) const;
        void SortUpdateModules(DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string moduleName) const;

        void SortSpawnEventModule(DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string moduleName) const;
        void SortVelocityModules(
            DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string className, const AZStd::string moduleName) const;
        void SortSizeModules(DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string moduleName) const;
        void ChangeModuleToEnd(AZStd::list<AZStd::any>& list, AZStd::any*& className) const;

        void UpdateRandomIndexes(AZStd::list<AZStd::any>& list);
        void UpdateCurveIndexes(AZStd::list<AZStd::any>& list);

        void SetEventHandler(bool unSelect = false, AZStd::string emitterName = "");
    };
} // namespace OpenParticle

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleSourceData::EmitterInfo, "{fe234883-7dc1-0dbb-9497-a3d5038e812f}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleSourceData::Lod, "{0E589EAF-1EE2-4A49-BBD2-391281EDA2B8}");
} // namespace AZ
