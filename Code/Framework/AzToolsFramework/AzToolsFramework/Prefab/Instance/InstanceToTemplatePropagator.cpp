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

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplatePropagator.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void InstanceToTemplatePropagator::RegisterInstanceToTemplateInterface()
        {
            AZ::Interface<InstanceToTemplateInterface>::Register(this);

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
        }

        void InstanceToTemplatePropagator::UnregisterInstanceToTemplateInterface()
        {
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

        bool InstanceToTemplatePropagator::GeneratePatch(PrefabDom& generatedPatch, const PrefabDom& initialState,
            const PrefabDom& modifiedState)
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
            
            Link& link = findLinkResult->get();

            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::CreatePatch(generatedPatch,
                link.GetLinkDom().GetAllocator(), initialState, modifiedState, AZ::JsonMergeApproach::JsonPatch);

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

        void InstanceToTemplatePropagator::AppendEntityAliasToPatchPaths(PrefabDom& providedPatch, const AZ::EntityId& entityId)
        {
            if (!providedPatch.IsArray())
            {
                AZ_Error("Prefab", false, "Patch is not an array of updates.  Update failed.");
                return;
            }

            //create the prefix for the update - choosing between container and regular entities
            AZStd::string prefix = "/";

            //grab the owning instance so we can use the entityIdMapper in settings
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            AZ_Assert(owningInstance != AZStd::nullopt, "Owning Instance is null");

            bool isContainerEntity = entityId == owningInstance->get().GetContainerEntityId();

            if (isContainerEntity)
            {
                prefix += PrefabDomUtils::ContainerEntityName;
            }
            else
            {
                EntityAliasOptionalReference entityAliasRef = owningInstance->get().GetEntityAlias(entityId);
                prefix += PrefabDomUtils::EntitiesName;
                prefix += "/";
                prefix += entityAliasRef->get();
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

                AZStd::string path = prefix + pathIter->value.GetString();

                pathIter->value.SetString(path.c_str(), static_cast<rapidjson::SizeType>(path.length()), providedPatch.GetAllocator());
            }
        }

        bool InstanceToTemplatePropagator::PatchTemplate(PrefabDomValue& providedPatch, TemplateId templateId, bool immediate, InstanceOptionalReference instanceToExclude)
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
                AZ_Error(
                    "Prefab",
                    (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::Skipped) &&
                    (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::PartialSkip),
                    "Some of the patches were not successfully applied.");
                m_prefabSystemComponentInterface->SetTemplateDirtyFlag(templateId, true);
                m_prefabSystemComponentInterface->PropagateTemplateChanges(templateId, immediate, instanceToExclude);
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
                patchPathReference->get().SetString(patchPathString.c_str(), linkToApplyPatches.GetLinkDom().GetAllocator());
            }

            AddPatchesToLink(patches, linkToApplyPatches);
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

        void InstanceToTemplatePropagator::AddPatchesToLink(const PrefabDom& patches, Link& link)
        {
            PrefabDom& linkDom = link.GetLinkDom();

            /*
            If the original allocator the patches were created with gets destroyed, then the patches would become garbage in the
            linkDom. Since we cannot guarantee the lifecycle of the patch allocators, we are doing a copy of the patches here to
            associate them with the linkDom's allocator.
            */
            PrefabDom patchesCopy;
            patchesCopy.CopyFrom(patches, linkDom.GetAllocator());
            linkDom.AddMember(rapidjson::StringRef(PrefabDomUtils::PatchesName), patchesCopy, linkDom.GetAllocator());
        }
    }
}
