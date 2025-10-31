/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::Data
{
    AZ_CLASS_ALLOCATOR_IMPL(AssetJsonSerializer, SystemAllocator);

    JsonSerializationResult::Result AssetJsonSerializer::Load(void* outputValue, const Uuid& /*outputValueTypeId*/,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        switch (inputValue.GetType())
        {
        case rapidjson::kObjectType:
            return LoadAsset(outputValue, inputValue, context);
        case rapidjson::kArrayType: // fall through
        case rapidjson::kNullType: // fall through
        case rapidjson::kStringType: // fall through
        case rapidjson::kFalseType: // fall through
        case rapidjson::kTrueType: // fall through
        case rapidjson::kNumberType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. Asset<T> can only be read from an object.");

        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for Asset<T>.");
        }
    }

    JsonSerializationResult::Result AssetJsonSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& /*valueTypeId*/, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        const Asset<AssetData>* instance = reinterpret_cast<const Asset<AssetData>*>(inputValue);
        const Asset<AssetData>* defaultInstance = reinterpret_cast<const Asset<AssetData>*>(defaultValue);

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        {
            ScopedContextPath subPathId(context, "m_assetId");
            const auto* id = &instance->GetId();
            const auto* defaultId = defaultInstance ? &defaultInstance->GetId() : nullptr;
            rapidjson::Value assetIdValue;
            result = ContinueStoring(assetIdValue, id, defaultId, azrtti_typeid<AssetId>(), context);
            if (result.GetOutcome() == JSR::Outcomes::Success || result.GetOutcome() == JSR::Outcomes::PartialDefaults)
            {
                if (!outputValue.IsObject())
                {
                    outputValue.SetObject();
                }
                outputValue.AddMember(rapidjson::StringRef("assetId"), AZStd::move(assetIdValue), context.GetJsonAllocator());
            }
        }

        {
            const AZ::Data::AssetLoadBehavior autoLoadBehavior = instance->GetAutoLoadBehavior();
            const AZ::Data::AssetLoadBehavior defaultAutoLoadBehavior = defaultInstance ?
                defaultInstance->GetAutoLoadBehavior() : AZ::Data::AssetLoadBehavior::Default;

            result.Combine(
                ContinueStoringToJsonObjectField(outputValue, "loadBehavior",
                    &autoLoadBehavior, &defaultAutoLoadBehavior,
                    azrtti_typeid<Data::AssetLoadBehavior>(), context));
        }

        {
            ScopedContextPath subPathHint(context, "m_assetHint");
            const AZStd::string* hint = &instance->GetHint();
            const AZStd::string defaultHint;
            rapidjson::Value assetHintValue;
            JSR::ResultCode resultHint = ContinueStoring(assetHintValue, hint, &defaultHint, azrtti_typeid<AZStd::string>(), context);
            if (resultHint.GetOutcome() == JSR::Outcomes::Success || resultHint.GetOutcome() == JSR::Outcomes::PartialDefaults)
            {
                if (!outputValue.IsObject())
                {
                    outputValue.SetObject();
                }
                outputValue.AddMember(rapidjson::StringRef("assetHint"), AZStd::move(assetHintValue), context.GetJsonAllocator());
            }
            result.Combine(resultHint);
        }

        if (SerializedAssetTracker* assetTracker = context.GetMetadata().Find<SerializedAssetTracker>();
            assetTracker != nullptr && result.GetProcessing() == JSR::Processing::Completed)
        {
            assetTracker->AddAsset(*instance);
        }

        return context.Report(result,
            result.GetProcessing() == JSR::Processing::Completed ? "Successfully stored Asset<T>." : "Failed to store Asset<T>.");
    }

    JsonSerializationResult::Result AssetJsonSerializer::LoadAsset(void* outputValue, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        Asset<AssetData>* instance = reinterpret_cast<Asset<AssetData>*>(outputValue);
        AssetId id;
        JSR::ResultCode result(JSR::Tasks::ReadField);

        SerializedAssetTracker* assetTracker =
            context.GetMetadata().Find<SerializedAssetTracker>();

        {
            Data::AssetLoadBehavior loadBehavior = instance->GetAutoLoadBehavior();

            result =
                ContinueLoadingFromJsonObjectField(&loadBehavior,
                    azrtti_typeid<Data::AssetLoadBehavior>(),
                    inputValue, "loadBehavior", context);

            instance->SetAutoLoadBehavior(loadBehavior);
        }

        auto it = inputValue.FindMember("assetId");
        if (it != inputValue.MemberEnd())
        {
            ScopedContextPath subPath(context, "assetId");
            result.Combine(ContinueLoading(&id, azrtti_typeid<AssetId>(), it->value, context));

            if (result.GetProcessing() == JSR::Processing::Completed)
            {
                const auto currentLoadBehavior = instance->GetAutoLoadBehavior();
                AZ::Data::AssetInfo foundAssetInfo;
                AssetCatalogRequestBus::BroadcastResult(foundAssetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, id);

                if (foundAssetInfo.m_assetId.IsValid())
                {
                    // The Asset is registered with the catalog, so check if there is an Asset Handler registered for its AssetType
                    if (AZ::Data::AssetHandler* assetHandler = AssetManager::Instance().GetHandler(foundAssetInfo.m_assetType);
                        assetHandler != nullptr)
                    {
                        // There is a registered handler for the AssetType, so call FindOrCreateAsset which can
                        // load the Asset using the current auto load behavior
                        if (Asset<AssetData> foundAsset = AssetManager::Instance().FindOrCreateAsset(
                                foundAssetInfo.m_assetId, foundAssetInfo.m_assetType, currentLoadBehavior);
                            foundAsset.GetId().IsValid())
                        {
                            // The Asset has been successfully found and the auto load behavior has been used
                            *instance = AZStd::move(foundAsset);
                            result.Combine(context.Report(result, "Successfully created and found Asset<T> with id."));
                        }
                        else
                        {
                            // The Asset failed to be found, so fall back to constructing a new Asset<AssetData>
                            // instance using the AssetId and AssetType from the catalog
                            *instance = Asset<AssetData>(foundAssetInfo.m_assetId, foundAssetInfo.m_assetType);
                            instance->SetAutoLoadBehavior(currentLoadBehavior);
                            result.Combine(context.Report(result, "Asset Info was found in the Asset Catalog and Asset Type for the Asset Id"
                                " has an Asset Handler registered, however the Asset cannot be found in the Asset Manager."));
                        }
                    }
                    else
                    {
                        // There is no Asset Handler registered for the Asset Type, so make a new Asset<AssetData>
                        // but don't use FindOrCreateAsset to use its autoload behavior
                        *instance = Asset<AssetData>(foundAssetInfo.m_assetId, foundAssetInfo.m_assetType);
                        instance->SetAutoLoadBehavior(currentLoadBehavior);
                        result.Combine(context.Report(result, "Asset Info was found in the Asset Catalog, but the Asset Type"
                            " does not have an Asset Handler registered that could load the Asset."));
                    }
                }
                else
                {
                    // The Asset ID is not registered with the catalog, so set the instance to the loaded Id, but
                    // maintain its Asset Type
                    *instance = Asset<AssetData>(id, instance->GetType());
                    instance->SetAutoLoadBehavior(currentLoadBehavior);

                    result.Combine(context.Report(
                        JSR::Tasks::ReadField,
                        JSR::Outcomes::PartialDefaults,
                        "Asset<T> created, however the Asset Info was not found in the Asset Catalog."
                        " The AssetType from the supplied output instance will be used"));
                }
            }
            else
            {
                result.Combine(context.Report(result, "Failed to retrieve asset id for Asset<T>."));
            }
        }
        else
        {
            result.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                "The asset id is missing, so there's not enough information to create an Asset<T>."));
        }

        it = inputValue.FindMember("assetHint");
        if (it != inputValue.MemberEnd())
        {
            ScopedContextPath subPath(context, "assetHint");
            AZStd::string hint;
            result.Combine(ContinueLoading(&hint, azrtti_typeid<AZStd::string>(), it->value, context));
            instance->SetHint(AZStd::move(hint));
        }
        else
        {
            result.Combine(context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                "The asset hint is missing for Asset<T>, so it will be left empty."));
        }

        if (assetTracker)
        {
            assetTracker->FixUpAsset(*instance);
            assetTracker->AddAsset(*instance);
        }

        bool success = result.GetOutcome() <= JSR::Outcomes::PartialSkip;
        bool defaulted = result.GetOutcome() == JSR::Outcomes::DefaultsUsed || result.GetOutcome() == JSR::Outcomes::PartialDefaults;
        AZStd::string_view message =
            success ? "Successfully loaded information and created instance of Asset<T>." :
            defaulted ? "A default id was provided for Asset<T>, so no instance could be created." :
            "Not enough information was available to create an instance of Asset<T> or data was corrupted.";
        return context.Report(result, message);
    }

    void SerializedAssetTracker::SetAssetFixUp(AssetFixUp assetFixUpCallback)
    {
        m_assetFixUpCallback = AZStd::move(assetFixUpCallback);
    }

    void SerializedAssetTracker::FixUpAsset(Asset<AssetData>& asset)
    {
        if (m_assetFixUpCallback)
        {
            m_assetFixUpCallback(asset);
        }
    }

    void SerializedAssetTracker::AddAsset(Asset<AssetData> asset)
    {
        m_serializedAssets.emplace_back(asset);
    }

    const AZStd::vector<Asset<AssetData>>& SerializedAssetTracker::GetTrackedAssets() const
    {
        return m_serializedAssets;
    }

    AZStd::vector<Asset<AssetData>>& SerializedAssetTracker::GetTrackedAssets()
    {
        return m_serializedAssets;
    }

} // namespace AZ::Data
