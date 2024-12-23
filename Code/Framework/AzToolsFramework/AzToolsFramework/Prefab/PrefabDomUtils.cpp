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
#include <AzCore/Component/EntitySerializer.h>

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
                [[maybe_unused]] static constexpr const char* const ComponentRemovalNotice =
                    "[INFORMATION] %s data has been altered to remove component '%s'. "
                    "Please edit and save %s to persist the change.";

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

                    // Returns whether to track deprecated components for the instance being deserialized
                    auto shouldTrackDeprecated = [&instance, &entityIdMapper]()
                    {
                        // Only track the instance that was passed in
                        return (&instance == entityIdMapper.GetLoadingInstance());
                    }; 

                    if ((flags & LoadFlags::ReportDeprecatedComponents) == LoadFlags::ReportDeprecatedComponents)
                    {
                        // Add metadata to track components that are deprecated
                        auto deprecationTracker = AZ::DeprecatedComponentMetadata{};
                        deprecationTracker.SetEnableDeprecationTrackingCallback(shouldTrackDeprecated);
                        settings.m_metadata.Add(deprecationTracker);
                    }

                    AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(instance, prefabDom, settings);
                    bool succeeded = (result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted);
#ifdef AZ_ENABLE_TRACING
                    if (succeeded)
                    {
                        // Display a message for skipped components
                        auto deprecatedComponents = settings.m_metadata.Find<AZ::DeprecatedComponentMetadata>();
                        if (deprecatedComponents)
                        {
                            const char* prefabName = instance.GetTemplateSourcePath().Filename().Native().data();
                            AZStd::vector<AZStd::string> componentNames = deprecatedComponents->GetComponentNames();
                            for (const auto& componentName : componentNames)
                            {
                                AZ_Warning(
                                    "JSON Serialization",
                                    false,
                                    Internal::ComponentRemovalNotice,
                                    prefabName,
                                    componentName.c_str(),
                                    instance.GetTemplateSourcePath().c_str());
                            }
                        }
                    }
                    else
                    {
                        AZ_Error(
                            "Prefab",
                            false,
                            "Failed to de-serialize Prefab Instance from Prefab DOM. "
                            "Unable to proceed.");
                    }
