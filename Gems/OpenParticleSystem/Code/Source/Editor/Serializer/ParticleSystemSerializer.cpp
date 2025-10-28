/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleSystemSerializer.h"

#include <OpenParticleSystem/ParticleEditDataConfig.h>
#include <API/EditorAssetSystemAPI.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <Editor/EditorParticleSystemComponentRequestBus.h>

namespace OpenParticle
{
    namespace JSR = AZ::JsonSerializationResult;

    AZ_CLASS_ALLOCATOR_IMPL(ParticleEmitterInfoSerializer, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(ParticleLODSerializer, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(ParticleDistributionSerializer, AZ::SystemAllocator, 0);

    AZStd::string_view ParticleBaseSerializer::GetClassName(AZ::TypeId id, AZ::JsonBaseContext& context)
    {
        auto serializeContext = context.GetSerializeContext();
        auto classData = serializeContext->FindClassData(id);
        return classData == nullptr ? AZStd::string_view{} : AZStd::string_view(classData->m_name);
    }

    AZ::JsonSerializationResult::ResultCode ParticleBaseSerializer::LoadModule(
        AZStd::unordered_map<AZ::TypeId, VersionConvertor>& list,
        AZStd::list<AZStd::any>& modules,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        int burstListCount = 0;
        for (auto iter = inputValue.MemberBegin(); iter != inputValue.MemberEnd(); ++iter)
        {
            for (auto& id : list)
            {
                auto className = GetClassName(id.first, context);
                if (iter->name.GetString() != className)
                {
                    continue;
                }
                if (id.first == azrtti_typeid<OpenParticle::EmitBurstList>())
                {
                    if (burstListCount >= 1)
                    {
                        continue;
                    }
                    else
                    {
                        burstListCount++;
                    }
                }

                AZStd::any module = context.GetSerializeContext()->CreateAny(id.first);
                if (id.second != nullptr)
                {
                    id.second(this, module, iter->value, context);
                }
                resultCode.Combine(ContinueLoading(AZStd::any_cast<void>(&module), id.first, iter->value, context));
                modules.emplace_back(module);
            }
        }
        return resultCode;
    }

    AZ::JsonSerializationResult::ResultCode ParticleBaseSerializer::LoadModule(
        AZStd::unordered_map<AZ::TypeId, VersionConvertor>& list,
        AZStd::any& module,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        for (auto& id : list)
        {
            auto iterator = inputValue.FindMember(rapidjson::StringRef(GetClassName(id.first, context).data()));
            if (iterator == inputValue.MemberEnd())
            {
                continue;
            }
            module = context.GetSerializeContext()->CreateAny(id.first);
            if (id.second != nullptr)
            {
                id.second(this, module, iterator->value, context);
            }
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                AZStd::any_cast<void>(&module), id.first, inputValue, rapidjson::StringRef(GetClassName(id.first, context).data()),
                context));
        }
        return resultCode;
    }

