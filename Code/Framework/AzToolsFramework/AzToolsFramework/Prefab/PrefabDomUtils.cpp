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
                static AZ::JsonSerializationResult::ResultCode JsonIssueReporter(AZStd::string& scratchBuffer,
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

                static bool StoreInstanceInPrefabDom(
                    const Instance& instance,
                    PrefabDom& prefabDom,
                    AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>* referencedAssets,
                    StoreFlags flags)
                {
                    InstanceEntityIdMapper entityIdMapper;
                    entityIdMapper.SetStoringInstance(instance);

                    // Need to store the id mapper as both its type and its base type
                    // Metadata is found by type id and we need access to both types at different levels (Instance, EntityId)
                    AZ::JsonSerializerSettings settings;
                    settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                    settings.m_metadata.Add(&entityIdMapper);
                    if (referencedAssets)
                    {
                        settings.m_metadata.Add(AZ::Data::SerializedAssetTracker{});
                    }

                    if ((flags & StoreFlags::StripDefaultValues) != StoreFlags::StripDefaultValues)
                    {
                        settings.m_keepDefaults = true;
                    }

                    if ((flags & StoreFlags::StripLinkIds) != StoreFlags::StripLinkIds)
                    {
                        settings.m_metadata.Create<LinkIdMetadata>();
                    }

                    AZStd::string scratchBuffer;
                    auto issueReportingCallback = [&scratchBuffer](
                                                      AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result,
                                                      AZStd::string_view path) -> AZ::JsonSerializationResult::ResultCode
                    {
                        return Internal::JsonIssueReporter(scratchBuffer, message, result, path);
                    };

                    settings.m_reporting = AZStd::move(issueReportingCallback);

                    AZ::JsonSerializationResult::ResultCode result =
                        AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), instance, settings);

                    if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                    {
                        AZ_Error(
                            "Prefab", false,
                            "Failed to serialize prefab instance with source path %s. "
                            "Unable to proceed.",
                            instance.GetTemplateSourcePath().c_str());

                        return false;
                    }

                    if (referencedAssets)
                    {
                        *referencedAssets = AZStd::move(settings.m_metadata.Find<AZ::Data::SerializedAssetTracker>()->GetTrackedAssets());
                    }
                    return true;
                }

                // Some assets may come in from the JSON serializer with no AssetID, but have an asset hint.
                // This attempts to fix up the assets using the assetHint field
                static void FixUpInvalidAssets(AZ::Data::Asset<AZ::Data::AssetData>& asset)
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

                static bool LoadInstanceHelper(
                    Instance& instance,
                    const PrefabDom& prefabDom,
                    LoadFlags flags,
                    AZ::JsonDeserializerSettings& settings)
                {
                    // When entities are rebuilt they are first destroyed. As a result any assets they were exclusively holding on to will
                    // be released and reloaded once the entities are built up again. By suspending asset release temporarily the asset
                    // reload is avoided.
                    AZ::Data::AssetManager::Instance().SuspendAssetRelease();

                    InstanceEntityIdMapper entityIdMapper;
                    entityIdMapper.SetLoadingInstance(instance);
                    if ((flags & LoadFlags::AssignRandomEntityId) == LoadFlags::AssignRandomEntityId)
                    {
                        entityIdMapper.SetEntityIdGenerationApproach(InstanceEntityIdMapper::EntityIdGenerationApproach::Random);
                    }

                    auto tracker = AZ::Data::SerializedAssetTracker{};
                    tracker.SetAssetFixUp(&FixUpInvalidAssets);

                    // The InstanceEntityIdMapper is registered twice because it's used in several places during deserialization where one
                    // is specific for the InstanceEntityIdMapper and once for the generic JsonEntityIdMapper. Because the Json Serializer's
                    // meta data has strict typing and doesn't look for inheritance both have to be explicitly added so they're found both
                    // locations.
                    settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                    settings.m_metadata.Add(&entityIdMapper);
                    settings.m_metadata.Add(tracker);

                    if ((flags & LoadFlags::UseSelectiveDeserialization) == LoadFlags::UseSelectiveDeserialization)
                    {
                        settings.m_metadata.Create<InstanceDomMetadata>();
                    }

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

                //! Identifies instance members to be added or removed by inspecting the patch entry provided.
                //! @param patchEntry The patch entry to inspect.
                //! @param membersToAdd The set to add the instance member to if an addition operation is detected.
                //! @param membersToRemove The set to add the instance member to if a remove operation is detected.
                //! @param memberName The name of the instance member found in the patch.
                static void IdentifyInstanceMembersToAddAndRemove(
                    const PrefabDomValue& patchEntry,
                    AZStd::unordered_set<AZStd::string>& membersToAdd,
                    AZStd::unordered_set<AZStd::string>& membersToRemove,
                    AZStd::string_view memberName)
                {
                    PrefabDomValue::ConstMemberIterator patchEntryIterator = patchEntry.FindMember("op");
                    if (patchEntryIterator != patchEntry.MemberEnd())
                    {
                        AZStd::string opPath = patchEntryIterator->value.GetString();

                        if (opPath == "remove")
                        {
                            membersToRemove.emplace(AZStd::move(memberName));
                        }
                        else if (opPath == "add" || opPath == "replace")
                        {
                            // Could be an add or change from empty->full. The later case is rare but not impossible.
                            membersToAdd.emplace(AZStd::move(memberName));
                        }
                    }
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
                return Internal::StoreInstanceInPrefabDom(instance, prefabDom, nullptr, flags);
            }

            bool StoreInstanceInPrefabDom(
                const Instance& instance,
                PrefabDom& prefabDom,
                AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets,
                StoreFlags flags)
            {
                return Internal::StoreInstanceInPrefabDom(instance, prefabDom, &referencedAssets, flags);
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

            bool LoadInstanceFromPrefabDom(Instance& instance, const PrefabDom& prefabDom, LoadFlags flags)
            {
                AZ::JsonDeserializerSettings settings;
                return Internal::LoadInstanceHelper(instance, prefabDom, flags, settings);
            }

            bool LoadInstanceFromPrefabDom(
                Instance& instance, const PrefabDom& prefabDom, AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets, LoadFlags flags)
            {
                AZ::JsonDeserializerSettings settings;

                if (Internal::LoadInstanceHelper(instance, prefabDom, flags, settings))
                {
                    AZ::Data::SerializedAssetTracker* assetTracker = settings.m_metadata.Find<AZ::Data::SerializedAssetTracker>();
                    referencedAssets = AZStd::move(assetTracker->GetTrackedAssets());
                }

                return true;
            }

            bool LoadInstanceFromPrefabDom(
                Instance& instance, EntityList& newlyAddedEntities, const PrefabDom& prefabDom, LoadFlags flags)
            {

                AZ::JsonDeserializerSettings settings;
                settings.m_metadata.Create<InstanceEntityScrubber>(newlyAddedEntities);

                AZStd::string scratchBuffer;
                auto issueReportingCallback = [&scratchBuffer](
                    AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result,
                    AZStd::string_view path) -> AZ::JsonSerializationResult::ResultCode
                {
                    return Internal::JsonIssueReporter(scratchBuffer, message, result, path);
                };
                settings.m_reporting = AZStd::move(issueReportingCallback);

                return Internal::LoadInstanceHelper(instance, prefabDom, flags, settings);
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

            PrefabDomValueReference GetInstancesValue(PrefabDomValue& prefabDom)
            {
                PrefabDomValueReference findInstancesResult = FindPrefabDomValue(prefabDom, PrefabDomUtils::InstancesName);
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

            //! Identifies the instance members to reload by parsing through the patches provided.
            PatchesMetadata IdentifyModifiedInstanceMembers(const PrefabDom& patches)
            {
                PrefabDomUtils::PatchesMetadata patchesMetadata;
                for (const PrefabDomValue& patchEntry : patches.GetArray())
                {
                    PrefabDomValue::ConstMemberIterator patchEntryIterator = patchEntry.FindMember("path");
                    if (patchEntryIterator == patchEntry.MemberEnd())
                    {
                        continue;
                    }
                
                    AZStd::string_view patchPath = patchEntryIterator->value.GetString();

                    // Entities
                    if (patchPath == PathMatchingEntities) // Path is /Entities
                    {
                        patchesMetadata.m_clearAndLoadAllEntities = true;
                    }
                    else if (patchPath.starts_with(PathStartingWithEntities)) // Path begins with /Entities/
                    {
                        if (patchesMetadata.m_clearAndLoadAllEntities)
                        {
                            continue;
                        }

                        patchPath.remove_prefix(strlen(PathStartingWithEntities));
                        AZStd::size_t pathSeparatorIndex = patchPath.find('/');
                        if (pathSeparatorIndex != AZStd::string::npos) // Path begins with /Entities/{someString}/
                        {
                            patchesMetadata.m_entitiesToReload.emplace(patchPath.substr(0, pathSeparatorIndex));
                        }
                        else // Path is with /Entities/{someString}
                        {
                            Internal::IdentifyInstanceMembersToAddAndRemove(
                                patchEntry, patchesMetadata.m_entitiesToReload, patchesMetadata.m_entitiesToRemove, AZStd::move(patchPath));
                        }
                    }
                    // Instances
                    else if (patchPath == PathMatchingInstances) // Path is /Instances
                    {
                        patchesMetadata.m_clearAndLoadAllInstances = true;
                    }
                    else if (patchPath.starts_with(PathStartingWithInstances))// Path begins with /Instances/
                    {
                        if (patchesMetadata.m_clearAndLoadAllInstances)
                        {
                            continue;
                        }

                        patchPath.remove_prefix(strlen(PathStartingWithInstances));
                        AZStd::size_t pathSeparatorIndex = patchPath.find('/');
                        if (pathSeparatorIndex != AZStd::string::npos) // Path begins with /Instances/{someString}/
                        {
                            patchesMetadata.m_instancesToReload.emplace(patchPath.substr(0, pathSeparatorIndex));
                        }
                        else // Path is /Instances/{someString}
                        {
                            Internal::IdentifyInstanceMembersToAddAndRemove(
                                patchEntry, patchesMetadata.m_instancesToAdd, patchesMetadata.m_instancesToRemove, AZStd::move(patchPath));
                        }
                    }
                    // ContainerEntity
                    else if (patchPath.starts_with(PathMatchingContainerEntity)) // Path begins with /ContainerEntity
                    {
                        patchesMetadata.m_shouldReloadContainerEntity = true;
                    }
                    // LinkId
                    else if (patchPath == PathMatchingLinkId) // Path is /LinkId
                    {
                        continue;
                    }
                    else
                    {
                        AZ_Warning(
                            "Prefab",
                            false,
                            "A patch targeting '%.*s' is identified. Patches must be routed to Entities, Instances, "
                            "ContainerEntity, or LinkId.",
                            AZ_STRING_ARG(patchPath));
                    }
                }

                return AZStd::move(patchesMetadata);
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

            AZStd::string PrefabDomValueToString(const PrefabDomValue& prefabDomValue)
            {
                rapidjson::StringBuffer prefabBuffer;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabBuffer);
                prefabDomValue.Accept(writer);

                return prefabBuffer.GetString();
            }

        } // namespace PrefabDomUtils
    } // namespace Prefab
} // namespace AzToolsFramework
