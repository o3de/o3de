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
        AZStd::string finalKey = baseKey;
        if (it.IsString())
        {
            if (translationFormat->m_database.find(finalKey) == translationFormat->m_database.end())
            {
                translationFormat->m_database[finalKey] = it.GetString();
            }
            else
            {
                AZStd::string existingValue = translationFormat->m_database[finalKey.c_str()];

                // There is a name collision
                AZStd::string error = AZStd::string::format("Unable to store key: %s with value: %s because that key already exists with value: %s (proposed: %s)", finalKey.c_str(), it.GetString(), existingValue.c_str(), it.GetString());
                AZ_Error("TranslationSerializer", false, error.c_str());
            }
        }
        else if (it.IsObject())
        {
            if (!name.empty())
            {
                finalKey.append(".");
                finalKey.append(name);
            }

            AZStd::string itemKey = finalKey;
            for (auto objIt = it.MemberBegin(); objIt != it.MemberEnd(); ++objIt)
            {
                itemKey.append(".");
                itemKey.append(objIt->name.GetString());

                AddEntryToDatabase(itemKey, name, objIt->value, translationFormat);

                itemKey = finalKey;
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

            AZStd::string itemKey = key;

            const rapidjson::Value& array = it;
            for (rapidjson::SizeType i = 0; i < array.Size(); ++i)
            {
                // if there is a "base" member within the object, then use it, otherwise use the index
                if (array[i].IsObject())
                {
                    if (array[i].HasMember(Schema::Field::key))
                    {
                        AZStd::string innerKey = array[i].FindMember(Schema::Field::key)->value.GetString();
                        itemKey.append(AZStd::string::format(".%s", innerKey.c_str()));
                    }
                    else
                    {
                        itemKey.append(AZStd::string::format(".%d", i));
                    }
                }

                AddEntryToDatabase(itemKey, "", array[i], translationFormat);

                itemKey = key;
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

            rapidjson::SizeType entryCount = entries->value.Size();
            for (rapidjson::SizeType i = 0; i < entryCount; ++i)
            {
                const rapidjson::Value& entry = entries->value[i];

                AZStd::string keyStr;
                rapidjson::Value::ConstMemberIterator keyValue;
                if (entry.HasMember(Schema::Field::key))
                {
                    keyValue = entry.FindMember(Schema::Field::key);
                    keyStr = keyValue->value.GetString();
                }

                AZStd::string contextStr;
                rapidjson::Value::ConstMemberIterator contextValue;
                if (entry.HasMember(Schema::Field::context))
                {
                    contextValue = entry.FindMember(Schema::Field::context);
                    contextStr = contextValue->value.GetString();
                }

                AZStd::string variantStr;
                rapidjson::Value::ConstMemberIterator variantValue;
                if (entry.HasMember(Schema::Field::variant))
                {
                    variantValue = entry.FindMember(Schema::Field::variant);
                    variantStr = variantValue->value.GetString();
                }

                AZStd::string baseKey = contextStr;
                if (keyStr.empty())
                {
                    AZ_Error("TranslationDatabase", false, "Every entry in the Translation data must have a key: %s", baseKey.c_str());
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Every entry in the Translation data must have a key");
                }

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
                    if (it == keyValue || it == contextValue || it == variantValue)
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
