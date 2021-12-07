/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityScrubber.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceSerializer.h>

#include <QDebug>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabDomUtils
        {
            namespace Internal
            {
                AZ::JsonSerializationResult::ResultCode JsonIssueReporter(AZStd::string& scratchBuffer,
                    AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
                {
                    namespace JSR = AZ::JsonSerializationResult;

                    if (result.GetProcessing() == JSR::Processing::Halted)
                    {
                        scratchBuffer.append(message.begin(), message.end());
                        scratchBuffer.append("\n    Reason: ");
                        result.AppendToString(scratchBuffer, path);
                        scratchBuffer.append(".");
                        AZ_Warning("Prefab Serialization", false, "%s", scratchBuffer.c_str());

                        scratchBuffer.clear();

                        return JSR::ResultCode(result.GetTask(), JSR::Outcomes::Skipped);
                    }

                    return result;
                }
            }

            PrefabDomValueReference FindPrefabDomValue(PrefabDomValue& parentValue, const char* valueName)
            {
                PrefabDomValue::MemberIterator valueIterator = parentValue.FindMember(valueName);
                if (valueIterator == parentValue.MemberEnd())
                {
                    return AZStd::nullopt;
                }

                return valueIterator->value;
            }

            PrefabDomValueConstReference FindPrefabDomValue(const PrefabDomValue& parentValue, const char* valueName)
            {
                PrefabDomValue::ConstMemberIterator valueIterator = parentValue.FindMember(valueName);
                if (valueIterator == parentValue.MemberEnd())
                {
                    return AZStd::nullopt;
                }

                return valueIterator->value;
            }

            bool StoreInstanceInPrefabDom(const Instance& instance, PrefabDom& prefabDom, StoreFlags flags)
            {
                InstanceEntityIdMapper entityIdMapper;
                entityIdMapper.SetStoringInstance(instance);

                // Need to store the id mapper as both its type and its base type
                // Meta data is found by type id and we need access to both types at different levels (Instance, EntityId)
                AZ::JsonSerializerSettings settings;
                settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                settings.m_metadata.Add(&entityIdMapper);

                if ((flags & StoreFlags::StripDefaultValues) != StoreFlags::StripDefaultValues)
                {
                    settings.m_keepDefaults = true;
                }

                if ((flags & StoreFlags::StoreLinkIds) != StoreFlags::None)
                {
                    settings.m_metadata.Create<LinkIdMetadata>();
                }

                AZStd::string scratchBuffer;
                auto issueReportingCallback = [&scratchBuffer]
                (AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result,
                 AZStd::string_view path) -> AZ::JsonSerializationResult::ResultCode
                {
                    return Internal::JsonIssueReporter(scratchBuffer, message, result, path);
                };

                settings.m_reporting = AZStd::move(issueReportingCallback);

                AZ::JsonSerializationResult::ResultCode result =
                    AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), instance, settings);

                if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    AZ_Error("Prefab", false,
                        "Failed to serialize prefab instance with source path %s. "
                        "Unable to proceed.",
                        instance.GetTemplateSourcePath().c_str());

                    return false;
                }

                return true;
            }

            bool StoreEntityInPrefabDomFormat(const AZ::Entity& entity, Instance& owningInstance, PrefabDom& prefabDom, StoreFlags flags)
            {
                InstanceEntityIdMapper entityIdMapper;
                entityIdMapper.SetStoringInstance(owningInstance);

                //create settings so that the serialized entity dom undergoes mapping from entity id to entity alias
                AZ::JsonSerializerSettings settings;
                settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));

                if ((flags & StoreFlags::StripDefaultValues) != StoreFlags::StripDefaultValues)
                {
                    settings.m_keepDefaults = true;
                }

                AZStd::string scratchBuffer;
                auto issueReportingCallback = [&scratchBuffer]
                (AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result,
                    AZStd::string_view path) -> AZ::JsonSerializationResult::ResultCode
                {
                    return Internal::JsonIssueReporter(scratchBuffer, message, result, path);
                };

                settings.m_reporting = AZStd::move(issueReportingCallback);

                //generate PrefabDom using Json serialization system
                AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Store(
                    prefabDom, prefabDom.GetAllocator(), entity, settings);

                return result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success;
            }

            // some assets may come in from the JSON serialzier with no AssetID, but have an asset hint
            // this attempts to fix up the assets using the assetHint field
            void FixUpInvalidAssets(AZ::Data::Asset<AZ::Data::AssetData>& asset)
            {
                if (!asset.GetId().IsValid() && !asset.GetHint().empty())
                {
                    AZ::Data::AssetId assetId;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, asset.GetHint().c_str(),
                        AZ::Data::s_invalidAssetType, false);

                    if (assetId.IsValid())
                    {
                        asset.Create(assetId, false);
                    }
                }
            }

            bool LoadInstanceFromPrefabDom(Instance& instance, const PrefabDom& prefabDom, LoadFlags flags)
            {
                // When entities are rebuilt they are first destroyed. As a result any assets they were exclusively holding on to will
                // be released and reloaded once the entities are built up again. By suspending asset release temporarily the asset reload
                // is avoided.
                AZ::Data::AssetManager::Instance().SuspendAssetRelease();

                InstanceEntityIdMapper entityIdMapper;
                entityIdMapper.SetLoadingInstance(instance);
                if ((flags & LoadFlags::AssignRandomEntityId) == LoadFlags::AssignRandomEntityId)
                {
                    entityIdMapper.SetEntityIdGenerationApproach(InstanceEntityIdMapper::EntityIdGenerationApproach::Random);
                }

                auto tracker = AZ::Data::SerializedAssetTracker{};
                tracker.SetAssetFixUp(&FixUpInvalidAssets);

                AZ::JsonDeserializerSettings settings;
                // The InstanceEntityIdMapper is registered twice because it's used in several places during deserialization where one is
                // specific for the InstanceEntityIdMapper and once for the generic JsonEntityIdMapper. Because the Json Serializer's meta
                // data has strict typing and doesn't look for inheritance both have to be explicitly added so they're found both locations.
                settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                settings.m_metadata.Add(&entityIdMapper);
                settings.m_metadata.Add(tracker);

                AZ::JsonSerializationResult::ResultCode result =
                    AZ::JsonSerialization::Load(instance, prefabDom, settings);

                AZ::Data::AssetManager::Instance().ResumeAssetRelease();

                if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    AZ_Error("Prefab", false,
                        "Failed to de-serialize Prefab Instance from Prefab DOM. "
                        "Unable to proceed.");

                    return false;
                }

                return true;
            }

            bool LoadInstanceFromPrefabDom(
                Instance& instance, const PrefabDom& prefabDom, AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets, LoadFlags flags)
            {
                // When entities are rebuilt they are first destroyed. As a result any assets they were exclusively holding on to will
                // be released and reloaded once the entities are built up again. By suspending asset release temporarily the asset reload
                // is avoided.
                AZ::Data::AssetManager::Instance().SuspendAssetRelease();

                InstanceEntityIdMapper entityIdMapper;
                entityIdMapper.SetLoadingInstance(instance);
                if ((flags & LoadFlags::AssignRandomEntityId) == LoadFlags::AssignRandomEntityId)
                {
                    entityIdMapper.SetEntityIdGenerationApproach(InstanceEntityIdMapper::EntityIdGenerationApproach::Random);
                }

                auto tracker = AZ::Data::SerializedAssetTracker{};
                tracker.SetAssetFixUp(&FixUpInvalidAssets);

                AZ::JsonDeserializerSettings settings;
                // The InstanceEntityIdMapper is registered twice because it's used in several places during deserialization where one is
                // specific for the InstanceEntityIdMapper and once for the generic JsonEntityIdMapper. Because the Json Serializer's meta
                // data has strict typing and doesn't look for inheritance both have to be explicitly added so they're found both locations.
                settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                settings.m_metadata.Add(&entityIdMapper);
                settings.m_metadata.Add(tracker);

                AZ::JsonSerializationResult::ResultCode result =
                    AZ::JsonSerialization::Load(instance, prefabDom, settings);

                AZ::Data::AssetManager::Instance().ResumeAssetRelease();

                if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    AZ_Error("Prefab", false,
                        "Failed to de-serialize Prefab Instance from Prefab DOM. "
                        "Unable to proceed.");

                    return false;
                }

                AZ::Data::SerializedAssetTracker* assetTracker = settings.m_metadata.Find<AZ::Data::SerializedAssetTracker>();

                referencedAssets = AZStd::move(assetTracker->GetTrackedAssets());
                return true;
            }

            bool LoadInstanceFromPrefabDom(
                Instance& instance, Instance::EntityList& newlyAddedEntities, const PrefabDom& prefabDom, LoadFlags flags)
            {
                // When entities are rebuilt they are first destroyed. As a result any assets they were exclusively holding on to will
                // be released and reloaded once the entities are built up again. By suspending asset release temporarily the asset reload
                // is avoided.
                AZ::Data::AssetManager::Instance().SuspendAssetRelease();

                InstanceEntityIdMapper entityIdMapper;
                entityIdMapper.SetLoadingInstance(instance);
                if ((flags & LoadFlags::AssignRandomEntityId) == LoadFlags::AssignRandomEntityId)
                {
                    entityIdMapper.SetEntityIdGenerationApproach(InstanceEntityIdMapper::EntityIdGenerationApproach::Random);
                }

                auto tracker = AZ::Data::SerializedAssetTracker{};
                tracker.SetAssetFixUp(&FixUpInvalidAssets);

                AZ::JsonDeserializerSettings settings;
                // The InstanceEntityIdMapper is registered twice because it's used in several places during deserialization where one is
                // specific for the InstanceEntityIdMapper and once for the generic JsonEntityIdMapper. Because the Json Serializer's meta
                // data has strict typing and doesn't look for inheritance both have to be explicitly added so they're found both locations.
                settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                settings.m_metadata.Add(&entityIdMapper);
                settings.m_metadata.Create<InstanceEntityScrubber>(newlyAddedEntities);
                settings.m_metadata.Add(tracker);

                AZStd::string scratchBuffer;
                auto issueReportingCallback = [&scratchBuffer](
                    AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result,
                    AZStd::string_view path) -> AZ::JsonSerializationResult::ResultCode
                {
                    return Internal::JsonIssueReporter(scratchBuffer, message, result, path);
                };
                settings.m_reporting = AZStd::move(issueReportingCallback);

                AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(instance, prefabDom, settings);

                AZ::Data::AssetManager::Instance().ResumeAssetRelease();

                if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    AZ_Error(
                        "Prefab", false,
                        "Failed to de-serialize Prefab Instance from Prefab DOM. "
                        "Unable to proceed.");

                    return false;
                }

                return true;
            }

            void GetTemplateSourcePaths(const PrefabDomValue& prefabDom, AZStd::unordered_set<AZ::IO::Path>& templateSourcePaths)
            {
                PrefabDomValueConstReference findSourceResult = PrefabDomUtils::FindPrefabDomValue(prefabDom, PrefabDomUtils::SourceName);
                if (!findSourceResult.has_value() || !(findSourceResult->get().IsString()) ||
                    findSourceResult->get().GetStringLength() == 0)
                {
                    AZ_Assert(
                        false,
                        "PrefabDomUtils::GetDependentTemplatePath - Source value of prefab in the provided DOM is not a valid string.");
                    return;
                }

                templateSourcePaths.emplace(findSourceResult->get().GetString());
                PrefabDomValueConstReference instancesReference = GetInstancesValue(prefabDom);
                if (instancesReference.has_value())
                {
                    const PrefabDomValue& instances = instancesReference->get();

                    for (PrefabDomValue::ConstMemberIterator instanceIterator = instances.MemberBegin();
                         instanceIterator != instances.MemberEnd(); ++instanceIterator)
                    {
                        GetTemplateSourcePaths(instanceIterator->value, templateSourcePaths);
                    }
                }
            }

            PrefabDomValueConstReference GetInstancesValue(const PrefabDomValue& prefabDom)
            {
                PrefabDomValueConstReference findInstancesResult = FindPrefabDomValue(prefabDom, PrefabDomUtils::InstancesName);
                if (!findInstancesResult.has_value() || !(findInstancesResult->get().IsObject()))
                {
                    return AZStd::nullopt;
                }

                return findInstancesResult->get();
            }

            AZ::JsonSerializationResult::ResultCode ApplyPatches(
                PrefabDomValue& prefabDomToApplyPatchesOn, PrefabDom::AllocatorType& allocator, const PrefabDomValue& patches)
            {
                AZStd::string scratchBuffer;
                auto issueReportingCallback = [&scratchBuffer]
                (AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result,
                    AZStd::string_view path) -> AZ::JsonSerializationResult::ResultCode
                {
                    return Internal::JsonIssueReporter(scratchBuffer, message, result, path);
                };

                AZ::JsonApplyPatchSettings applyPatchSettings;
                applyPatchSettings.m_reporting = AZStd::move(issueReportingCallback);
                return AZ::JsonSerialization::ApplyPatch(
                    prefabDomToApplyPatchesOn, allocator, patches, AZ::JsonMergeApproach::JsonPatch, applyPatchSettings);
            }

            void PrintPrefabDomValue(
                [[maybe_unused]] const AZStd::string_view printMessage,
                [[maybe_unused]] const PrefabDomValue& prefabDomValue)
            {
                rapidjson::StringBuffer prefabBuffer;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabBuffer);
                prefabDomValue.Accept(writer);

                qDebug() << printMessage.data() << "\n" << prefabBuffer.GetString();
            }
        } // namespace PrefabDomUtils
    } // namespace Prefab
} // namespace AzToolsFramework
