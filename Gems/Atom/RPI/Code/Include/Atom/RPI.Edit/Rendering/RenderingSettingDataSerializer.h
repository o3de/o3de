/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>


// Rendering Settings
class MultisampleStateSerializer final : public AZ::BaseJsonSerializer
{
public:
    AZ_RTTI(MultisampleStateSerializer, "{F620192C-B202-4621-B59A-EFA273F1DBC3}", AZ::BaseJsonSerializer);
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

class RenderingSettingsSerializer final : public AZ::BaseJsonSerializer
{
public:
    AZ_RTTI(RenderingSettingsSerializer, "{1CBA9693-CE9D-448D-A132-21A2D88092E0}", AZ::BaseJsonSerializer);
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

class RenderingPipelineDescriptorSerializer final : public AZ::BaseJsonSerializer
{
public:
    AZ_RTTI(RenderingPipelineDescriptorSerializer, "{714E663A-6063-4EE4-A094-67F43018C75F}", AZ::BaseJsonSerializer);
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

class RenderingSettingDataSerializer final : public AZ::BaseJsonSerializer
{
public:
    AZ_RTTI(RenderingSettingDataSerializer, "{65E79D29-2882-440A-998B-3487DE4487BC}", AZ::BaseJsonSerializer);
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