#endif // AZ_ENABLE_TRACING

                    AZ::Data::AssetManager::Instance().ResumeAssetRelease();

                    return succeeded;
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

                // Set a custom Json reporting handler to only report "Processing::Halted" warnings
                // and skip any "Processing::Altered" warnings which end up spamming the logs when
                // a component has been deprecated. This aligns with the same behavior as saving and
                // instantiating a prefab
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
                    // Source
                    else if (patchPath == PathMatchingSource)
                    {
                        // Ignore nested instance file path changes resulting from source asset renaming
                        continue;
                    }
                    else
                    {
                        AZ_Warning(
                            "Prefab",
                            false,
                            "A patch targeting '%.*s' is identified. Patches must be routed to Entities, Instances, "
                            "ContainerEntity, LinkId or Source path.",
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

            void AddNestedInstance(
                PrefabDom& prefabDom,
                const InstanceAlias& nestedInstanceAlias,
                PrefabDomValueConstReference nestedInstanceDom)
            {
                auto instancesMember = prefabDom.FindMember(PrefabDomUtils::InstancesName);
                if (instancesMember == prefabDom.MemberEnd())
                {
                    // Add an "Instances" member in the prefab DOM
                    prefabDom.AddMember(
                        rapidjson::StringRef(PrefabDomUtils::InstancesName), PrefabDomValue(), prefabDom.GetAllocator());
                    instancesMember = prefabDom.FindMember(PrefabDomUtils::InstancesName);
                }

                PrefabDomValueReference instancesValue = instancesMember->value;
                if (!instancesValue->get().IsObject())
                {
                    instancesValue->get().SetObject();
                }

                auto instanceMember = instancesValue->get().FindMember(rapidjson::StringRef(nestedInstanceAlias.c_str()));
                if (instanceMember == instancesValue->get().MemberEnd())
                {
                    // Add a member for the nested instance in the prefab DOM
                    instancesValue->get().AddMember(
                        rapidjson::Value(nestedInstanceAlias.c_str(), prefabDom.GetAllocator()),
                        nestedInstanceDom.has_value() ? rapidjson::Value(nestedInstanceDom->get(), prefabDom.GetAllocator())
                                                      : PrefabDomValue(),
                        prefabDom.GetAllocator());
                }
                else
                {
                    AZ_Assert(
                        false,
                        "PrefabDomUtils::AddNestedInstance - Nested instance already exists in prefab DOM.");
                }
            }

            bool SubstituteInvalidParentsInEntities(PrefabDom& templateDomRef)
            {
                // Search for a TransformComponent and check the "Parent Entity" statement
                auto sourceIt = templateDomRef.FindMember(PrefabDomUtils::SourceName);
                if (sourceIt == templateDomRef.MemberEnd() || !sourceIt->value.IsString())
                {
                    AZ_Warning("Prefab", false, "lacks a 'Source' container.");
                    return false;
                }
                const AZStd::string path(sourceIt->value.GetString());

                const auto containerEntityIt = templateDomRef.FindMember(PrefabDomUtils::ContainerEntityName);
                if (containerEntityIt == templateDomRef.MemberEnd() || !containerEntityIt->value.IsObject())
                {
                    AZ_Warning("Prefab", false, "'%s' lacks a 'ContainerEntity' container.", path.c_str());
                    return false;
                }
                const auto containerEntityAliasIt = containerEntityIt->value.FindMember(PrefabDomUtils::EntityIdName);
                if (containerEntityAliasIt == containerEntityIt->value.MemberEnd() || !containerEntityAliasIt->value.IsString())
                {
                    AZ_Error("Prefab", false, "'%s' lacks 'Id' object for ContainerEntity.", path.c_str());
                    return false;
                }
                const AZStd::string containerEntityAlias(containerEntityAliasIt->value.GetString());

                const auto entitiesIt = templateDomRef.FindMember(PrefabDomUtils::EntitiesName);
                if (entitiesIt == templateDomRef.MemberEnd() || !entitiesIt->value.IsObject())
                {
                    AZ_Error("Prefab", false, "'%s' lacks 'Entities' container.", path.c_str());
                    return false;
                }

                bool result = true;
                // Search through entities for their TransformComponents
                for (auto entityIt = entitiesIt->value.MemberBegin(); entityIt != entitiesIt->value.MemberEnd(); ++entityIt)
                {
                    // More information for logs
                    constexpr const auto unknownName = "[unknown]";
                    AZStd::string entityAlias = unknownName;
                    const auto entityAliasIt = entityIt->value.FindMember(PrefabDomUtils::EntityIdName);
                    if (entityAliasIt != entityIt->value.MemberEnd() && entityAliasIt->value.IsString())
                    {
                        entityAlias = entityAliasIt->value.GetString();
                    }
                    AZStd::string entityName = unknownName;
                    const auto entityNameIt = entityIt->value.FindMember("Name");
                    if (entityNameIt != entityIt->value.MemberEnd() && entityNameIt->value.IsString())
                    {
                        entityName = entityNameIt->value.GetString();
                    }

                    const auto componentsIt = entityIt->value.FindMember(PrefabDomUtils::ComponentsName);
                    if (componentsIt == entityIt->value.MemberEnd() || !componentsIt->value.IsObject())
                    {
                        result = false;
                        AZ_Error("Prefab", false, "'%s' lacks 'Components' container in Entity (%s, '%s').",
                            path.c_str(), entityAlias.c_str(), entityName.c_str());
                        continue;
                    }

                    // Search current entity object for TransformComponent looping through components
                    for (auto componentIt = componentsIt->value.MemberBegin(); componentIt != componentsIt->value.MemberEnd();
                         ++componentIt)
                    {
                        const auto componentsTypeIt = componentIt->value.FindMember(PrefabDomUtils::TypeName);
                        if (componentsTypeIt == componentIt->value.MemberEnd() || !componentsTypeIt->value.IsString())
                        {
                            result = false;
                            AZStd::string componentName = componentIt->name.GetString();
                            AZ_Error("Prefab", false, "'%s' lacks '$type' sub-object in a component object of Entity (%s, '%s').",
                                path.c_str(), entityAlias.c_str(), entityName.c_str());
                            continue;
                        }
                        
                        AZStd::string componentAlias(componentsTypeIt->value.GetString());
                        if (!componentAlias.contains("TransformComponent"))
                        {
                            continue;
                        }
                        
                        constexpr const auto parentObjectName = "Parent Entity";
                        auto parentIt = componentIt->value.FindMember(parentObjectName);
                        if (parentIt == componentIt->value.MemberEnd() || !parentIt->value.IsString())
                        {
                            result = false;
                            AZ_Error("Prefab", false, "'%s' lacks 'Parent Entity' object in TransformComponent of Entity (%s, '%s').",
                                path.c_str(), entityAlias.c_str(), entityName.c_str());
                            break; // one TransformComponent for an Entity -> invalid
                        }

                        if (AZStd::string(parentIt->value.GetString()).empty()) // parent link empty ?
                        {
                            // Re-link this entity under the ContainerEntity using its Alias
                            parentIt->value.SetString(rapidjson::StringRef(containerEntityAlias.c_str()), templateDomRef.GetAllocator());

                            result = false; // report that changes were needed and made 
                            AZ_Error("Prefab", false,
                                "'%s' lacks 'Parent Entity' value in TransformComponent of Entity (%s, '%s')"
                                "\nEntity re-linked under the root container '%s'.",
                                path.c_str(), entityAlias.c_str(), entityName.c_str(), containerEntityAlias.c_str());
                        }
                        break; // single TransformComponent for an Entity -> evaluated
                    }
                }
                return result;
            }
        } // namespace PrefabDomUtils
    } // namespace Prefab
} // namespace AzToolsFramework
