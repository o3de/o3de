/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <Serializer/ParticleSourceData.h>

namespace AZ::Data
{
    template<typename T>
    class Asset;
};

namespace OpenParticle
{
    class ParticleBaseSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        using VersionConvertor = AZStd::function<void(
            ParticleBaseSerializer*, AZStd::any&, const rapidjson::Value&, AZ::JsonDeserializerContext&)>;

        template<typename T>
        void ConvertOldModule(AZStd::any& module, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

        //! Helper function to convert a source file name(string) to an asset reference(Asset<T>)
        template<typename T>
        AZ::JsonSerializationResult::ResultCode ConvertSourceFileNameToAsset(
            AZ::Data::Asset<T>& destField,
            const rapidjson::Value& inputValue,
            const char* fieldName,
            AZ::Data::AssetLoadBehavior loadBehavior,
            AZ::JsonDeserializerContext& context);

        // Convert Type: float -> ValueFloatObj / AZ::Vector3 -> ValueVec3Obj / AZ::Color -> ValueObjColor
        template<typename T, size_t size>
        AZ::JsonSerializationResult::ResultCode ConvertToValueObject(
            ValueObject<T, size>& valueObj,
            const AZStd::string& newName,
            const AZStd::string& oldName,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context);
        // Strength(float) + Direction(AZ::Vector3) -> Strength(ValueFloatObj) + Direction(ValueVec3Obj)
        AZ::JsonSerializationResult::ResultCode ConvertToVelocityObject(
            ValueObjFloat& strengthObject,
            ValueObjVec3& directionObject,
            const AZStd::string& newStrength,
            const AZStd::string& strength,
            const AZStd::string& newDirection,
            const AZStd::string& direction,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context);
        // float + float -> ValueFloatObj + ValueFloatObj
        AZ::JsonSerializationResult::ResultCode ConvertTo2FloatObject(
            ValueObjFloat& floatObject1,
            ValueObjFloat& floatObject2,
            const AZStd::string& newFloatName1,
            const AZStd::string& floatName1,
            const AZStd::string& newFloatName2,
            const AZStd::string& floatName2,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context);
    protected:
        AZ::JsonSerializationResult::ResultCode LoadModule(
            AZStd::unordered_map<AZ::TypeId, VersionConvertor>& list,
            AZStd::list<AZStd::any>& modules,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context);

        AZ::JsonSerializationResult::ResultCode LoadModule(
            AZStd::unordered_map<AZ::TypeId, VersionConvertor>& list,
            AZStd::any& module,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context);

        AZ::JsonSerializationResult::ResultCode LoadModule(
            AZ::TypeId id,
            AZStd::any& module,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context);

        AZStd::string_view GetClassName(AZ::TypeId id, AZ::JsonBaseContext& context);
    };

    class ParticleEmitterInfoSerializer final
        : public ParticleBaseSerializer
    {
    public:
        AZ_RTTI(OpenParticle::ParticleEmitterInfoSerializer, "{afb58a1a-541d-46f4-a78f-60512d2df50d}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(
            void* outputValue,
            const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context) override;
    };

    class ParticleLODSerializer final
        : public ParticleBaseSerializer
    {
    public:
        AZ_RTTI(OpenParticle::ParticleLODSerializer, "{F94B0FAB-A5A4-40F4-9608-26FE6D029BFD}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(
            void* outputValue,
            const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context) override;
    };

    class ParticleDistributionSerializer final
        : public ParticleBaseSerializer
    {
    public:
        AZ_RTTI(OpenParticle::ParticleDistributionSerializer, "{d5e58499-38fd-4432-b232-8f2f64f777b4}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(
            void* outputValue,
            const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context) override;
    };
} // namespace OpenParticle