    AZ::JsonSerializationResult::ResultCode ParticleBaseSerializer::LoadModule(
        AZ::TypeId id,
        AZStd::any& module,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        auto iterator = inputValue.FindMember(rapidjson::StringRef(GetClassName(id, context).data()));
        if (iterator == inputValue.MemberEnd())
        {
            return resultCode;
        }
        module = context.GetSerializeContext()->CreateAny(id);
        resultCode.Combine(ContinueLoadingFromJsonObjectField(
            AZStd::any_cast<void>(&module), id, inputValue, rapidjson::StringRef(GetClassName(id, context).data()), context));
        return resultCode;
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<EmitSpawn>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& emitSpawn = AZStd::any_cast<OpenParticle::EmitSpawn&>(module);
        auto resultCode = ConvertToValueObject(emitSpawn.spawnRateObject, "spawnRateObject", "spawnRate", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            emitSpawn.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            emitSpawn.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<EmitSpawnOverMoving>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& emitSpawnOverMoving = AZStd::any_cast<OpenParticle::EmitSpawnOverMoving&>(module);
        auto resultCode = ConvertToValueObject(emitSpawnOverMoving.spawnRatePerUnitObject, "spawnRatePerUnitObject", "spawnRatePerUnit", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            emitSpawnOverMoving.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            emitSpawnOverMoving.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnColor>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& spawnColor = AZStd::any_cast<OpenParticle::SpawnColor&>(module);
        auto resultCode = ConvertToValueObject(spawnColor.startColorObject, "startColorObject", "startColor", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            spawnColor.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            spawnColor.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnLifetime>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& spawnLifetime = AZStd::any_cast<OpenParticle::SpawnLifetime&>(module);
        auto resultCode = ConvertToValueObject(spawnLifetime.lifeTimeObject, "lifeTimeObject", "lifeTime", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            spawnLifetime.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            spawnLifetime.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnLocPoint>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& spawnLocPoint = AZStd::any_cast<OpenParticle::SpawnLocPoint&>(module);
        auto resultCode = ConvertToValueObject(spawnLocPoint.positionObject, "positionObject", "position", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            spawnLocPoint.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            spawnLocPoint.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnRotation>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& val = AZStd::any_cast<OpenParticle::SpawnRotation&>(module);
        auto resultCode = ConvertTo2FloatObject(val.initAngleObject, val.rotateSpeedObject,
            "initAngleObject", "initAngle", "rotateSpeedObject", "rotateSpeed", inputValue,
            context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            val.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            val.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnSize>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& spawnSize = AZStd::any_cast<OpenParticle::SpawnSize&>(module);
        auto resultCode = ConvertToValueObject(spawnSize.sizeObject, "sizeObject", "size", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            spawnSize.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            spawnSize.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnVelDirection>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& spawnVelDirection = AZStd::any_cast<OpenParticle::SpawnVelDirection&>(module);
        auto resultCode = ConvertToValueObject(spawnVelDirection.directionObject, "directionObject", "velocity", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            spawnVelDirection.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            spawnVelDirection.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnVelSector>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& val = AZStd::any_cast<OpenParticle::SpawnVelSector&>(module);
        auto resultCode = ConvertToValueObject(val.strengthObject, "strengthObject", "strength", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            val.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            val.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnVelCone>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& val = AZStd::any_cast<OpenParticle::SpawnVelCone&>(module);
        auto resultCode = ConvertToValueObject(val.strengthObject, "strengthObject", "strength", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            val.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            val.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnVelSphere>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& spawnVelSphere = AZStd::any_cast<OpenParticle::SpawnVelSphere&>(module);
        auto resultCode = ConvertToValueObject(spawnVelSphere.strengthObject, "strengthObject", "strength", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            spawnVelSphere.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            spawnVelSphere.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnVelConcentrate>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& spawnVelConcentrate = AZStd::any_cast<OpenParticle::SpawnVelConcentrate&>(module);
        auto resultCode = ConvertToValueObject(spawnVelConcentrate.rateObject, "rateObject", "rate", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            spawnVelConcentrate.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            spawnVelConcentrate.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SpawnLightEffect>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        auto& val = AZStd::any_cast<OpenParticle::SpawnLightEffect&>(module);
        auto iterator = inputValue.FindMember("lightColor");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &val.lightColorObject.dataValue, azrtti_typeid<AZ::Color>(), inputValue, "lightColor", context));
        }
        iterator = inputValue.FindMember("intensity");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &val.intensityObject.dataValue, azrtti_typeid<float>(), inputValue, "intensity", context));
        }
        iterator = inputValue.FindMember("radianScale");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &val.radianScaleObject.dataValue, azrtti_typeid<float>(), inputValue, "radianScale", context));
        }
        iterator = inputValue.FindMember("distIndex");
        if (iterator != inputValue.MemberEnd())
        {
            AZStd::array<size_t, 6> distIndex; // old index count : 6
            resultCode.Combine(ContinueLoadingFromJsonObjectField(&distIndex,
                azrtti_typeid<AZStd::array<size_t, 6>>(), inputValue, "distIndex", context));
            AZStd::copy(distIndex.begin(), distIndex.begin() + 4, val.lightColorObject.distIndex.begin());
            val.intensityObject.distIndex[0] = distIndex[4];
            val.radianScaleObject.distIndex[0] = distIndex[5];
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<UpdateColor>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& updateColor = AZStd::any_cast<OpenParticle::UpdateColor&>(module);
        auto resultCode = ConvertToValueObject(updateColor.currentColorObject, "currentColorObject", "currentColor", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            updateColor.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            updateColor.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<UpdateConstForce>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& updateConstForce = AZStd::any_cast<OpenParticle::UpdateConstForce&>(module);
        auto resultCode = ConvertToValueObject(updateConstForce.forceObject, "forceObject", "force", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            updateConstForce.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            updateConstForce.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<UpdateDragForce>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& updateDragForce = AZStd::any_cast<OpenParticle::UpdateDragForce&>(module);
        auto resultCode = ConvertToValueObject(updateDragForce.dragCoefficientObject, "dragCoefficientObject", "dragCoefficient", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            updateDragForce.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            updateDragForce.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<UpdateSizeLinear>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& updateSizeLinear = AZStd::any_cast<OpenParticle::UpdateSizeLinear&>(module);
        auto resultCode = ConvertToValueObject(updateSizeLinear.sizeObject, "sizeObject", "scale", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            updateSizeLinear.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            updateSizeLinear.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<UpdateSizeByVelocity>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        auto& val = AZStd::any_cast<OpenParticle::UpdateSizeByVelocity&>(module);
        auto iterator = inputValue.FindMember("velScaleObject");
        if (iterator != inputValue.MemberEnd())
        {
            val.version = 1;
            return;
        }
        iterator = inputValue.FindMember("minVelScale");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &val.velScaleObject.dataValue.minValue, azrtti_typeid<AZ::Vector3>(), inputValue, "minVelScale", context));
        }
        else
        {
            resultCode.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Missing));
        }
        iterator = inputValue.FindMember("maxVelScale");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &val.velScaleObject.dataValue.maxValue, azrtti_typeid<AZ::Vector3>(), inputValue, "maxVelScale", context));
        }
        else
        {
            resultCode.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Missing));
        }
        iterator = inputValue.FindMember("distIndex");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &val.velScaleObject.distIndex, azrtti_typeid<AZStd::array<size_t, 3>>(), inputValue, "distIndex", context));
        }
        else
        {
            resultCode.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Missing));
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            val.version = 0;
        }
        else
        {
            context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing param name");
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<RibbonConfig>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& ribbonCfg = AZStd::any_cast<OpenParticle::RibbonConfig&>(module);
        auto resultCode = ConvertToValueObject(ribbonCfg.ribbonWidthObject, "ribbonWidthObject", "ribbonWidth", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            ribbonCfg.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            ribbonCfg.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<SizeScale>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& sizeScale = AZStd::any_cast<OpenParticle::SizeScale&>(module);
        auto resultCode = ConvertToValueObject(sizeScale.scaleFactorObject, "scaleFactorObject", "scaleFactor", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            sizeScale.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            sizeScale.version = 0;
        }
    }

    template<>
    void ParticleBaseSerializer::ConvertOldModule<UpdateVelocity>(
        AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        auto& updateVelocity = AZStd::any_cast<OpenParticle::UpdateVelocity&>(module);
        auto resultCode = ConvertToValueObject(updateVelocity.directionObject, "directionObject", "velocity", inputValue, context);
        if (resultCode.GetOutcome() == JSR::Outcomes::Skipped)
        {
            updateVelocity.version = 1;
        }
        if (resultCode.GetOutcome() == JSR::Outcomes::Success)
        {
            updateVelocity.version = 0;
        }
    }

    // convert an old field, which is the file name of a source or product file (like "Blah.Material" or "mesh.azmodel")
    // into the new format, an actual asset, ie, Asset<MaterialAsset> refcounted smart pointer.
    // If its already an object, we can just load it normally.
    template<class T>
    JSR::ResultCode ParticleBaseSerializer::ConvertSourceFileNameToAsset(
        AZ::Data::Asset<T>& destField,
        const rapidjson::Value& inputValue,
        const char* fieldName,
        AZ::Data::AssetLoadBehavior loadBehavior,
        AZ::JsonDeserializerContext& context)
    {
        AZ::Data::AssetType typeId = azrtti_typeid<T>();
        destField.Reset(); // make sure any old ones are cleared out.
        // Convert old string names to Assets.
        auto iterator = inputValue.FindMember(fieldName);
        if (iterator == inputValue.MemberEnd())
        {
            return JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Skipped);
        }

        if (iterator->value.IsObject())
        {
            // if its an object, then its in the new format, an actual AZ::Asset<T> type, so just load it normally:
            auto returnCode = ContinueLoadingFromJsonObjectField(
                &destField, azrtti_typeid<AZ::Data::Asset<T>>(), inputValue, rapidjson::GenericStringRef<char>(fieldName), context);
            return returnCode;
        }

        if (iterator->value.IsString())
        {
            AZStd::string sourceAssetPath = iterator->value.GetString();
            if (sourceAssetPath.empty())
            {
                return JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Skipped);
            }

            AZ::Data::AssetId resultId = AzToolsFramework::AssetSystem::FindAssetIdFromFileName(sourceAssetPath.c_str(), typeId);
            if (resultId.IsValid())
            {
                destField = AZ::Data::AssetManager::Instance().FindOrCreateAsset<T>(resultId, loadBehavior);
            }
        }

        if (destField.GetId().IsValid())
        {
            return JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success);
        }

        return JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed);
    }

    AZ::JsonSerializationResult::Result ParticleEmitterInfoSerializer::Load(
        void* outputValue,
        const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        (void)outputValueTypeId;
        (void)outputValue;
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        ParticleSourceData::EmitterInfo* emitInfo = reinterpret_cast<ParticleSourceData::EmitterInfo*>(outputValue);

        resultCode.Combine(
            ContinueLoadingFromJsonObjectField(&emitInfo->m_name, azrtti_typeid<AZStd::string>(), inputValue, "name", context));

        AZ::TypeId configId(azrtti_typeid<OpenParticle::EmitterConfig>());
        emitInfo->m_config = context.GetSerializeContext()->CreateAny(configId);
        resultCode.Combine(
            ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(&emitInfo->m_config), configId, inputValue, "config", context));

        AZ::Data::AssetLoadBehavior loadBehavior = AZ::Data::AssetLoadBehavior::PreLoad;
        resultCode.Combine(ConvertSourceFileNameToAsset(emitInfo->m_material, inputValue, "material", loadBehavior, context));
        resultCode.Combine(ConvertSourceFileNameToAsset(emitInfo->m_model, inputValue, "model", loadBehavior, context));
        resultCode.Combine(ConvertSourceFileNameToAsset(emitInfo->m_skeletonModel, inputValue, "skeleton model", loadBehavior, context));

        AZStd::unordered_map<AZ::TypeId, VersionConvertor> emits = {
            { azrtti_typeid<OpenParticle::EmitBurstList>(),        nullptr },
            { azrtti_typeid<OpenParticle::EmitSpawn>(),            &ParticleBaseSerializer::ConvertOldModule<EmitSpawn> },
            { azrtti_typeid<OpenParticle::EmitSpawnOverMoving>(),  &ParticleBaseSerializer::ConvertOldModule<EmitSpawnOverMoving> },
            { azrtti_typeid<OpenParticle::ParticleEventHandler>(), nullptr },
            { azrtti_typeid<OpenParticle::InheritanceHandler>(),   nullptr }
        };

        AZStd::unordered_map<AZ::TypeId, VersionConvertor> spawns = {
            { azrtti_typeid<OpenParticle::SpawnColor>(),          &ParticleBaseSerializer::ConvertOldModule<SpawnColor> },
            { azrtti_typeid<OpenParticle::SpawnLifetime>(),       &ParticleBaseSerializer::ConvertOldModule<SpawnLifetime> },
            { azrtti_typeid<OpenParticle::SpawnLocBox>(),         nullptr },
            { azrtti_typeid<OpenParticle::SpawnLocPoint>(),       &ParticleBaseSerializer::ConvertOldModule<SpawnLocPoint> },
            { azrtti_typeid<OpenParticle::SpawnLocSphere>(),      nullptr },
            { azrtti_typeid<OpenParticle::SpawnLocCylinder>(),    nullptr },
            { azrtti_typeid<OpenParticle::SpawnLocSkeleton>(),    nullptr },
            { azrtti_typeid<OpenParticle::SpawnLocTorus>(),       nullptr },
            { azrtti_typeid<OpenParticle::SpawnSize>(),           &ParticleBaseSerializer::ConvertOldModule<SpawnSize> },
            { azrtti_typeid<OpenParticle::SpawnVelDirection>(),   &ParticleBaseSerializer::ConvertOldModule<SpawnVelDirection> },
            { azrtti_typeid<OpenParticle::SpawnVelSector>(),      &ParticleBaseSerializer::ConvertOldModule<SpawnVelSector> },
            { azrtti_typeid<OpenParticle::SpawnVelSphere>(),      &ParticleBaseSerializer::ConvertOldModule<SpawnVelSphere> },
            { azrtti_typeid<OpenParticle::SpawnVelConcentrate>(), &ParticleBaseSerializer::ConvertOldModule<SpawnVelConcentrate> },
            { azrtti_typeid<OpenParticle::SpawnVelCone>(),        &ParticleBaseSerializer::ConvertOldModule<SpawnVelCone> },
            { azrtti_typeid<OpenParticle::SpawnRotation>(),       &ParticleBaseSerializer::ConvertOldModule<SpawnRotation> },
            { azrtti_typeid<OpenParticle::SpawnLightEffect>(),    &ParticleBaseSerializer::ConvertOldModule<SpawnLightEffect> },
            { azrtti_typeid<OpenParticle::SpawnLocationEvent>(),  nullptr },
        };
        
        AZStd::unordered_map<AZ::TypeId, VersionConvertor> updates = {
            { azrtti_typeid<OpenParticle::UpdateConstForce>(),        &ParticleBaseSerializer::ConvertOldModule<UpdateConstForce> },
            { azrtti_typeid<OpenParticle::UpdateDragForce>(),         &ParticleBaseSerializer::ConvertOldModule<UpdateDragForce> },
            { azrtti_typeid<OpenParticle::UpdateVortexForce>(),       nullptr },
            { azrtti_typeid<OpenParticle::UpdateCurlNoiseForce>(),    nullptr },
            { azrtti_typeid<OpenParticle::UpdateColor>(),             &ParticleBaseSerializer::ConvertOldModule<UpdateColor> },
            { azrtti_typeid<OpenParticle::UpdateSizeLinear>(),        &ParticleBaseSerializer::ConvertOldModule<UpdateSizeLinear> },
            { azrtti_typeid<OpenParticle::UpdateSizeByVelocity>(),    &ParticleBaseSerializer::ConvertOldModule<UpdateSizeByVelocity> },
            { azrtti_typeid<OpenParticle::SizeScale>(),               &ParticleBaseSerializer::ConvertOldModule<SizeScale> },
            { azrtti_typeid<OpenParticle::UpdateSubUv>(),             nullptr },
            { azrtti_typeid<OpenParticle::UpdateRotateAroundPoint>(), nullptr },
            { azrtti_typeid<OpenParticle::UpdateVelocity>(),          &ParticleBaseSerializer::ConvertOldModule<UpdateVelocity> },
            { azrtti_typeid<OpenParticle::ParticleCollision>(),       nullptr },
        };

        AZStd::unordered_map<AZ::TypeId, VersionConvertor> events = {
            { azrtti_typeid<OpenParticle::UpdateLocationEvent>(),     nullptr },
            { azrtti_typeid<OpenParticle::UpdateDeathEvent>(),        nullptr },
            { azrtti_typeid<OpenParticle::UpdateCollisionEvent>(),    nullptr },
            { azrtti_typeid<OpenParticle::UpdateInheritanceEvent>(),  nullptr },
        };

        AZStd::unordered_map<AZ::TypeId, VersionConvertor> renderCfgs = {
            { azrtti_typeid<OpenParticle::SpriteConfig>(), nullptr },
            { azrtti_typeid<OpenParticle::MeshConfig>(),   nullptr },
            { azrtti_typeid<OpenParticle::RibbonConfig>(), &ParticleBaseSerializer::ConvertOldModule<RibbonConfig> },
        };
        resultCode.Combine(
            OpenParticle::ParticleEmitterInfoSerializer::LoadModule(renderCfgs, emitInfo->m_renderConfig, inputValue, context));

        resultCode.Combine(OpenParticle::ParticleEmitterInfoSerializer::LoadModule(emits, emitInfo->m_emitModules, inputValue, context));

        resultCode.Combine(OpenParticle::ParticleEmitterInfoSerializer::LoadModule(spawns, emitInfo->m_spawnModules, inputValue, context));

        resultCode.Combine(OpenParticle::ParticleEmitterInfoSerializer::LoadModule(updates, emitInfo->m_updateModules, inputValue, context));

        resultCode.Combine(OpenParticle::ParticleEmitterInfoSerializer::LoadModule(events, emitInfo->m_eventModules, inputValue, context));

        return context.Report(resultCode, "Processed particle.");
    }

    AZ::JsonSerializationResult::Result ParticleEmitterInfoSerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        const void*,
        const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        (void)valueTypeId;

        const ParticleSourceData::EmitterInfo* info = reinterpret_cast<const ParticleSourceData::EmitterInfo*>(inputValue);
        AZ_Assert(info, "Input value for ParticleAssetDataSerializer can't be null.");

        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        resultCode.Combine(
            ContinueStoringToJsonObjectField(outputValue, "name", &info->m_name, nullptr, azrtti_typeid<AZStd::string>(), context));

        AZ::TypeId configId(azrtti_typeid<OpenParticle::EmitterConfig>());
        auto config = info->m_config;
        if (config.empty())
        {
            config = context.GetSerializeContext()->CreateAny(configId);
        }
        resultCode.Combine(
            ContinueStoringToJsonObjectField(outputValue, "config", AZStd::any_cast<void>(&config), nullptr, configId, context));

        // its not necessary to load the asset just to check its id or to store the id.
        auto mat = info->m_material; 
        if (!info->m_material.GetId().IsValid()) 
        {
            AZ::Data::AssetId defaultSpriteEmitMaterialId;
            using editorBus = EditorParticleSystemComponentRequestBus;
            using editorBusEvents = EditorParticleSystemComponentRequestBus::Events;
            editorBus::BroadcastResult(defaultSpriteEmitMaterialId, &editorBusEvents::GetDefaultEmitterMaterialId);
            if (defaultSpriteEmitMaterialId.IsValid())
            {
                // we don't actually have to load the asset, just want to place it into a struct for serialize.
                mat = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(
                    defaultSpriteEmitMaterialId, AZ::Data::AssetLoadBehaviorNamespace::NoLoad);
            }
        }

        auto materialAssetType = azrtti_typeid<AZ::Data::Asset<AZ::RPI::MaterialAsset>>();
        auto modelAssetType = azrtti_typeid<AZ::Data::Asset<AZ::RPI::ModelAsset>>();
        resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "material", &mat, nullptr, materialAssetType, context));

        if (info->m_model.GetId().IsValid())
        {
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "model", &info->m_model, nullptr, modelAssetType, context));
        }

        if (info->m_skeletonModel.GetId().IsValid())
        {
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "skeleton model", &info->m_skeletonModel, nullptr, modelAssetType, context));
        }

        if (!info->m_renderConfig.empty())
        {
            auto renderType = info->m_renderConfig.type();
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue,
                rapidjson::StringRef(GetClassName(renderType, context).data()),
                AZStd::any_cast<void>(&info->m_renderConfig), nullptr, renderType, context));
        }

        AZStd::vector<AZStd::any> orderList;
        auto fn = [&orderList](const AZStd::list<AZStd::any>& list)
        {
            for (auto& any : list)
            {
                orderList.push_back(any);
            }
        };
        fn(info->m_emitModules);
        fn(info->m_spawnModules);
        fn(info->m_updateModules);
        fn(info->m_eventModules);
        for (auto& any : orderList)
        {
            rapidjson::Value moduleNodes;
            resultCode.Combine(ContinueStoring(moduleNodes, AZStd::any_cast<void>(&any), nullptr, any.type(), context));
            if (resultCode.GetProcessing() != JSR::Processing::Halted &&
                (context.ShouldKeepDefaults() || resultCode.GetOutcome() != JSR::Outcomes::DefaultsUsed))
            {
                outputValue.AddMember(
                    rapidjson::StringRef(GetClassName(any.type(), context).data()), moduleNodes, context.GetJsonAllocator());
            }
        }
        return context.Report(resultCode, "Processed particle.");
    }

    template<typename T, size_t size>
    AZ::JsonSerializationResult::ResultCode ParticleBaseSerializer::ConvertToValueObject(
        ValueObject<T, size>& valueObj,
        const AZStd::string& newName,
        const AZStd::string& oldName,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        auto iterator = inputValue.FindMember(newName.c_str());
        if (iterator != inputValue.MemberEnd())
        {
            return resultCode.Combine(JSR::ResultCode(JSR::Tasks::Convert, JSR::Outcomes::Skipped));
        }
        iterator = inputValue.FindMember(oldName.c_str());
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &valueObj.dataValue, azrtti_typeid<T>(), inputValue, rapidjson::Value::StringRefType(oldName.c_str()), context));
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing param name");
        }
        iterator = inputValue.FindMember("distIndex");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &valueObj.distIndex, azrtti_typeid<AZStd::array<size_t, size>>(), inputValue, "distIndex", context));
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing distIndex");
        }
        return resultCode;
    }

    AZ::JsonSerializationResult::ResultCode ParticleBaseSerializer::ConvertToVelocityObject(
        ValueObjFloat& strengthObject,
        ValueObjVec3& directionObject,
        const AZStd::string& newStrength,
        const AZStd::string& strength,
        const AZStd::string& newDirection,
        const AZStd::string& direction,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        auto it1 = inputValue.FindMember(newStrength.c_str());
        auto it2 = inputValue.FindMember(newDirection.c_str());
        if (it1 != inputValue.MemberEnd() && it2 != inputValue.MemberEnd())
        {
            return resultCode.Combine(JSR::ResultCode(JSR::Tasks::Convert, JSR::Outcomes::Skipped));
        }
        else if (it1 != inputValue.MemberEnd() || it2 != inputValue.MemberEnd())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Convert old version Unsupported");
        }
        auto iterator = inputValue.FindMember(strength.c_str());
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(&strengthObject.dataValue,
                azrtti_typeid<float>(), inputValue, rapidjson::Value::StringRefType(strength.c_str()), context));
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing strength name");
        }
        iterator = inputValue.FindMember(direction.c_str());
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(&directionObject.dataValue,
                azrtti_typeid<float>(), inputValue, rapidjson::Value::StringRefType(direction.c_str()), context));
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing direction name");
        }
        iterator = inputValue.FindMember("distIndex");
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(&strengthObject.distIndex,
                azrtti_typeid<AZStd::array<size_t, 1>>(), inputValue, "distIndex", context));
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing distIndex");
        }
        return resultCode;
    }

    AZ::JsonSerializationResult::ResultCode ParticleBaseSerializer::ConvertTo2FloatObject(
        ValueObjFloat& floatObject1,
        ValueObjFloat& floatObject2,
        const AZStd::string& newFloatName1,
        const AZStd::string& floatName1,
        const AZStd::string& newFloatName2,
        const AZStd::string& floatName2,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        auto it1 = inputValue.FindMember(newFloatName1.c_str());
        auto it2 = inputValue.FindMember(newFloatName2.c_str());
        if (it1 != inputValue.MemberEnd() && it2 != inputValue.MemberEnd())
        {
            return resultCode.Combine(JSR::ResultCode(JSR::Tasks::Convert, JSR::Outcomes::Skipped));
        }
        else if (it1 != inputValue.MemberEnd() || it2 != inputValue.MemberEnd())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Convert old version Unsupported");
        }
        auto iterator = inputValue.FindMember(floatName1.c_str());
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &floatObject1.dataValue, azrtti_typeid<float>(), inputValue, rapidjson::Value::StringRefType(floatName1.c_str()), context));
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing param name");
        }
        iterator = inputValue.FindMember(floatName2.c_str());
        if (iterator != inputValue.MemberEnd())
        {
            resultCode.Combine(ContinueLoadingFromJsonObjectField(
                &floatObject2.dataValue, azrtti_typeid<float>(), inputValue, rapidjson::Value::StringRefType(floatName2.c_str()),
                context));
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing param name");
        }
        iterator = inputValue.FindMember("distIndex");
        if (iterator != inputValue.MemberEnd())
        {
            AZStd::array<size_t, 2> distIndex;
            resultCode.Combine(ContinueLoadingFromJsonObjectField(&distIndex, azrtti_typeid<AZStd::array<size_t, 2>>(), inputValue, "distIndex", context));
            floatObject1.distIndex[0] = distIndex[0];
            floatObject2.distIndex[0] = distIndex[1];
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Missing, "Convert old version missing distIndex");
        }
        return resultCode;
    }

    AZ::JsonSerializationResult::Result ParticleLODSerializer::Load(
        void* outputValue,
        const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        (void)outputValueTypeId;
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        ParticleSourceData::Lod* lod = reinterpret_cast<ParticleSourceData::Lod*>(outputValue);

        resultCode.Combine(
            ContinueLoadingFromJsonObjectField(&lod->m_level, azrtti_typeid<AZ::u32>(), inputValue, "level", context));

        resultCode.Combine(ContinueLoadingFromJsonObjectField(
            &lod->m_distance, azrtti_typeid<float>(), inputValue, "maxDistance", context));

        auto iter = inputValue.FindMember(rapidjson::StringRef("emitters"));
        if (iter == inputValue.MemberEnd())
        {
            return context.Report(resultCode, "Processed particle with no emittersIndexes.");
        }

        const auto& emittersIndexes = iter->value;
        if (emittersIndexes.IsArray())
        {
            for (rapidjson_ly::SizeType i = 0; i < emittersIndexes.Size(); ++i)
            {
                AZ::u32 index;
                resultCode.Combine(
                    ContinueLoading(&index, azrtti_typeid<AZ::u32>(), emittersIndexes[i], context));
                lod->m_emitters.emplace_back(index);
            }
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "emitter indexes is not an Array");
        }

        return context.Report(resultCode, "Processed particle.");
    }

    AZ::JsonSerializationResult::Result ParticleLODSerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        const void* defaultValue,
        const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        (void)valueTypeId;
        (void)defaultValue;
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        const ParticleSourceData::Lod* lod = reinterpret_cast<const ParticleSourceData::Lod*>(inputValue);
        AZ_Assert(lod, "Input value for ParticleAssetDataSerializer can't be null.");

        resultCode.Combine(
            ContinueStoringToJsonObjectField(outputValue, "level", &lod->m_level, nullptr, azrtti_typeid<AZ::u32>(), context));

        resultCode.Combine(
            ContinueStoringToJsonObjectField(outputValue, "maxDistance", &lod->m_distance, nullptr, azrtti_typeid<float>(), context));

        rapidjson::Value emitterIndexes;
        emitterIndexes.SetArray();
        for (auto& emitter : lod->m_emitters)
        {
            rapidjson::Value emitterIndex;
            resultCode.Combine(ContinueStoring(emitterIndex, &emitter, nullptr, azrtti_typeid<AZ::u32>(), context));
            if (resultCode.GetProcessing() != JSR::Processing::Halted &&
                (context.ShouldKeepDefaults() || resultCode.GetOutcome() != JSR::Outcomes::DefaultsUsed))
            {
                emitterIndexes.PushBack(emitterIndex, context.GetJsonAllocator());
            }
        }
        outputValue.AddMember(rapidjson::StringRef("emitters"), emitterIndexes, context.GetJsonAllocator());

        return context.Report(resultCode, "Processed particle.");
    }

    AZ::JsonSerializationResult::Result ParticleDistributionSerializer::Load(
        void* outputValue,
        const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        (void)outputValueTypeId;
        (void)outputValue;
        JSR::ResultCode resultCode(JSR::Tasks::ReadField);
        Distribution* distribution = reinterpret_cast<Distribution*>(outputValue);

        auto iter = inputValue.FindMember(rapidjson::StringRef("randoms"));
        if (iter == inputValue.MemberEnd())
        {
            return context.Report(resultCode, "Processed particle with no randoms.");
        }

        const auto& randomsNode = iter->value;
        if (randomsNode.IsArray())
        {
            for (rapidjson_ly::SizeType i = 0; i < randomsNode.Size(); ++i)
            {
                auto* randomModule = aznew Random();
                resultCode.Combine(ContinueLoading(randomModule, azrtti_typeid<Random>(), randomsNode[i], context));
                distribution->randoms.emplace_back(randomModule);
            }
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Value is not an Array");
        }

        iter = inputValue.FindMember(rapidjson::StringRef("curves"));
        if (iter == inputValue.MemberEnd())
        {
            return context.Report(resultCode, "Processed particle with no randoms.");
        }

        const auto& curvesNode = iter->value;
        if (curvesNode.IsArray())
        {
            for (rapidjson_ly::SizeType i = 0; i < curvesNode.Size(); ++i)
            {
                auto* curveModule = aznew Curve();
                resultCode.Combine(ContinueLoading(curveModule, azrtti_typeid<Curve>(), curvesNode[i], context));
                distribution->curves.emplace_back(curveModule);
            }
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Value is not an Array");
        }

        return context.Report(resultCode, "Processed particle.");
    }

    AZ::JsonSerializationResult::Result ParticleDistributionSerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        const void* defaultValue,
        const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        (void)valueTypeId;
        (void)defaultValue;
        const Distribution* dist = reinterpret_cast<const Distribution*>(inputValue);
        AZ_Assert(dist, "Input value for ParticleAssetDataSerializer can't be null.");

        JSR::ResultCode resultCode(JSR::Tasks::ReadField);

        rapidjson::Value randomNodes;
        randomNodes.SetArray();
        for (auto random : dist->randoms)
        {
            rapidjson::Value randomNode;
            resultCode.Combine(ContinueStoring(randomNode, random, nullptr, azrtti_typeid<Random*>(), context));
            if (resultCode.GetProcessing() != JSR::Processing::Halted &&
                (context.ShouldKeepDefaults() || resultCode.GetOutcome() != JSR::Outcomes::DefaultsUsed))
            {
                randomNodes.PushBack(randomNode, context.GetJsonAllocator());
            }
        }

        outputValue.AddMember(rapidjson::StringRef("randoms"), randomNodes, context.GetJsonAllocator());

        rapidjson::Value curveNodes;
        curveNodes.SetArray();
        for (auto& curve : dist->curves)
        {
            rapidjson::Value curveNode;
            resultCode.Combine(ContinueStoring(curveNode, curve, nullptr, azrtti_typeid<Curve*>(), context));
            if (resultCode.GetProcessing() != JSR::Processing::Halted &&
                (context.ShouldKeepDefaults() || resultCode.GetOutcome() != JSR::Outcomes::DefaultsUsed))
            {
                curveNodes.PushBack(curveNode, context.GetJsonAllocator());
            }
        }
        outputValue.AddMember(rapidjson::StringRef("curves"), curveNodes, context.GetJsonAllocator());
        return context.Report(resultCode, "Processed particle.");
    }
} // namespace OpenParticle
