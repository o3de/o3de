/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SelectionComponent.h"

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        class SelectionComponentSerializer : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(SelectionComponentSerializer, "{469D6DA3-0960-4A84-B806-4FCD0C7A7EBC}", AZ::BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            SelectionComponentSerializer() = default;
            ~SelectionComponentSerializer() override = default;

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

        AZ_CLASS_ALLOCATOR_IMPL(SelectionComponentSerializer, AZ::SystemAllocator, 0);

        AZ::JsonSerializationResult::Result SelectionComponentSerializer::Load(
            [[maybe_unused]] void* outputValue,
            [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
            [[maybe_unused]] const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;
            return context.Report(JSR::Tasks::Clear, JSR::Outcomes::Success, "empty");
        }

        AZ::JsonSerializationResult::Result SelectionComponentSerializer::Store(
            [[maybe_unused]] rapidjson::Value& outputValue,
            [[maybe_unused]] const void* inputValue,
            [[maybe_unused]] const void* defaultValue,
            [[maybe_unused]] const AZ::Uuid& valueTypeId,
            [[maybe_unused]] AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "empty");
        }

        void SelectionComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SelectionComponent, EditorComponentBase>();
            }

            if (AZ::JsonRegistrationContext* jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                jsonRegistration->Serializer<SelectionComponentSerializer>()->HandlesType<SelectionComponent>();
            }
        }
    } // namespace Components
} // namespace AzToolsFramework
