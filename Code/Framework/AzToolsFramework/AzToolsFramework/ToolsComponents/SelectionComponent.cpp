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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/EntitySerializer.h>

namespace AzToolsFramework
{
    namespace Components
    {
        static constexpr const char* const SelectionComponentLoadMessage =
            "The editor SelectionComponent is being skipped and will be removed from the .prefab file the next time it is saved.";

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
        };

        AZ_CLASS_ALLOCATOR_IMPL(SelectionComponentSerializer, AZ::SystemAllocator);

        AZ::JsonSerializationResult::Result SelectionComponentSerializer::Load(
            [[maybe_unused]] void* outputValue,
            [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
            [[maybe_unused]] const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context)
        {
            // Mark this component as deprecated to inform the user that it was skipped
            AZ::DeprecatedComponentMetadata* deprecatedComponents = context.GetMetadata().Find<AZ::DeprecatedComponentMetadata>();
            if (deprecatedComponents)
            {
                deprecatedComponents->AddComponent(outputValueTypeId);
            }

            // Report to the deserializer that this component should no longer be part of the data
            namespace JSR = AZ::JsonSerializationResult;
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unavailable, SelectionComponentLoadMessage);
        }

        void SelectionComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SelectionComponent, EditorComponentBase>()->Version(2);
                serializeContext->ClassDeprecate("SelectionComponent", AZ::Uuid("{73B724FC-43D1-4C75-ACF5-79AA8A3BF89D}"));
            }

            if (AZ::JsonRegistrationContext* jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                jsonRegistration->Serializer<SelectionComponentSerializer>()->HandlesType<SelectionComponent>();
            }
        }
    } // namespace Components
} // namespace AzToolsFramework
