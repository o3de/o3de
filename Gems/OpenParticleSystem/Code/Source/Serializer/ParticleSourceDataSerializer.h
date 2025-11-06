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

namespace OpenParticle
{
    class ParticleSourceDataSerializer final
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(OpenParticle::ParticleSourceDataSerializer, "{00fa916a-6389-45d5-85e8-75a2e9b185d8}", AZ::BaseJsonSerializer);
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

    private:
        BaseJsonSerializer::OperationFlags GetOperationsFlags() const override;
    };
} // namespace OpenParticle
