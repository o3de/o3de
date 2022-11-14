/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Component/Entity.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplatePropagator.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AzFramework::EntityContextId InstanceToTemplatePropagator::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        void InstanceToTemplatePropagator::RegisterInstanceToTemplateInterface()
        {
            AZ::Interface<InstanceToTemplateInterface>::Register(this);

            EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            //get instance id associated with entityId
            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface,
                "Prefab - InstanceToTemplateInterface - "
                "Instance Entity Mapper Interface could not be found. "
                "Check that it is being correctly initialized.");

            //use system component to grab template dom
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface,
                "Prefab - InstanceToTemplateInterface - "
                "Prefab System Component Interface could not be found. "
                "Check that it is being correctly initialized.");

            m_instanceDomGenerator.RegisterInstanceDomGeneratorInterface();
        }

        void InstanceToTemplatePropagator::UnregisterInstanceToTemplateInterface()
        {
            m_instanceDomGenerator.UnregisterInstanceDomGeneratorInterface();

            AZ::Interface<InstanceToTemplateInterface>::Unregister(this);
        }

        bool InstanceToTemplatePropagator::GenerateDomForEntity(PrefabDom& generatedEntityDom, const AZ::Entity& entity)
        {
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entity.GetId());

            if (!owningInstance)
            {
                AZ_Error("Prefab", false, "Entity does not belong to an instance");
                return false;
            }

            return PrefabDomUtils::StoreEntityInPrefabDomFormat(entity, owningInstance->get(), generatedEntityDom);
        }

        bool InstanceToTemplatePropagator::GenerateDomForInstance(PrefabDom& generatedInstanceDom, const Prefab::Instance& instance)
        {
            return PrefabDomUtils::StoreInstanceInPrefabDom(instance, generatedInstanceDom);
        }

        bool InstanceToTemplatePropagator::GeneratePatch(
            PrefabDom& generatedPatch, const PrefabDomValue& initialState, const PrefabDomValue& modifiedState)
        {
            //generate patch using json serialization CreatePatch
            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::CreatePatch(generatedPatch,
                generatedPatch.GetAllocator(), initialState, modifiedState, AZ::JsonMergeApproach::JsonPatch);

            return result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted;
        }

        bool InstanceToTemplatePropagator::GeneratePatchForLink(PrefabDom& generatedPatch, const PrefabDom& initialState,
            const PrefabDom& modifiedState, LinkId linkId)
        {
            AZStd::optional<AZStd::reference_wrapper<Link>> findLinkResult = m_prefabSystemComponentInterface->FindLink(linkId);

            if (!findLinkResult.has_value())
            {
                AZ_Assert(false, "Link with id %llu couldn't be found. Patch cannot be generated.", linkId);
                return false;
            }

            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::CreatePatch(
                generatedPatch, generatedPatch.GetAllocator(), initialState, modifiedState, AZ::JsonMergeApproach::JsonPatch);

            return result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted;
        }

        bool InstanceToTemplatePropagator::PatchEntityInTemplate(PrefabDom& providedPatch, AZ::EntityId entityId)
        {
            InstanceOptionalReference instanceOptionalReference = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

            if (!instanceOptionalReference)
            {
                AZ_Error(
                    "Prefab", false, "Failed to find an owning instance for the entity with id %llu.",
                    static_cast<AZ::u64>(entityId));
                return false;
            }

            //get template id associated with instance
            TemplateId templateId = instanceOptionalReference->get().GetTemplateId();
            AppendEntityAliasToPatchPaths(providedPatch, entityId);
            return PatchTemplate(providedPatch, templateId);
        }

        AZStd::string InstanceToTemplatePropagator::GenerateEntityAliasPath(AZ::EntityId entityId)
        {
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance.has_value())
            {
                AZ_Error("Prefab", false, "Failed to find an owning instance for entity with id %llu.", static_cast<AZ::u64>(entityId));
                return AZStd::string();
            }

            // create the prefix for the update - choosing between container and regular entities
            AZStd::string entityAliasPath = "/";
            const bool isContainerEntity = entityId == owningInstance->get().GetContainerEntityId();
            if (isContainerEntity)
            {
                entityAliasPath += PrefabDomUtils::ContainerEntityName;
            }
            else
            {
                EntityAliasOptionalReference entityAliasRef = owningInstance->get().GetEntityAlias(entityId);
                entityAliasPath += PrefabDomUtils::EntitiesName;
                entityAliasPath += "/";
                entityAliasPath += entityAliasRef->get();
            }
            return AZStd::move(entityAliasPath);
        }

        AZ::Dom::Path InstanceToTemplatePropagator::GenerateEntityPathFromFocusedPrefab(AZ::EntityId entityId)
        {
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance.has_value())
            {
                AZ_Error("Prefab", false, "Failed to find an owning instance for entity with id %llu.", static_cast<AZ::u64>(entityId));
                return AZ::Dom::Path();
            }

            auto* prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (prefabFocusInterface == nullptr)
            {
                AZ_Error("Prefab", false, "Cannot find PrefabFocusInterface.");
                return AZ::Dom::Path();
            }

            auto focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);

            if (!focusedInstance.has_value())
            {
                AZ_Error("Prefab", false, "Focused prefab instance is null.");
                return AZ::Dom::Path();
            }
            
            auto relativePathBetweenInstances =
                PrefabInstanceUtils::GetRelativePathBetweenInstances(focusedInstance->get(), owningInstance->get());

            AZ::Dom::Path fullPathBetweenInstances(relativePathBetweenInstances);
            AZStd::string entityPath = GenerateEntityAliasPath(entityId);
            fullPathBetweenInstances = fullPathBetweenInstances / AZ::Dom::Path(entityPath);
            return AZStd::move(fullPathBetweenInstances);
        }

        void InstanceToTemplatePropagator::AppendEntityAliasToPatchPaths(PrefabDom& providedPatch, AZ::EntityId entityId, const AZStd::string& prefix)
        {
            AppendEntityAliasPathToPatchPaths(providedPatch, prefix + GenerateEntityAliasPath(entityId));
        }

        void InstanceToTemplatePropagator::AppendEntityAliasPathToPatchPaths(PrefabDom& providedPatch, const AZStd::string& entityAliasPath)
        {
            if (!providedPatch.IsArray())
            {
                AZ_Error("Prefab", false, "Patch is not an array of updates.  Update failed.");
                return;
            }

            if (entityAliasPath.empty())
            {
                return;
            }

            //update all entities or just the single container
            for (auto& patchValue : providedPatch.GetArray())
            {
                auto pathIter = patchValue.FindMember("path");

                if (pathIter == patchValue.MemberEnd() || !(pathIter->value.IsString()))
                {
                    AZ_Error("Prefab", false, "Was not able to find path member within patch dom. "
                        "A non prefab dom patch may have been passed in.");
                    continue;
                }

                AZStd::string path = entityAliasPath + pathIter->value.GetString();

                pathIter->value.SetString(path.c_str(), static_cast<rapidjson::SizeType>(path.length()), providedPatch.GetAllocator());
            }
        }

        bool InstanceToTemplatePropagator::PatchTemplate(PrefabDomValue& providedPatch, TemplateId templateId, InstanceOptionalConstReference instanceToExclude)
        {
            PrefabDom& templateDomReference = m_prefabSystemComponentInterface->FindTemplateDom(templateId);

            //apply patch to template
            AZ::JsonSerializationResult::ResultCode result =
                PrefabDomUtils::ApplyPatches(templateDomReference, templateDomReference.GetAllocator(), providedPatch);

            //trigger propagation
            if (result.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
            {
                AZ_Error("Prefab", false, "Patch was not successfully applied.");
                return false; 
            }
            else
            {
                AZ_Warning(
                    "Prefab",
                    (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::Skipped) &&
                    (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::PartialSkip),
                    "Some of the patches were not successfully applied.");
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(templateId, true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(templateId, instanceToExclude);
                return true;
            }
        }

        void InstanceToTemplatePropagator::ApplyPatchesToInstance(const AZ::EntityId& entityId, PrefabDom& patches,
            const Instance& instanceToAddPatches)
        {
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance)
            {
                AZ_Error("Prefab", false, "Entity does not belong to an instance.");
                return;
            }

            EntityAliasOptionalReference entityAliasOptionalReference = owningInstance->get().GetEntityAlias(entityId);
            if (!entityAliasOptionalReference)
            {
                AZ_Error("Prefab", false, "Entity alias for entity with id %llu cannot be found.", static_cast<AZ::u64>(entityId));
                return;
            }

            // We need to generate a patch prefix so that the path of the patches correctly refelects the hierarchy path
            // from the entity to the instance where the patch needs to be added.
            AZStd::string patchPrefix;
            patchPrefix.insert(0, "/Entities/" + entityAliasOptionalReference->get());
            while (!owningInstance->get().IsParentInstance(instanceToAddPatches))
            {
                patchPrefix.insert(0, "/Instances/" + owningInstance->get().GetInstanceAlias());
                owningInstance = owningInstance->get().GetParentInstance();
                if (!owningInstance.has_value())
                {
                    AZ_Error("Prefab", false, "The Entity couldn't be patched because the instance "
                        "to which the patches must be applied couldn't be found in the hierarchy of the entity");
                    return;
                }
            }
            LinkId linkIdToAddPatches = owningInstance->get().GetLinkId();

            auto findLinkResult = m_prefabSystemComponentInterface->FindLink(linkIdToAddPatches);
            if (!findLinkResult.has_value())
            {
                AZ_Error("Prefab", false,
                    "A valid link corresponding to the instance couldn't be found. Patches won't be applied.");
                return;
            }
            Link& linkToApplyPatches = findLinkResult->get();

            for (auto patchIterator = patches.Begin(); patchIterator != patches.End(); ++patchIterator)
            {
                PrefabDomValueReference patchPathReference = PrefabDomUtils::FindPrefabDomValue(*patchIterator, "path");
                AZStd::string patchPathString = patchPathReference->get().GetString();
                patchPathString.insert(0, patchPrefix);
                patchPathReference->get().SetString(patchPathString.c_str(), patches.GetAllocator());
            }

            linkToApplyPatches.AddPatchesToLink(patches);
            linkToApplyPatches.UpdateTarget();

            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(linkToApplyPatches.GetTargetTemplateId(), true);
            m_prefabSystemComponentInterface->PropagateTemplateChanges(linkToApplyPatches.GetTargetTemplateId());
        }

        InstanceOptionalReference InstanceToTemplatePropagator::GetTopMostInstanceInHierarchy(AZ::EntityId entityId)
        {
            InstanceOptionalReference parentInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

            if (!parentInstance)
            {
                AZ_Error("Prefab", false, "Entity does not belong to an instance");
                return AZStd::nullopt;
            }

            while (parentInstance->get().GetParentInstance())
            {
                parentInstance = parentInstance->get().GetParentInstance();
            }

            return parentInstance;
        }
    }
}
