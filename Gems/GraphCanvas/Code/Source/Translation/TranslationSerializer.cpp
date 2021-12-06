/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Translation/TranslationSerializer.h>
#include <Translation/TranslationAsset.h>

namespace GraphCanvas
{
    AZ_CLASS_ALLOCATOR_IMPL(TranslationFormatSerializer, AZ::SystemAllocator, 0);

    void AddEntryToDatabase(const AZStd::string& baseKey, const AZStd::string& name, const rapidjson::Value& it, TranslationFormat* translationFormat)
    {
        if (it.IsString())
        {
            auto translationDbItr = translationFormat->m_database.find(baseKey);
            if (translationDbItr == translationFormat->m_database.end())
            {
                translationFormat->m_database[baseKey] = it.GetString();
            }
            else
            {
                const AZStd::string& existingValue = translationDbItr->second;

                // There is a name collision
                const AZStd::string error = AZStd::string::format("Unable to store key: %s with value: %s because that key already exists with value: %s (proposed: %s)", baseKey.c_str(), it.GetString(), existingValue.c_str(), it.GetString());
                AZ_Error("TranslationSerializer", false, error.c_str());
            }
        }
        else if (it.IsObject())
        {
            AZStd::string finalKey = baseKey;
            if (!name.empty())
            {
                finalKey.append(".");
                finalKey.append(name);
            }

            AZStd::string itemKey;
            for (auto objIt = it.MemberBegin(); objIt != it.MemberEnd(); ++objIt)
            {
                itemKey = finalKey;
                itemKey.append(".");
                itemKey.append(objIt->name.GetString());

                AddEntryToDatabase(itemKey, name, objIt->value, translationFormat);
            }

        }
        else if (it.IsArray())
        {
            AZStd::string key = baseKey;
            if (!name.empty())
            {
                key.append(".");
                key.append(name);
            }

            AZStd::string itemKey;

            const rapidjson::Value& array = it;
            for (rapidjson::SizeType i = 0; i < array.Size(); ++i)
            {
                itemKey = key;

                // if there is a "base" member within the object, then use it, otherwise use the index
                const auto& element = array[i];
                if (element.IsObject())
                {
                    rapidjson::Value::ConstMemberIterator innerKeyItr = element.FindMember(Schema::Field::key);
                    if (innerKeyItr != element.MemberEnd())
                    {
                        itemKey.append(AZStd::string::format(".%s", innerKeyItr->value.GetString()));
                    }
                    else
                    {
                        itemKey.append(AZStd::string::format(".%d", i));
                    }
                }

                AddEntryToDatabase(itemKey, "", element, translationFormat);
            }
        }
    }

    AZ::JsonSerializationResult::Result TranslationFormatSerializer::Load(void* outputValue, const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<TranslationFormat>() == outputValueTypeId,
            "Unable to deserialize TranslationFormat to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        TranslationFormat* translationFormat = reinterpret_cast<TranslationFormat*>(outputValue);
        AZ_Assert(translationFormat, "Output value for TranslationFormatSerializer can't be null.");

        namespace JSR = AZ::JsonSerializationResult;
        JSR::ResultCode result(JSR::Tasks::ReadField);

        if (!inputValue.IsObject())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Translation data must be a JSON object.");
        }

        if (!inputValue.HasMember(Schema::Field::entries) || !inputValue[Schema::Field::entries].IsArray())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Translation data must be have a top level array of: entries");
        }

        // Go over the entries
        {
            const rapidjson::Value::ConstMemberIterator entries = inputValue.FindMember(Schema::Field::entries);

            AZStd::string keyStr;
            AZStd::string contextStr;
            AZStd::string variantStr;
            AZStd::string baseKey;

            rapidjson::SizeType entryCount = entries->value.Size();
            for (rapidjson::SizeType i = 0; i < entryCount; ++i)
            {
                const rapidjson::Value& entry = entries->value[i];

                rapidjson::Value::ConstMemberIterator keyItr = entry.FindMember(Schema::Field::key);
                keyStr = keyItr != entry.MemberEnd() ? keyItr->value.GetString() : "";

                rapidjson::Value::ConstMemberIterator contextItr = entry.FindMember(Schema::Field::context);
                contextStr = contextItr != entry.MemberEnd() ? contextItr->value.GetString() : "";

                rapidjson::Value::ConstMemberIterator variantItr = entry.FindMember(Schema::Field::variant);
                variantStr = variantItr != entry.MemberEnd() ? variantItr->value.GetString() : "";

                if (keyStr.empty())
                {
                    AZ_Error("TranslationDatabase", false, "Every entry in the Translation data must have a key: %s", contextStr.c_str());
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Every entry in the Translation data must have a key");
                }

                baseKey = contextStr;
                if (!baseKey.empty())
                {
                    baseKey.append(".");
                }

                baseKey.append(keyStr);

                if (!variantStr.empty())
                {
                    baseKey.append(".");
                    baseKey.append(variantStr);
                }

                // Iterate over all members of entry
                for (auto it = entry.MemberBegin(); it != entry.MemberEnd(); ++it)
                {
                    // Skip the fixed elements
                    if (it == keyItr || it == contextItr || it == variantItr)
                    {
                        continue;
                    }
                    AddEntryToDatabase(baseKey, it->name.GetString(), it->value, translationFormat);
                }
            }
        }

        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Translation load success");
    }

    AZ::JsonSerializationResult::Result TranslationFormatSerializer::Store(rapidjson::Value& /*outputValue*/, const void* /*inputValue*/,
        const void* /*defaultValue*/, const AZ::Uuid& /*valueTypeId*/, AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Unsupported, "Storing a Translation asset is not currently supported");
    }
}
