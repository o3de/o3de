/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class JsonEntitySerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonEntitySerializer, "{015BBF46-E21A-41AA-816A-C63119FB2852}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
            JsonSerializerContext& context) override;
    };

    //! Helper class to track components that have been skipped during loading.
    //! When this class is added to the metadata of a Json deserializer setting,
    //! custom component serializers can add themselves to the list so users
    //! can be informed of component deprecation upon load completion
    class DeprecatedComponentMetadata
    {
    public:
        AZ_TYPE_INFO(DeprecatedComponentMetadata, "{3D5F5EAE-BDA9-43AA-958E-E87158BAFB9F}");
        using EnableDeprecationTrackingCallback = AZStd::function<bool()>;

        ~DeprecatedComponentMetadata() = default;

        void SetEnableDeprecationTrackingCallback(EnableDeprecationTrackingCallback callback);
        void AddComponent(const TypeId& componentType);
        AZStd::vector<AZStd::string> GetComponentNames() const;

    private:
        AZStd::unordered_set<AZ::TypeId> m_componentTypes;
        EnableDeprecationTrackingCallback m_enableDeprecationTrackingCallback;
    };
}
