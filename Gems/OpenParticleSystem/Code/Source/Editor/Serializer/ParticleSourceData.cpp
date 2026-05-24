/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystem/ParticleConfig.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Editor/DistributionCacheInterface.h>
#include <Editor/Serializer/ParticleSystemSerializer.h>
#include <Serializer/ParticleSourceData.h>
#include <Serializer/ParticleSourceDataSerializer.h>
#include <numeric>

namespace OpenParticle
{
    void ParticleSourceData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<ParticleSourceDataSerializer>()->HandlesType<ParticleSourceData>();
            jsonContext->Serializer<ParticleEmitterInfoSerializer>()->HandlesType<ParticleSourceData::EmitterInfo>();
            jsonContext->Serializer<ParticleLODSerializer>()->HandlesType<ParticleSourceData::Lod>();
            jsonContext->Serializer<ParticleDistributionSerializer>()->HandlesType<Distribution>();
        }
        else if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleSourceData>()
                ->Version(0)
                ->Field("emitters", &ParticleSourceData::m_emitters)
                ->Field("LODs", &ParticleSourceData::m_lods)
                ->Field("distribution", &ParticleSourceData::m_distribution);
        }

        ParticleAssetData::Reflect(context);
    }

    ParticleSourceData::ParticleSourceData()
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext != nullptr, "invalid serialize context");
    }

    ParticleSourceData::~ParticleSourceData()
    {
        for (auto& emitter : m_emitters)
        {
            delete emitter;
            emitter = nullptr;
        }
        m_emitters.clear();
        for (auto& detail : m_details)
        {
            delete detail;
            detail = nullptr;
        }
        m_distribution.Clear();

        if (m_destEmitter != nullptr)
        {
            delete m_destEmitter;
            m_destEmitter = nullptr;
        }
        if (m_destDetail != nullptr)
        {
            delete m_destDetail;
            m_destDetail = nullptr;
        }
    }

    ParticleSourceData::ParticleSourceData(const ParticleSourceData& other)
    {
        Reset();
        m_name = other.m_name;
        m_config = other.m_config;
        for (auto& emitter : other.m_emitters)
        {
            auto newEmitter = aznew EmitterInfo();
            newEmitter->m_name = emitter->m_name;
            newEmitter->m_config = emitter->m_config;
            newEmitter->m_renderConfig = emitter->m_renderConfig;
            newEmitter->m_material = emitter->m_material;
            newEmitter->m_model = emitter->m_model;
            newEmitter->m_skeletonModel = emitter->m_skeletonModel;
            newEmitter->m_emitModules.assign(emitter->m_emitModules.begin(), emitter->m_emitModules.end());
            newEmitter->m_spawnModules.assign(emitter->m_spawnModules.begin(), emitter->m_spawnModules.end());
            newEmitter->m_updateModules.assign(emitter->m_updateModules.begin(), emitter->m_updateModules.end());
            newEmitter->m_eventModules.assign(emitter->m_eventModules.begin(), emitter->m_eventModules.end());
            m_emitters.emplace_back(newEmitter);
        }
        m_lods.assign(other.m_lods.begin(), other.m_lods.end());
        for (auto random : other.m_distribution.randoms)
        {
            auto val = aznew Random(*random);
            this->m_distribution.randoms.emplace_back(val);
        }
        for (auto curve : other.m_distribution.curves)
        {
            auto val = aznew Curve(*curve);
            this->m_distribution.curves.emplace_back(val);
        }
        m_assetData = other.m_assetData;
        m_serializeContext = other.m_serializeContext;
    }

    ParticleSourceData& ParticleSourceData::operator = (const ParticleSourceData& other)
    {
        this->Reset();
        this->m_name = other.m_name;
        this->m_config = other.m_config;
        for (auto& emitter : other.m_emitters)
        {
            auto newEmitter = aznew EmitterInfo();
            newEmitter->m_name = emitter->m_name;
            newEmitter->m_config = emitter->m_config;
            newEmitter->m_renderConfig = emitter->m_renderConfig;
            newEmitter->m_material = emitter->m_material;
            newEmitter->m_model = emitter->m_model;
            newEmitter->m_skeletonModel = emitter->m_skeletonModel;
            newEmitter->m_emitModules.assign(emitter->m_emitModules.begin(), emitter->m_emitModules.end());
            newEmitter->m_spawnModules.assign(emitter->m_spawnModules.begin(), emitter->m_spawnModules.end());
            newEmitter->m_updateModules.assign(emitter->m_updateModules.begin(), emitter->m_updateModules.end());
            newEmitter->m_eventModules.assign(emitter->m_eventModules.begin(), emitter->m_eventModules.end());
            this->m_emitters.emplace_back(newEmitter);
        }
        this->m_lods.assign(other.m_lods.begin(), other.m_lods.end());
        for (auto random : other.m_distribution.randoms)
        {
            auto val = aznew Random(*random);
            this->m_distribution.randoms.emplace_back(val);
        }
        for (auto curve : other.m_distribution.curves)
        {
            auto val = aznew Curve(*curve);
            this->m_distribution.curves.emplace_back(val);
        }
        this->m_assetData = other.m_assetData;
        this->m_serializeContext = other.m_serializeContext;
        return *this;
    }

    void ParticleSourceData::CheckParam(AZStd::any& module)
    {
        auto* classData = m_serializeContext->FindClassData(module.get_type_info().m_id);
        if (classData == nullptr)
        {
            return;
        }
        auto attribute = classData->FindAttribute(AZ_CRC("CHECK_PARAM"));
        if (attribute != nullptr)
        {
            AZ::AttributeInvoker(static_cast<void*>(nullptr), attribute).Invoke<void>(module);
        }
    }

    void ParticleSourceData::ConvertOldVersionDistribution(AZStd::any& module)
    {
        auto* classData = m_serializeContext->FindClassData(module.get_type_info().m_id);
        if (classData == nullptr)
        {
            return;
        }
        auto attribute = classData->FindAttribute(AZ_CRC("CONVERTOR"));
        if (attribute != nullptr)
        {
            AZ::AttributeInvoker(static_cast<void*>(nullptr), attribute).Invoke<void>(module, m_distribution);
        }
    }

    void ParticleSourceData::Reset()
    {
        for (auto& emitter : m_emitters)
        {
            delete emitter;
            emitter = nullptr;
        }
        m_emitters.clear();
        m_lods.clear();
        m_distribution.Clear();
        for (auto& detail : m_details)
        {
            delete detail;
            detail = nullptr;
        }
        m_details.clear();
    }

    void ParticleSourceData::RebuildLODs()
    {
    }

    bool ParticleSourceData::CheckDistributionIndex()
    {
        return m_distribution.CheckDistributionIndex();
    }

    bool ParticleSourceData::CheckEmitterNames() const
    {
        AZStd::set<AZStd::string_view> compare;
        for (auto& iter : m_emitters)
        {
            compare.insert(iter->m_name);
        }
        return m_emitters.size() == compare.size();
    }

    void ParticleSourceData::Normalize()
    {
        CheckParam(m_preWarm);
        for (auto& emitter : m_emitters)
        {
            if (!emitter->m_config.empty())
            {
                CheckParam(emitter->m_config);
            }

            if (!emitter->m_renderConfig.empty())
            {
                CheckParam(emitter->m_renderConfig);
            }

            for (auto& emitModule : emitter->m_emitModules)
            {
                if (!emitModule.empty())
                {
                    CheckParam(emitModule);
                    ConvertOldVersionDistribution(emitModule);
                }
            }

            for (auto& spawnModule : emitter->m_spawnModules)
            {
                if (!spawnModule.empty())
                {
                    CheckParam(spawnModule);
                    ConvertOldVersionDistribution(spawnModule);
                }
            }

            for (auto& updateModule : emitter->m_updateModules)
            {
                if (!updateModule.empty())
                {
                    CheckParam(updateModule);
                    ConvertOldVersionDistribution(updateModule);
                }
            }

            for (auto& eventModule : emitter->m_eventModules)
            {
                if (!eventModule.empty())
                {
                    CheckParam(eventModule);
                }
            }
        }
    }

    void ParticleSourceData::ToRuntime()
    {
        m_assetData.Reset();
        m_assetData.m_name = m_name;
        m_assetData.m_config = DataConvertor::ToRuntime(m_config);
        m_assetData.m_preWarm = DataConvertor::ToRuntime(m_preWarm);

        EmitterToRuntime();

        for (auto& editLod : m_lods)
        {
            ParticleLOD lod;
            lod.m_distance = editLod.m_distance;
            lod.m_emitters = editLod.m_emitters;
            m_assetData.m_lods.emplace_back(lod);
        }

        DistributionToRuntime();
    }

    void ParticleSourceData::EmitterToRuntime()
    {
        for (auto& editEmitter : m_emitters)
        {
            auto emitter = aznew ParticleAssetData::EmitterInfo();
            emitter->m_name = editEmitter->m_name;
            emitter->m_material = editEmitter->m_material;
            emitter->m_model = editEmitter->m_model;
            emitter->m_skeletonModel = editEmitter->m_skeletonModel;
            emitter->m_config = DataConvertor::ToRuntime(editEmitter->m_config);
            emitter->m_renderConfig = DataConvertor::ToRuntime(editEmitter->m_renderConfig);
            for (auto& emitModule : editEmitter->m_emitModules)
            {
                AZStd::any val = DataConvertor::ToRuntime(emitModule);
                emitter->m_emitModules.emplace_back(val);
            }

            for (auto& spawnModule : editEmitter->m_spawnModules)
            {
                if (spawnModule.is<OpenParticle::SpawnLocSkeleton>())
                {
                    auto skeleton = AZStd::any_cast<OpenParticle::SpawnLocSkeleton>(spawnModule);
                    emitter->m_meshSampleType = skeleton.sampleType;
                }
                AZStd::any val = DataConvertor::ToRuntime(spawnModule);
                emitter->m_spawnModules.emplace_back(val);
            }

            for (auto& updateModule : editEmitter->m_updateModules)
            {
                AZStd::any val = DataConvertor::ToRuntime(updateModule);
                emitter->m_updateModules.emplace_back(val);
            }

            for (auto& eventModule : editEmitter->m_eventModules)
            {
                AZStd::any val = DataConvertor::ToRuntime(eventModule);
                emitter->m_eventModules.emplace_back(val);
            }
            m_assetData.m_emitters.emplace_back(emitter);
        }
    }

    void ParticleSourceData::DistributionToRuntime()
    {
        for (auto editRandom : m_distribution.randoms)
        {
            ParticleRandom particleRandom;
            particleRandom.tickMode = DataConvertor::RandomTickModeToRuntime(editRandom->tickMode);
            particleRandom.min = editRandom->min;
            particleRandom.max = editRandom->max;
            m_assetData.m_distribution.randoms.emplace_back(particleRandom);
        }

        for (auto editCurve : m_distribution.curves)
        {
            ParticleCurve particleCurve;
            particleCurve.extrapModeLeft = DataConvertor::ExtrapModeToRuntime(editCurve->leftExtrapMode);
            particleCurve.extrapModeRight = DataConvertor::ExtrapModeToRuntime(editCurve->rightExtrapMode);
            particleCurve.valueFactor = editCurve->valueFactor;
            particleCurve.timeFactor = editCurve->timeFactor;
            particleCurve.tickMode = DataConvertor::CurveTickModeToRuntime(editCurve->tickMode);
            for (auto& point : editCurve->keyPoints)
            {
                particleCurve.keyPoints.emplace_back(
                    SimuCore::ParticleCore::KeyPoint(point.time, point.value, DataConvertor::InterpModeToRuntime(point.interpMode)));
            }
            m_assetData.m_distribution.curves.emplace_back(particleCurve);
        }
    }

    void ParticleSourceData::ToEditor()
    {
        m_distribution.ClearDistributionIndex();
        for (auto& emitterInfo : m_emitters)
        {
            m_distribution.GetDistributionIndex(emitterInfo->m_emitModules);
            m_distribution.GetDistributionIndex(emitterInfo->m_spawnModules);
            m_distribution.GetDistributionIndex(emitterInfo->m_updateModules);
            m_distribution.GetDistributionIndex(emitterInfo->m_eventModules);
            m_distribution.GetDistributionIndex(emitterInfo->m_renderConfig);
        }
        m_distribution.SortDistributionIndex();
        m_distribution.RebuildDistribution();
        m_distribution.RebuildDistributionIndex();
    }

    ParticleSourceData::EmitterInfo* ParticleSourceData::AddEmitter(const AZStd::string& str)
    {
        auto emitter = aznew ParticleSourceData::EmitterInfo();
        emitter->m_name = str;
        m_emitters.emplace_back(emitter);
        return emitter;
    }

    void ParticleSourceData::RemoveEmitter(ParticleSourceData::EmitterInfo* info)
    {
        auto iter = AZStd::find(m_emitters.begin(), m_emitters.end(), info);
        if (iter != m_emitters.end())
        {
            UpdateLODIndex(static_cast<AZ::u32>(iter - m_emitters.begin()));
            delete *iter;
            m_emitters.erase(iter);
        }
    }
    void ParticleSourceData::UpdateLODIndex(AZ::u32 index)
    {
        for (auto& lod : m_lods)
        {
            auto iter = AZStd::find(lod.m_emitters.begin(), lod.m_emitters.end(), index);
            if (iter != lod.m_emitters.end())
            {
                lod.m_emitters.erase(iter);
            }
            for (auto it = lod.m_emitters.begin(); it < lod.m_emitters.end(); it++)
            {
                if (*it > index)
                {
                    *it = *it - 1;
                }
            }
        }
    }
    void ParticleSourceData::SortEmitters(AZStd::vector<AZStd::string>& emitterNames)
    {
        AZStd::vector<EmitterInfo*> emitters;
        for (const auto& name : emitterNames)
        {
            auto it = AZStd::find_if(m_emitters.begin(), m_emitters.end(), [&name](auto emitter)
                {
                    return emitter->m_name.compare(name) == 0;
                });
            if (it != m_emitters.end())
            {
                emitters.push_back(*it);
                m_emitters.erase(it);
            }
        }
        m_emitters.clear();
        m_emitters = emitters;
        SetEventHandler();
    }

    void ParticleSourceData::AddCurve(const Curve& val)
    {
        auto* curve = aznew Curve(val);
        m_distribution.curves.emplace_back(curve);
    }

    void ParticleSourceData::AddCurve(Curve* val)
    {
        auto* curve = val;
        auto iter = AZStd::find(m_distribution.curves.begin(), m_distribution.curves.end(), curve);
        if (iter != m_distribution.curves.end())
        {
            curve = aznew Curve(*val);
        }
        m_distribution.curves.emplace_back(curve);
    }

    void ParticleSourceData::RemoveCurve(Curve* curve)
    {
        auto iter = AZStd::find(m_distribution.curves.begin(), m_distribution.curves.end(), curve);
        if (iter != m_distribution.curves.end())
        {
            delete *iter;
            m_distribution.curves.erase(iter);
        }
    }

    void ParticleSourceData::AddRandom(const Random& val)
    {
        auto* random = aznew Random(val);
        m_distribution.randoms.emplace_back(random);
    }

    void ParticleSourceData::AddRandom(Random* val)
    {
        auto* random = val;
        auto iter = AZStd::find(m_distribution.randoms.begin(), m_distribution.randoms.end(), random);
        if (iter != m_distribution.randoms.end())
        {
            random = aznew Random(*val);
        }
        m_distribution.randoms.emplace_back(random);
    }

    void ParticleSourceData::RemoveRandom(Random* random)
    {
        auto iter = AZStd::find(m_distribution.randoms.begin(), m_distribution.randoms.end(), random);
        if (iter != m_distribution.randoms.end())
        {
            delete *iter;
            m_distribution.randoms.erase(iter);
        }
    }

    void* AddModule(AZStd::list<AZStd::any>& list, const AZStd::any& val)
    {
        list.emplace_back(val);
        return AZStd::any_cast<void>(&list.back());
    }

    bool RemoveModule(AZStd::list<AZStd::any>& list, const void* val)
    {
        auto iter = AZStd::find_if(list.begin(), list.end(),
            [val](const AZStd::any& v)
            {
                return val == AZStd::any_cast<void>(&v);
            });
        if (iter != list.end())
        {
            list.erase(iter);
            return true;
        }
        return false;
    }

    void* ParticleSourceData::EmitterInfo::AddEmitModule(const AZStd::any& val)
    {
        if (val.is<OpenParticle::EmitBurstList>())
        {
            for (auto& module : m_emitModules)
            {
                if (module.is<OpenParticle::EmitBurstList>())
                {
                    return nullptr;
                }
            }
        }
        return AddModule(m_emitModules, val);
    }

    void* ParticleSourceData::EmitterInfo::AddSpawnModule(const AZStd::any& val)
    {
        return AddModule(m_spawnModules, val);
    }

    void* ParticleSourceData::EmitterInfo::AddUpdateModule(const AZStd::any& val)
    {
        return AddModule(m_updateModules, val);
    }

    void* ParticleSourceData::EmitterInfo::AddEventModule(const AZStd::any& val)
    {
        return AddModule(m_eventModules, val);
    }

    void* ParticleSourceData::EmitterInfo::SetRender(const AZStd::any& val)
    {
        m_renderConfig = val;
        return AZStd::any_cast<void>(&m_renderConfig);
    }

    bool ParticleSourceData::EmitterInfo::RemoveEmitModule(const void* val)
    {
        return RemoveModule(m_emitModules, val);
    }

    bool ParticleSourceData::EmitterInfo::RemoveSpawnModule(const void* val)
    {
        return RemoveModule(m_spawnModules, val);
    }

    bool ParticleSourceData::EmitterInfo::RemoveUpdateModule(const void* val)
    {
        return RemoveModule(m_updateModules, val);
    }

    bool ParticleSourceData::EmitterInfo::RemoveEventModule(const void* val)
    {
        return RemoveModule(m_eventModules, val);
    }

    void ParticleSourceData::EmitterInfo::ResetRender()
    {
        m_renderConfig.clear();
    }

    ParticleAssetData::CreateParticleResult ParticleSourceData::CreateParticleAsset(
        AZ::Data::AssetId assetId, AZStd::string_view sourceFilePath, bool elevateWarnings) const
    {
        return m_assetData.CreateParticleAsset(assetId, sourceFilePath, elevateWarnings);
    }

    // Convert emitters data to details data
    void ParticleSourceData::EmittersToDetails()
    {
        for (auto& emitter : m_emitters)
        {
            m_details.emplace_back(aznew DetailInfo());
            EmitterInfoToDetailInfo(m_details.back(), emitter);
        }
    }

    // Convert emitter to detail (private)
    void ParticleSourceData::EmitterInfoToDetailInfo(DetailInfo* detailInfo, EmitterInfo* emitterInfo)
    {
        for (auto& name : m_detailConstant.classNames)
        {
            detailInfo->m_modules.emplace(name, ModuleType());
        }

        detailInfo->m_name = emitterInfo->m_name;
        detailInfo->m_material = emitterInfo->m_material;
        detailInfo->m_model =  emitterInfo->m_model;
        detailInfo->m_skeletonModel = emitterInfo->m_skeletonModel;
        detailInfo->m_isUse = true;

        AZStd::unordered_map<AZ::TypeId, AZStd::vector<AZStd::any*>> emitterModules;
        auto GetModuleTypeId = [&emitterModules](AZStd::list<AZStd::any>& list)
        {
            for (auto& module : list)
            {
                emitterModules[module.type()].push_back(&module);
            }
        };

        AZStd::invoke(GetModuleTypeId, emitterInfo->m_emitModules);
        AZStd::invoke(GetModuleTypeId, emitterInfo->m_spawnModules);
        AZStd::invoke(GetModuleTypeId, emitterInfo->m_updateModules);
        AZStd::invoke(GetModuleTypeId, emitterInfo->m_eventModules);
        emitterModules[emitterInfo->m_config.type()].push_back(&emitterInfo->m_config);
        emitterModules[emitterInfo->m_renderConfig.type()].push_back(&emitterInfo->m_renderConfig);

        for (auto& moduleClass : m_detailConstant.moduleClasses)
        {
            auto& moduleMap = detailInfo->m_modules[moduleClass.first];
            for (auto& tuple : moduleClass.second)
            {
                if (!emitterModules[AZStd::get<DetailConstant::TYPE_ID>(tuple)].empty())
                {
                    for (auto& module : emitterModules[AZStd::get<DetailConstant::TYPE_ID>(tuple)])
                    {
                        auto name = AZStd::get<DetailConstant::MODULE_NAME>(tuple) +
                            ((detailInfo->m_itemNumber[module->type()]++ == 0)
                                 ? ""
                                 : AZStd::to_string(detailInfo->m_itemNumber[module->type()] - 1));
                        moduleMap[name] = { true, module };
                    }
                }
                else
                {
                    detailInfo->m_unusedModules.emplace_back(m_serializeContext->CreateAny(AZStd::get<DetailConstant::TYPE_ID>(tuple)));
                    moduleMap[AZStd::get<DetailConstant::MODULE_NAME>(tuple)] = { false, &detailInfo->m_unusedModules.back() };
                }
            }
        }
    }

    // Add Emitter in the Particle Editor
    ParticleSourceData::DetailInfo* ParticleSourceData::AddDetail(const AZStd::string& str)
    {
        auto emitter = AddEmitter(str);
        m_details.emplace_back(aznew ParticleSourceData::DetailInfo());
        EmitterInfoToDetailInfo(m_details.back(), emitter);
        SetDefaultModules(m_details.back());
        return m_details.back();
    }

    // Remove Emitter in the Particle Editor
    void ParticleSourceData::RemoveDetail(ParticleSourceData::DetailInfo* info)
    {
        auto iter = AZStd::find(m_details.begin(), m_details.end(), info);
        if (iter != m_details.end())
        {
            RemoveEmitter(GetPointerFromEmitterName(info->m_name, m_emitters));
            delete *iter;
            m_details.erase(iter);
        }
    }

    // Select Emitter in the Particle Editor
    void ParticleSourceData::SelectDetail(AZStd::string emitterName)
    {
        EmitterInfo* emitter = GetPointerFromEmitterName(emitterName, m_emitters);
        DetailInfo* detailInfo = GetPointerFromEmitterName(emitterName, m_details);
        if (detailInfo == nullptr || emitter != nullptr)
        {
            return;
        }
        detailInfo->m_isUse = true;
        auto emitterInfo = AddEmitter(emitterName);
        emitterInfo->m_name = detailInfo->m_name;
        detailInfo->m_name = emitterInfo->m_name;
        emitterInfo->m_material = detailInfo->m_material;
        detailInfo->m_material = emitterInfo->m_material;
        emitterInfo->m_model = detailInfo->m_model;
        detailInfo->m_model = emitterInfo->m_model;
        emitterInfo->m_skeletonModel = detailInfo->m_skeletonModel;
        detailInfo->m_skeletonModel = emitterInfo->m_skeletonModel;

        for (auto& tuple : detailInfo->m_unusedListForDetail)
        {
            SelectModule(
                detailInfo, AZStd::get<DetailInfo::CLASS_NAME>(tuple), AZStd::get<DetailInfo::MODULE_NAME>(tuple));
        }
        detailInfo->m_unusedListForDetail.clear();
    }

    void ParticleSourceData::UpdateEmitterAsset(const AZStd::string& emitterName, const AZ::u8 index)
    {

        EmitterInfo* emitter = GetPointerFromEmitterName(emitterName, m_emitters);
        DetailInfo* detailInfo = GetPointerFromEmitterName(emitterName, m_details);
        if (detailInfo == nullptr || emitter == nullptr)
        {
            return;
        }

        switch (index)
        {
            case DetailConstant::ASSET_MATERIAL:
                emitter->m_material = detailInfo->m_material;
                break;
            case DetailConstant::ASSET_MODEL:
                emitter->m_model = detailInfo->m_model;
                break;
            case DetailConstant::ASSET_SKELETON_MODEL:
                emitter->m_skeletonModel = detailInfo->m_skeletonModel;
                break;
        }
    }

    void ParticleSourceData::UpdateEmitterName(const AZStd::string& oldEmitterName, const AZStd::string& newEmitterName)
    {
        EmitterInfo* emitter = GetPointerFromEmitterName(oldEmitterName, m_emitters);
        DetailInfo* detailInfo = GetPointerFromEmitterName(oldEmitterName, m_details);
        if (detailInfo == nullptr || emitter == nullptr)
        {
            return;
        }
        emitter->m_name = newEmitterName;
        detailInfo->m_name = newEmitterName;
    }

    // Unselect Emitter in the Particle Editor
    // Note that when we do this, we actually call RemoveEmitter from the source data,
    // which leaves the data inside just the Details object.
    // We can then re-enable it later using SelectDetail above, without losing the data.
    void ParticleSourceData::UnselectDetail(AZStd::string emitterName)
    {
        EmitterInfo* emitterInfo = GetPointerFromEmitterName(emitterName, m_emitters);
        DetailInfo* detailInfo = GetPointerFromEmitterName(emitterName, m_details);
        if (emitterInfo == nullptr || detailInfo == nullptr)
        {
            return;
        }

        auto& moduleMap = detailInfo->m_modules;
        for (auto& pair : moduleMap)
        {
            ReserveRemovedModules(pair.first, &pair.second, &detailInfo->m_unusedListForDetail);
        }

        RemovedModulesSort(detailInfo, emitterInfo);
        for (auto& tuple : detailInfo->m_unusedListForDetail)
        {
            UnselectModule(
                detailInfo, AZStd::get<DetailInfo::CLASS_NAME>(tuple), AZStd::get<DetailInfo::MODULE_NAME>(tuple));
        }
        RemoveEmitter(emitterInfo);
        detailInfo->m_isUse = false;
        SetEventHandler(true, detailInfo->m_name);
    }

    // Add module to emitter
    void ParticleSourceData::SelectModule(DetailInfo* detailInfo, AZStd::string className, AZStd::string moduleName)
    {
        EmitterInfo* emitterInfo = GetPointerFromEmitterName(detailInfo->m_name, m_emitters);
        auto& listIndex = AZStd::get<DetailConstant::LIST_INDEX>(GetTypeIdItem(className, moduleName));
        if (emitterInfo == nullptr)
        {
            return;
        }

        auto& module = detailInfo->m_modules[className][moduleName];
        auto& any = *module.second;
        module.first = true;
        bool rebuild = PopDistribution(any, detailInfo, className, moduleName);

        switch (listIndex)
        {
        case DetailConstant::EMITTER_CONFIG:
            emitterInfo->m_config = any;
            module.second = &emitterInfo->m_config;
            break;
        case DetailConstant::RENDER_TYPE:
            emitterInfo->m_renderConfig = any;
            module.second = &emitterInfo->m_renderConfig;
            break;
        case DetailConstant::EMITTER_EMIT:
            emitterInfo->m_emitModules.emplace_back(any);
            module.second = &emitterInfo->m_emitModules.back();
            break;
        case DetailConstant::EMITTER_SPAWN:
            emitterInfo->m_spawnModules.emplace_back(any);
            module.second = &emitterInfo->m_spawnModules.back();
            SortSpawnModules(detailInfo, emitterInfo, className, moduleName);
            break;
        case DetailConstant::EMITTER_UPDATE:
            emitterInfo->m_updateModules.emplace_back(any);
            module.second = &emitterInfo->m_updateModules.back();
            SortUpdateModules(detailInfo, emitterInfo, moduleName);
            break;
        case DetailConstant::EMITTER_EVENT:
            emitterInfo->m_eventModules.emplace_back(any);
            module.second = &emitterInfo->m_eventModules.back();
            break;
        default:
            break;
        }
        RemoveUnusedModule(detailInfo, any);
        if (rebuild)
        {
            ToEditor();
        }
    }

    // Remove module from emitter
    void ParticleSourceData::UnselectModule(DetailInfo* detailInfo, AZStd::string className, AZStd::string moduleName)
    {
        EmitterInfo* emitterInfo = GetPointerFromEmitterName(detailInfo->m_name, m_emitters);
        auto& listIndex = AZStd::get<DetailConstant::LIST_INDEX>(GetTypeIdItem(className, moduleName));
        if (emitterInfo == nullptr)
        {
            return;
        }

        auto& module = detailInfo->m_modules[className][moduleName];
        auto& moduleList = detailInfo->m_unusedModules;

        switch (listIndex)
        {
        case DetailConstant::EMITTER_CONFIG:
            moduleList.emplace_back(emitterInfo->m_config);
            module.second = &moduleList.back();
            break;
        case DetailConstant::RENDER_TYPE:
            module.first = false;
            moduleList.emplace_back(emitterInfo->m_renderConfig);
            module.second = &moduleList.back();
            emitterInfo->m_renderConfig.clear();
            break;
        case DetailConstant::EMITTER_EMIT:
            RemoveDetailModule(detailInfo, className, moduleName, emitterInfo->m_emitModules);
            break;
        case DetailConstant::EMITTER_SPAWN:
            RemoveDetailModule(detailInfo, className, moduleName, emitterInfo->m_spawnModules);
            break;
        case DetailConstant::EMITTER_UPDATE:
            RemoveDetailModule(detailInfo, className, moduleName, emitterInfo->m_updateModules);
            break;
        case DetailConstant::EMITTER_EVENT:
            RemoveDetailModule(detailInfo, className, moduleName, emitterInfo->m_eventModules);
            break;
        default:
            break;
        }
    }

    void ParticleSourceData::UpdateDistributionIndexes()
    {
        UpdateRandomIndexes();
        UpdateCurveIndexes();
    }

    void ParticleSourceData::UpdateRandomIndexes()
    {
        // no duplicates
        AZStd::sort(m_distribution.stashedRandomIndexes.begin(), m_distribution.stashedRandomIndexes.end(), AZStd::greater<size_t>());

        for (auto& emitterInfo : m_emitters)
        {
            UpdateRandomIndexes(emitterInfo->m_emitModules);
            UpdateRandomIndexes(emitterInfo->m_spawnModules);
            UpdateRandomIndexes(emitterInfo->m_updateModules);
            UpdateRandomIndexes(emitterInfo->m_eventModules);
        }
    }

    void ParticleSourceData::UpdateCurveIndexes()
    {
        // no duplicates
        AZStd::sort(m_distribution.stashedCurveIndexes.begin(), m_distribution.stashedCurveIndexes.end(), AZStd::greater<size_t>());

        for (auto& emitterInfo : m_emitters)
        {
            UpdateCurveIndexes(emitterInfo->m_emitModules);
            UpdateCurveIndexes(emitterInfo->m_spawnModules);
            UpdateCurveIndexes(emitterInfo->m_updateModules);
            UpdateCurveIndexes(emitterInfo->m_eventModules);
        }
    }

    void ParticleSourceData::UpdateRandomIndexes(AZStd::list<AZStd::any>& list)
    {
        // refresh indexes of other item in list
        for (auto removedIndex : m_distribution.stashedRandomIndexes)
        {
            for (auto& any : list)
            {
                DistInfos distInfosToRefresh;
                DataConvertor::GetDistIndex(any, distInfosToRefresh);
                for (auto& info : distInfosToRefresh.randomIndexInfos)
                {
                    if (info.distIndex > removedIndex)
                    {
                        info.distIndex -= 1;
                    }
                }
                DataConvertor::UpdateDistIndex(any, distInfosToRefresh);
            }
        }
    }

    void ParticleSourceData::UpdateCurveIndexes(AZStd::list<AZStd::any>& list)
    {
        // refresh indexes of other item in list
        for (auto removedIndex : m_distribution.stashedCurveIndexes)
        {
            for (auto& any : list)
            {
                DistInfos distInfosToRefresh;
                DataConvertor::GetDistIndex(any, distInfosToRefresh);
                for (auto& info : distInfosToRefresh.curveIndexInfos)
                {
                    if (info.distIndex > removedIndex)
                    {
                        info.distIndex -= 1;
                    }
                }
                DataConvertor::UpdateDistIndex(any, distInfosToRefresh);
            }
        }
    }

    // Remove module by type (private)
    void ParticleSourceData::RemoveDetailModule(
        DetailInfo* detailInfo, AZStd::string& className, AZStd::string& moduleName, AZStd::list<AZStd::any>& list)
    {
        auto& module = detailInfo->m_modules[className][moduleName];
        auto& moduleList = detailInfo->m_unusedModules;
        auto it = AZStd::find_if(
            list.begin(), list.end(),
            [&module](const AZStd::any& any)
            {
                return &any == module.second;
            });
        if (it != list.end())
        {
            StashDistribution(*it, detailInfo, className, moduleName);

            DistInfos distInfos;
            DataConvertor::GetDistIndex(*it, distInfos);
            if (!distInfos.randomIndexInfos.empty())
            {
                for (auto& info : distInfos.randomIndexInfos)
                {
                    m_distribution.stashedRandomIndexes.emplace_back(info.distIndex);
                    info.distIndex = 0;
                }
            }
            if (!distInfos.curveIndexInfos.empty())
            {
                for (auto& info : distInfos.curveIndexInfos)
                {
                    m_distribution.stashedCurveIndexes.emplace_back(info.distIndex);
                    info.distIndex = 0;
                }
            }
            DataConvertor::UpdateDistIndex(*it, distInfos);

            module.first = false;
            moduleList.emplace_back(*it);
            module.second = &moduleList.back();
            list.erase(it);
        }

        UpdateDistributionIndexes();
    }

    void ParticleSourceData::ReserveRemovedModules(const AZStd::string& className, const ModuleType* moduleType, UnusedListType* unusedList)
    {
        auto& moduleNames = m_detailConstant.moduleClasses.at(className);
        for (auto& module : *moduleType)
        {
            if (!module.second.first)
            {
                continue;
            }
            auto find = AZStd::find_if(
                moduleNames.begin(), moduleNames.end(),
                [&module](const auto& t)
                {
                    return AZStd::get<DetailConstant::TYPE_ID>(t) == module.second.second->type();
                });
            if (find == moduleNames.end())
            {
                AZ_Error("OpenParticleSystem", false, "Any[%s] information lost\n", module.second.second->type());
                continue;
            }
            auto& listIndex = AZStd::get<DetailConstant::LIST_INDEX>(*find);
            unusedList->emplace_back(UnusedListNodeType(className, module.first, listIndex));
        }
    }

    void ParticleSourceData::SelectClass(AZStd::string emitterName, AZStd::string className)
    {
        DetailInfo* detailInfo = GetPointerFromEmitterName(emitterName, m_details);
        if (detailInfo == nullptr)
        {
            return;
        }
        bool isUnused = true;
        for (auto iter = detailInfo->m_unusedListForClass.begin(); iter != detailInfo->m_unusedListForClass.end();)
        {
            if (className == AZStd::get<DetailInfo::CLASS_NAME>(*iter))
            {
                SelectModule(detailInfo, className, AZStd::get<DetailInfo::MODULE_NAME>(*iter));
                iter = detailInfo->m_unusedListForClass.erase(iter);
                isUnused = false;
            }
            else
            {
                iter++;
            }
        }

        if (isUnused)
        {
            SelectModule(detailInfo, className, AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(className)[0]));
        }
    }

    void ParticleSourceData::UnselectClass(AZStd::string emitterName, AZStd::string className)
    {
        DetailInfo* detailInfo = GetPointerFromEmitterName(emitterName, m_details);
        if (detailInfo == nullptr)
        {
            return;
        }

        for (auto iter = detailInfo->m_unusedListForClass.begin(); iter != detailInfo->m_unusedListForClass.end();)
        {
            if (className == AZStd::get<DetailInfo::CLASS_NAME>(*iter))
            {
                iter = detailInfo->m_unusedListForClass.erase(iter);
            }
            else
            {
                iter++;
            }
        }

        ReserveRemovedModules(className, &detailInfo->m_modules[className], &detailInfo->m_unusedListForClass);

        for (auto& iter : detailInfo->m_modules[className])
        {
            if (iter.second.first)
            {
                UnselectModule(detailInfo, className, iter.first);
            }
        }
    }

    void ParticleSourceData::RemovedModulesSort(DetailInfo* detailInfo, EmitterInfo* emitterInfo)
    {
        auto listSort = [](const auto& first, const auto& second)
        {
            return AZStd::get<DetailInfo::LIST_INDEX>(first) < AZStd::get<DetailInfo::LIST_INDEX>(second);
        };
        AZStd::sort(detailInfo->m_unusedListForDetail.begin(), detailInfo->m_unusedListForDetail.end(), listSort);

        RemovedModulesModuleSort(detailInfo, emitterInfo);
    }

    void ParticleSourceData::AddEventHandler(DetailInfo* detail, AZ::u8 index)
    {
        auto className = m_detailConstant.classNames[DetailConstant::EVENT_INDEX];
        auto& [moduleName, typeId, listIndex] = m_detailConstant.moduleClasses.at(className)[index];
        AZStd::any module = m_serializeContext->CreateAny(typeId);
        auto name = moduleName;
        if (index == DetailConstant::EventIndex::EVENT_HANDLER)
        {
            name = name + AZStd::to_string(detail->m_itemNumber[typeId]++);
        }
        AZStd::any* pointer = nullptr;
        auto emitter = GetPointerFromEmitterName(detail->m_name, m_emitters);
        emitter->m_emitModules.emplace_back(module);
        pointer = &emitter->m_emitModules.back();
        detail->m_modules[className][name] = { true, pointer };
    }

    void ParticleSourceData::DeleteEventHandler(DetailInfo* detail, AZStd::string& moduleName)
    {
        auto className = m_detailConstant.classNames[DetailConstant::EVENT_INDEX];
        auto& moduleMap = detail->m_modules[className];
        auto& any = moduleMap[moduleName].second;
        auto listFun = [&any](auto& list)
        {
            auto find = AZStd::find_if(
                list.begin(), list.end(),
                [&any](const auto& iter)
                {
                    return &iter == any;
                });
            if (find != list.end())
            {
                list.erase(find);
            }
        };
        auto eraseFun = [](auto& list, const auto& value)
        {
            auto find = AZStd::find(list.begin(), list.end(), value);
            if (find != list.end())
            {
                list.erase(find);
            }
        };
        auto emitter = GetPointerFromEmitterName(detail->m_name, m_emitters);
        AZStd::invoke(listFun, emitter->m_emitModules);
        AZStd::invoke(eraseFun, moduleMap, ModuleType::value_type(moduleName, AZStd::pair<bool, AZStd::any*>(true, any)));
    }

    const ParticleSourceData::TypeId::value_type& ParticleSourceData::GetTypeIdItem(
        const AZStd::string& className, const AZStd::string& moduleName) const
    {
        auto& classList = m_detailConstant.moduleClasses.at(className);
        auto find = AZStd::find_if(
            classList.begin(), classList.end(),
            [&moduleName](const auto& it)
            {
                auto& name = AZStd::get<DetailConstant::MODULE_NAME>(it);
                return moduleName.compare(0, name.length(), name) == 0;
            });
        return *find;
    }

    void ParticleSourceData::RemoveUnusedModule(DetailInfo* detailInfo, AZStd::any& any) const
    {
        auto& modulesList = detailInfo->m_unusedModules;
        auto findIter = AZStd::find_if(
            modulesList.begin(), modulesList.end(),
            [&any](const AZStd::any& anyIt)
            {
                return &anyIt == &any;
            });
        if (findIter != modulesList.end())
        {
            modulesList.erase(findIter);
        }
    }

    void ParticleSourceData::RemovedModulesModuleSort(DetailInfo* detailInfo, EmitterInfo* emitterInfo) const
    {
        auto moduleSort = [&emitterInfo, &detailInfo](const auto& first, const auto& second) -> bool
        {
            if (AZStd::get<DetailInfo::LIST_INDEX>(first) != AZStd::get<DetailInfo::LIST_INDEX>(second))
            {
                return false;
            }
            auto& firstAnyPointer = detailInfo->m_modules[AZStd::get<DetailInfo::CLASS_NAME>(first)][AZStd::get<DetailInfo::MODULE_NAME>(first)].second;
            auto& secondAnyPointer = detailInfo->m_modules[AZStd::get<DetailInfo::CLASS_NAME>(second)][AZStd::get<DetailInfo::MODULE_NAME>(second)].second;
            auto firstFind = [&firstAnyPointer](const auto& iter)
            {
                return &iter == firstAnyPointer;
            };
            auto secondFind = [&secondAnyPointer](const auto& iter)
            {
                return &iter == secondAnyPointer;
            };
            bool rtn = false;
            auto findFun = [&rtn, &firstFind, &secondFind](const auto& list)
            {
                for (auto firstIter = AZStd::find_if(list.begin(), list.end(), firstFind),
                          secondIter = AZStd::find_if(list.begin(), list.end(), secondFind);
                     firstIter != list.end(); firstIter++)
                {
                    if (firstIter == secondIter)
                    {
                        rtn = true;
                        break;
                    }
                }
            };
            switch (AZStd::get<DetailInfo::LIST_INDEX>(first))
            {
            case DetailConstant::EMITTER_EMIT:
                AZStd::invoke(findFun, emitterInfo->m_emitModules);
                break;
            case DetailConstant::EMITTER_SPAWN:
                AZStd::invoke(findFun, emitterInfo->m_spawnModules);
                break;
            case DetailConstant::EMITTER_UPDATE:
                AZStd::invoke(findFun, emitterInfo->m_updateModules);
                break;
            case DetailConstant::EMITTER_EVENT:
                AZStd::invoke(findFun, emitterInfo->m_eventModules);
                break;
            default:
                break;
            }
            return rtn;
        };
        AZStd::sort(detailInfo->m_unusedListForDetail.begin(), detailInfo->m_unusedListForDetail.end(), moduleSort);
    }

    bool ParticleSourceData::CheckModuleState(const AZ::TypeId& id) const
    {
        bool used = true;
        if (m_currentDetailInfo)
        {
            for (const auto& iter : m_currentDetailInfo->m_unusedModules)
            {
                if (iter.get_type_info().m_id == id)
                {
                    used = false;
                    break;
                }
            }
        }
        return used;
    }

    void ParticleSourceData::SetDefaultModules(DetailInfo* detailInfo)
    {
        SelectModule(detailInfo, "Emitter", "Emitter");
        SelectModule(detailInfo, "Spawn", "Spawn Rate");
        SelectModule(detailInfo, "Particles", "Particle Lifetime");
        SelectModule(detailInfo, "Particles", "Start Velocity");
        SelectModule(detailInfo, "Shape", "Point");
        SelectModule(detailInfo, "Renderer", "Sprite Renderer");

        AZ::Data::AssetId defaultSpriteEmitMaterialId;
        using editorBus = EditorParticleSystemComponentRequestBus;
        using editorBusEvents = EditorParticleSystemComponentRequestBus::Events;
        editorBus::BroadcastResult(defaultSpriteEmitMaterialId, &editorBusEvents::GetDefaultEmitterMaterialId);

        if (defaultSpriteEmitMaterialId.IsValid())
        {
            detailInfo->m_material =
                AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(defaultSpriteEmitMaterialId, AZ::Data::AssetLoadBehaviorNamespace::NoLoad);
        }
    }

    void ParticleSourceData::StashDistribution(AZStd::any& any, const void* detailInfo, const AZStd::string& className, const AZStd::string& moduleName)
    {
        auto cacheInterface = AZ::Interface<DistributionCacheInterface>::Get();
        if (cacheInterface)
        {
            DistInfos distInfos;
            DataConvertor::GetDistIndex(any, distInfos);
            AZStd::vector<AZ::TypeId> paramIds;
            DataConvertor::GetParamId(any, paramIds);
            for (auto& paramId : paramIds)
            {
                auto key = cacheInterface->GetKey(detailInfo, className, moduleName, paramId);
                for (auto& randomInfo : distInfos.randomIndexInfos)
                {
                    if (randomInfo.distIndex && randomInfo.distIndex <= m_distribution.randoms.size() && randomInfo.paramName == paramId)
                    {
                        auto* random = m_distribution.randoms.at(randomInfo.distIndex - 1);
                        auto stashRandom = AZStd::make_pair(randomInfo, random);
                        cacheInterface->StashDistributionRandom(key, stashRandom);
                        m_distribution.randomCaches[key].emplace_back(random);
                    }
                }
                for (const auto& curveInfo : distInfos.curveIndexInfos)
                {
                    if (curveInfo.distIndex && curveInfo.distIndex <= m_distribution.curves.size() && curveInfo.paramName == paramId)
                    {
                        auto* curve = m_distribution.curves.at(curveInfo.distIndex - 1);
                        auto stashCurve = AZStd::make_pair(curveInfo, curve);
                        cacheInterface->StashDistributionCurve(key, stashCurve);
                        m_distribution.curveCaches[key].curves[curveInfo.paramIndex] = curve;
                    }
                }
            }
        }
    }

    bool ParticleSourceData::PopDistribution(
        AZStd::any& any, const void* detailInfo, const AZStd::string& className, const AZStd::string& moduleName, bool stash)
    {
        auto cacheInterface = AZ::Interface<DistributionCacheInterface>::Get();
        bool rebuild = false;
        if (cacheInterface)
        {
            DistInfos distInfos;
            AZStd::vector<AZ::TypeId> paramIds;
            DataConvertor::GetParamId(any, paramIds);
            for (auto& paramId : paramIds)
            {
                auto key = cacheInterface->GetKey(detailInfo, className, moduleName, paramId);
                
                AZStd::vector<RandomData> popRandom;
                rebuild |= cacheInterface->PopDistributionRandom(key, popRandom);
                if (!stash)
                {
                    m_distribution.randomCaches.erase(key);
                }
                for (auto& randomInfo : popRandom)
                {
                    AddRandom(randomInfo.second);
                    randomInfo.first.distIndex = static_cast<AZ::u32>(m_distribution.randoms.size());
                    distInfos.randomIndexInfos.emplace_back(randomInfo.first);
                    if (stash)
                    {
                        cacheInterface->StashDistributionRandom(key, randomInfo);
                    }
                }

                AZStd::vector<CurveData> popCurve;
                rebuild |= cacheInterface->PopDistributionCurve(key, popCurve);
                if (!stash)
                {
                    m_distribution.curveCaches.erase(key);
                }
                for (auto& curveInfo : popCurve)
                {
                    AddCurve(curveInfo.second);
                    curveInfo.first.distIndex = static_cast<AZ::u32>(m_distribution.curves.size());
                    distInfos.curveIndexInfos.emplace_back(curveInfo.first);
                    m_distribution.curveCaches[key].curves[curveInfo.first.paramIndex] = nullptr;
                    if (stash)
                    {
                        cacheInterface->StashDistributionCurve(key, curveInfo);
                    }
                }
            }
            DataConvertor::UpdateDistIndex(any, distInfos);
        }
        return rebuild;
    }

    void ParticleSourceData::SortSpawnModules(
        DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string className, const AZStd::string moduleName) const
    {
        SortVelocityModules(detailInfo, emitterInfo, className, moduleName);
        SortSpawnEventModule(detailInfo, emitterInfo, moduleName);
    }

    void ParticleSourceData::SortSpawnEventModule(DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string moduleName) const
    {
        auto& eventClassName = m_detailConstant.classNames[DetailConstant::EVENT_INDEX];
        auto& spawnEventName =
            AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(eventClassName)[DetailConstant::SPAWN_EVENT]);

        auto& spawnList = emitterInfo->m_spawnModules;
        auto& moduleType = detailInfo->m_modules[eventClassName];
        if (moduleType[spawnEventName].first && spawnEventName != moduleName)
        {
            ChangeModuleToEnd(spawnList, moduleType[spawnEventName].second);
        }
    }

    void ParticleSourceData::SortVelocityModules(
        DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string className, const AZStd::string moduleName) const
    {
        AZ::u8 velDirectionIndex = 4;
        auto& velDirectionName = AZStd::get<DetailConstant::MODULE_NAME>(
            m_detailConstant.moduleClasses.at(m_detailConstant.classNames[DetailConstant::PARTICLES_INDEX])[velDirectionIndex]);

        auto& velocityClassName = m_detailConstant.classNames[DetailConstant::VELOCITY_INDEX];
        auto& velConeName = AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(velocityClassName)[DetailConstant::VELOCITY_CONE]);
        auto& velSphereName = AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(velocityClassName)[DetailConstant::VELOCITY_SPHERE]);
        auto& velConcentrateName = AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(velocityClassName)[DetailConstant::VELOCITY_CONCENTRATE]);

        auto& shapeClassName = m_detailConstant.classNames[DetailConstant::SHAPE_INDEX];

        auto& spawnList = emitterInfo->m_spawnModules;
        auto& moduleType = detailInfo->m_modules[velocityClassName];
        if (moduleName == velDirectionName)
        {
            if (moduleType[velConeName].first)
            {
                ChangeModuleToEnd(spawnList, moduleType[velConeName].second);
            }
            if (moduleType[velSphereName].first)
            {
                ChangeModuleToEnd(spawnList, detailInfo->m_modules[velocityClassName][velSphereName].second);
            }
            if (moduleType[velConcentrateName].first)
            {
                ChangeModuleToEnd(spawnList, detailInfo->m_modules[velocityClassName][velConcentrateName].second);
            }
        }
        else if (moduleName == velConeName)
        {
            if (moduleType[velSphereName].first)
            {
                ChangeModuleToEnd(spawnList, moduleType[velSphereName].second);
            }
        }
        else if (className == shapeClassName)
        {
            if (moduleType[velConcentrateName].first)
            {
                ChangeModuleToEnd(spawnList, detailInfo->m_modules[velocityClassName][velConcentrateName].second);
            }
        }
    }

    void ParticleSourceData::SortUpdateModules(DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string moduleName) const
    {
        SortSizeModules(detailInfo, emitterInfo, moduleName);
    }

    void ParticleSourceData::SortSizeModules(DetailInfo* detailInfo, EmitterInfo* emitterInfo, const AZStd::string moduleName) const
    {
        auto& sizeName = m_detailConstant.classNames[DetailConstant::SIZE_INDEX];
        auto& sizeLinearName = AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(sizeName)[DetailConstant::SIZE_LINEAR]);
        auto& sizeVelocityName =
            AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(sizeName)[DetailConstant::SIZE_BY_VELOCITY]);
        auto& sizeScaleName =
            AZStd::get<DetailConstant::MODULE_NAME>(m_detailConstant.moduleClasses.at(sizeName)[DetailConstant::SIZE_SCALE]);

        auto& updateList = emitterInfo->m_updateModules;
        auto& moduleType = detailInfo->m_modules[sizeName];
        if (moduleName == sizeLinearName)
        {
            if (moduleType[sizeVelocityName].first)
            {
                ChangeModuleToEnd(updateList, moduleType[sizeVelocityName].second);
            }
            if (moduleType[sizeScaleName].first)
            {
                ChangeModuleToEnd(updateList, moduleType[sizeScaleName].second);
            }
        }
        else if (moduleName == sizeVelocityName)
        {
            if (moduleType[sizeScaleName].first)
            {
                ChangeModuleToEnd(updateList, moduleType[sizeScaleName].second);
            }
        }
    }

    void ParticleSourceData::ChangeModuleToEnd(AZStd::list<AZStd::any>& list, AZStd::any*& module) const
    {
        auto find = AZStd::find_if(
            list.begin(), list.end(),
            [&module](const auto& t)
            {
                return &t == module;
            });
        if (find != list.end())
        {
            list.emplace_back(*find);
            module = &list.back();
            list.erase(find);
        }
    }

    void ParticleSourceData::SetEventHandler(bool unSelect, AZStd::string emitterName)
    {
        for (auto& detailInfo : m_details)
        {
            if (!detailInfo->m_isUse)
            {
                continue;
            }
            for (const auto& iter : detailInfo->m_modules[m_detailConstant.classNames[DetailConstant::EVENT_INDEX]])
            {
                if (!iter.second.first || !iter.second.second->is<OpenParticle::ParticleEventHandler>())
                {
                    continue;
                }
                auto& any = *iter.second.second;
                auto eventHandler = AZStd::any_cast<OpenParticle::ParticleEventHandler>(any);
                auto find = AZStd::find_if(
                    m_emitters.begin(), m_emitters.end(),
                    [&eventHandler](const auto& t)
                    {
                        return t->m_name == eventHandler.emitterName;
                    });
                if (find != m_emitters.end())
                {
                    auto index = static_cast<AZ::u32>(find - m_emitters.begin());
                    if (index < m_emitters.size() && index != eventHandler.emitterIndex)
                    {
                        eventHandler.emitterIndex = index;
                        any = eventHandler;
                    }
                }
                else if (unSelect && eventHandler.emitterName == emitterName)
                {
                    eventHandler.emitterIndex = static_cast<AZ::u32>(-1);
                    any = eventHandler;
                }
            }
        }
    }
    void ParticleSourceData::UpdateDetailSoloState(AZStd::string& name, bool solo)
    {
        DetailInfo* detailInfo = GetPointerFromEmitterName(name, m_details);
        if (detailInfo == nullptr)
        {
            return;
        }
        detailInfo->m_solo = solo;
    }
    bool ParticleSourceData::SoloChecked(AZStd::string& name)
    {
        for (const auto& detail : m_details)
        {
            if (detail->m_solo)
            {
                name = detail->m_name;
                return true;
            }
        }
        return false;
    }

    AZStd::string ParticleSourceData::GetModuleKey(const AZ::TypeId& moduleId, const AZ::TypeId& paramId) const
    {
        return AZStd::string::format("%llx-%zx-%zx", reinterpret_cast<AZ::u64>(m_currentDetailInfo), moduleId.GetHash(), paramId.GetHash());
    }

    ParticleSourceData::DetailInfo* ParticleSourceData::CopyDetail(AZStd::string& sourceName, const AZStd::string& destName)
    {
        auto sourceDetail = GetPointerFromEmitterName(sourceName, m_details);
        auto sourceEmitter = GetPointerFromEmitterName(sourceName, m_emitters);
        if (sourceDetail == nullptr && sourceEmitter == nullptr)
        {
            return nullptr;
        }

        auto destDetail = aznew DetailInfo();
        auto destEmitter = aznew ParticleSourceData::EmitterInfo();
        destEmitter->m_name = destName;

        if (sourceDetail != nullptr && sourceEmitter != nullptr)
        {
            CopyDetailFromEmitter(sourceEmitter, destEmitter, destDetail);
        }
        else
        {
            CopyDetailFromDetail(sourceDetail, destEmitter, destDetail);
        }

        AZStd::vector<AZStd::any*> modulesWithDist;
        DistInfos distInfos;
        auto fn = [&distInfos, &modulesWithDist](auto& modules)
        {
            for (auto& module : modules)
            {
                DataConvertor::GetDistIndex(module, distInfos);
                if (!distInfos.randomIndexInfos.empty() || !distInfos.curveIndexInfos.empty())
                {
                    modulesWithDist.emplace_back(&module);
                    distInfos.Clear();
                }
            }
        };
        AZStd::invoke(fn, destEmitter->m_spawnModules);
        AZStd::invoke(fn, destEmitter->m_emitModules);
        AZStd::invoke(fn, destEmitter->m_updateModules);
        AZStd::invoke(fn, destEmitter->m_eventModules);
        for (auto& module : modulesWithDist)
        {
            StashDistribution(*module, destDetail, "className", "moduleName");
        }

        m_destEmitter = destEmitter;
        m_destDetail = destDetail;
        return m_destDetail;
    }

    ParticleSourceData::EmitterInfo* ParticleSourceData::CopyEmitter()
    {
        return m_destEmitter;
    }

    ParticleSourceData::DetailInfo* ParticleSourceData::SetDestItem(EmitterInfo* sourceEmitter, DetailInfo* sourceDetail, AZStd::string destName)
    {
        AZStd::vector<AZStd::any*> modulesWithDist;
        DistInfos distInfos;
        auto fn = [&distInfos, &modulesWithDist](auto& modules)
        {
            for (auto& module : modules)
            {
                DataConvertor::GetDistIndex(module, distInfos);
                if (!distInfos.randomIndexInfos.empty() || !distInfos.curveIndexInfos.empty())
                {
                    modulesWithDist.emplace_back(&module);
                    distInfos.Clear();
                }
            }
        };
        AZStd::invoke(fn, sourceEmitter->m_spawnModules);
        AZStd::invoke(fn, sourceEmitter->m_emitModules);
        AZStd::invoke(fn, sourceEmitter->m_updateModules);
        AZStd::invoke(fn, sourceEmitter->m_eventModules);
        for (auto& module : modulesWithDist)
        {
            auto cacheInterface = AZ::Interface<DistributionCacheInterface>::Get();
            if (cacheInterface)
            {
                AZStd::vector<RandomData> popRandom;
                AZStd::vector<CurveData> popCurve;
                AZStd::vector<AZ::TypeId> paramIds;
                DataConvertor::GetParamId(*module, paramIds);
                for (auto& paramId : paramIds)
                {
                    auto key = cacheInterface->GetKey(sourceDetail, "className", "moduleName", paramId);
                    cacheInterface->PopDistributionRandom(key, popRandom);
                    for (auto& randomInfo : popRandom)
                    {
                        cacheInterface->StashDistributionRandom(key, randomInfo);

                        AddRandom(*randomInfo.second);
                        randomInfo.first.distIndex = static_cast<AZ::u32>(m_distribution.randoms.size());
                        distInfos.randomIndexInfos.emplace_back(randomInfo.first);
                    }

                    cacheInterface->PopDistributionCurve(key, popCurve);
                    for (auto& curveInfo : popCurve)
                    {
                        cacheInterface->StashDistributionCurve(key, curveInfo);

                        AddCurve(*curveInfo.second);
                        curveInfo.first.distIndex = static_cast<AZ::u32>(m_distribution.curves.size());
                        distInfos.curveIndexInfos.emplace_back(curveInfo.first);
                    }
                }
                DataConvertor::UpdateDistIndex(*module, distInfos);
            }
        }

        if (sourceDetail == nullptr && sourceEmitter == nullptr)
        {
            return nullptr;
        }

        auto destDetail = aznew DetailInfo();
        auto destEmitter = AddEmitter(destName);
        m_details.emplace_back(destDetail);

        if (sourceDetail != nullptr && sourceEmitter != nullptr)
        {
            CopyDetailFromEmitter(sourceEmitter, destEmitter, destDetail);
        }
        else
        {
            CopyDetailFromDetail(sourceDetail, destEmitter, destDetail);
        }

        return destDetail;
    }

    void ParticleSourceData::CopyDetailFromEmitter(EmitterInfo* sourceEmitter, EmitterInfo* destEmitter, DetailInfo* destDetail)
    {
        // copy modules
        destEmitter->m_config = sourceEmitter->m_config;
        destEmitter->m_renderConfig = sourceEmitter->m_renderConfig;
        destEmitter->m_material = sourceEmitter->m_material;
        destEmitter->m_model = sourceEmitter->m_model;
        destEmitter->m_skeletonModel = sourceEmitter->m_skeletonModel;
        destEmitter->m_emitModules.assign(sourceEmitter->m_emitModules.begin(), sourceEmitter->m_emitModules.end());
        destEmitter->m_spawnModules.assign(sourceEmitter->m_spawnModules.begin(), sourceEmitter->m_spawnModules.end());
        destEmitter->m_updateModules.assign(sourceEmitter->m_updateModules.begin(), sourceEmitter->m_updateModules.end());
        destEmitter->m_eventModules.assign(sourceEmitter->m_eventModules.begin(), sourceEmitter->m_eventModules.end());

        EmitterInfoToDetailInfo(destDetail, destEmitter);
        // copy distributions
        AZStd::vector<AZStd::any*> modulesWithDist;
        DistInfos distInfos;
        auto fn = [&distInfos, &modulesWithDist](auto& modules)
        {
            for (auto& module : modules)
            {
                DataConvertor::GetDistIndex(module, distInfos);
                if (!distInfos.randomIndexInfos.empty() || !distInfos.curveIndexInfos.empty())
                {
                    modulesWithDist.emplace_back(&module);
                    distInfos.Clear();
                }
            }
        };
        AZStd::invoke(fn, destEmitter->m_spawnModules);
        AZStd::invoke(fn, destEmitter->m_emitModules);
        AZStd::invoke(fn, destEmitter->m_updateModules);
        AZStd::invoke(fn, destEmitter->m_eventModules);

        CopyDistributions(modulesWithDist);
    }
    void ParticleSourceData::CopyDetailFromDetail(DetailInfo* sourceDetail, EmitterInfo* destEmitter, DetailInfo* destDetail)
    {
        destEmitter->m_material = sourceDetail->m_material;
        destEmitter->m_model = sourceDetail->m_model;
        destEmitter->m_skeletonModel = sourceDetail->m_skeletonModel;
        // copy modules
        for (auto& tuple : sourceDetail->m_unusedListForDetail)
        {
            auto& className = AZStd::get<DetailInfo::CLASS_NAME>(tuple);
            auto& moduleName = AZStd::get<DetailInfo::MODULE_NAME>(tuple);

            auto& module = sourceDetail->m_modules[className][moduleName];
            auto any = *module.second;
            auto& listIndex = AZStd::get<DetailConstant::LIST_INDEX>(GetTypeIdItem(className, moduleName));
            bool rebuild = PopDistribution(any, sourceDetail, className, moduleName, true);
            switch (listIndex)
            {
            case DetailConstant::EMITTER_CONFIG:
                destEmitter->m_config = any;
                break;
            case DetailConstant::RENDER_TYPE:
                destEmitter->m_renderConfig = any;
                break;
            case DetailConstant::EMITTER_EMIT:
                destEmitter->m_emitModules.emplace_back(any);
                break;
            case DetailConstant::EMITTER_SPAWN:
                destEmitter->m_spawnModules.emplace_back(any);
                break;
            case DetailConstant::EMITTER_UPDATE:
                destEmitter->m_updateModules.emplace_back(any);
                break;
            case DetailConstant::EMITTER_EVENT:
                destEmitter->m_eventModules.emplace_back(any);
                break;
            default:
                break;
            }
            if (rebuild)
            {
                ToEditor();
            }
        }
        EmitterInfoToDetailInfo(destDetail, destEmitter);
    }

    void ParticleSourceData::CopyDistributions(AZStd::vector<AZStd::any*>& modules)
    {
        for (auto module : modules)
        {
            DistInfos distInfos;
            DataConvertor::GetDistIndex(*module, distInfos);
            // copy randoms
            for (auto& randomInfo : distInfos.randomIndexInfos)
            {
                if (randomInfo.distIndex <= m_distribution.randoms.size())
                {
                    auto* oldRandom = m_distribution.randoms[randomInfo.distIndex - 1];
                    AddRandom(oldRandom);
                    randomInfo.distIndex = static_cast<AZ::u32>(m_distribution.randoms.size());
                }
            }
            // copy curves
            for (auto& curveInfo : distInfos.curveIndexInfos)
            {
                if (curveInfo.distIndex <= m_distribution.curves.size())
                {
                    auto* oldCurve = m_distribution.curves[curveInfo.distIndex - 1];
                    AddCurve(oldCurve);
                    curveInfo.distIndex = static_cast<AZ::u32>(m_distribution.curves.size());
                }
            }
            DataConvertor::UpdateDistIndex(*module, distInfos);
        }
    }

    void ParticleSourceData::ClearCopyCache()
    {
        AZStd::string name = "_className_moduleName";
        for (const auto& randomCache : m_distribution.randomCaches)
        {
            if (randomCache.first.contains(name))
            {
                for (auto& random : randomCache.second)
                {
                    delete random;
                }
            }
        }
        for (const auto& curveCache : m_distribution.curveCaches)
        {
            if (curveCache.first.contains(name))
            {
                for (auto& curve : curveCache.second.curves)
                {
                    if (curve != nullptr)
                    {
                        delete curve;
                    }
                }
            }
        }
        m_distribution.ClearCaches();
        if (m_destEmitter != nullptr)
        {
            delete m_destEmitter;
            m_destEmitter = nullptr;
        }
        if (m_destDetail != nullptr)
        {
            delete m_destDetail;
            m_destDetail = nullptr;
        }
    }
} // namespace OpenParticle
