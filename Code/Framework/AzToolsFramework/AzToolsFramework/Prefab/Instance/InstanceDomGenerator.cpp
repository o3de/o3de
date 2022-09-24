/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceDomGenerator.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AzFramework::EntityContextId InstanceDomGenerator::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        void InstanceDomGenerator::RegisterInstanceDomGeneratorInterface()
        {
            AZ::Interface<InstanceDomGeneratorInterface>::Register(this);

            EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface, "Prefab - InstanceDomGenerator::RegisterInstanceDomGeneratorInterface - "
                "InstanceEntityMapperInterface could not be found.");

            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "Prefab - InstanceDomGenerator::RegisterInstanceDomGeneratorInterface - "
                "InstanceToTemplateInterface could not be found.");

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "Prefab - InstanceDomGenerator::RegisterInstanceDomGeneratorInterface - "
                "PrefabSystemComponentInterface could not be found.");
        }
        
        void InstanceDomGenerator::UnregisterInstanceDomGeneratorInterface()
        {
            m_prefabSystemComponentInterface = nullptr;
            m_instanceToTemplateInterface = nullptr;
            m_instanceEntityMapperInterface = nullptr;

            s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

            AZ::Interface<InstanceDomGeneratorInterface>::Unregister(this);
        }

        bool InstanceDomGenerator::GenerateInstanceDom(PrefabDom& instanceDom, const Instance& instance) const
        {
            // Retrieves the focused instance.
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (!prefabFocusInterface)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "PrefabFocusInterface could not be found.");
                return false;
            }

            InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);
            if (!focusedInstance.has_value())
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Could not get the focused instance. It should not be null.");
                return false;
            }

            // Climbs up from the given instance to root instance, but stops at the focused instance if they can meet.
            const InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance, &(focusedInstance->get()));
            const Instance* focusedOrRootInstance = climbUpResult.m_reachedInstance;
            if (!focusedOrRootInstance)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Could not get the focused or root instance. It should not be null.");
                return false;
            }

            // Copies the instance DOM, that is stored in the focused or root template DOM, into the output instance DOM.
            AZStd::string relativePathFromTop = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);
            PrefabDomPath relativeDomPath(relativePathFromTop.c_str());

            // IMPROVEMENT: Copy the nested instance DOM instead of the whole template DOM.
            {
                // only valid in this scope.
                const PrefabDom& focusedOrRootTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(
                    focusedOrRootInstance->GetTemplateId());
                const PrefabDomValue* instanceDomFromTemplate = relativeDomPath.Get(focusedOrRootTemplateDom);
                if (!instanceDomFromTemplate)
                {
                    AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                        "Could not get the instance DOM stored in the focused or root template DOM.");
                    return false;
                }

                // Directly copy into the output DOM.
                instanceDom.CopyFrom(*instanceDomFromTemplate, instanceDom.GetAllocator());
            }

            // Additional processing on focused instance DOM...

            // If the focused instance is the given instance, then update the container entity in focused instance DOM
            // (i.e. the given instance DOM) with the one seen by the root.
            if (&instance == &(focusedInstance->get()))
            {
                UpdateContainerEntityInDomFromRoot(instanceDom, focusedInstance->get());
            }
            // If the focused instance is a descendant of the given instance, then update the corresponding
            // portion in the output instance DOM with the focused template DOM. Also, updates the container entity
            // in focused instance DOM with the one seen by the root.
            else if (PrefabInstanceUtils::IsDescendantInstance(focusedInstance->get(), instance))
            {
                PrefabDom focusedTemplateDom;
                focusedTemplateDom.CopyFrom(m_prefabSystemComponentInterface->FindTemplateDom(focusedInstance->get().GetTemplateId()),
                    focusedTemplateDom.GetAllocator());

                UpdateContainerEntityInDomFromRoot(focusedTemplateDom, focusedInstance->get());

                // REMOVED: Forces a hard copy using the instanceDom's allocator.
                // PrefabDom focusedTemplateDomCopy;
                // focusedTemplateDomCopy.CopyFrom(focusedTemplateDom, instanceDom.GetAllocator());

                // Stores the focused DOM into the instance DOM.
                AZStd::string relativePathToFocus = PrefabInstanceUtils::GetRelativePathBetweenInstances(instance, focusedInstance->get());
                PrefabDomPath relativeDomPathToFocus(relativePathToFocus.c_str());
                relativeDomPathToFocus.Set(instanceDom, PrefabDomValue(focusedTemplateDom, instanceDom.GetAllocator()).Move()); // make a copy

                // IMPROVEMENT: NO, because we don't reduce # of copying.
            }
            // Skips additional processing if the focused instance is a proper ancestor of the given instance, or
            // the focused instance has no hierarchy relation with the given instance.

            // See [Other flow] in GenerateEntityDom

            return true;
        }

        bool InstanceDomGenerator::GenerateEntityDom(PrefabDom& entityDom, const AZ::Entity& entity) const
        {
            // Retrieves the focused instance.
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (!prefabFocusInterface)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateEntityDom - "
                    "PrefabFocusInterface could not be found.");
                return false;
            }

            InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);
            if (!focusedInstance.has_value())
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateEntityDom - "
                    "Could not get the focused instance. It should not be null.");
                return false;
            }

            const AZ::EntityId entityId = entity.GetId();
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance.has_value())
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateEntityDom - "
                    "Could not get the owning instance for the given entity id.");
                return false;
            }

            // Climbs up from the owning instance to root instance, but stops at the focused instance if they can meet.
            const InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(
                owningInstance->get(), &(focusedInstance->get()));
            const Instance* focusedOrRootInstance = climbUpResult.m_reachedInstance;
            if (!focusedOrRootInstance)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateEntityDom - "
                    "Could not get the focused or root instance from climb-up. It should not be null.");
                return false;
            }

            // Prepares the path from the focused (or root) instance to the entity.
            AZStd::string relativePathFromTop = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);
            relativePathFromTop.append(m_instanceToTemplateInterface->GenerateEntityAliasPath(entityId));
            PrefabDomPath relativeDomPathFromTopToEntity(relativePathFromTop.c_str());

            // Retrieves the entity DOM from the template of the focused or root instance.
            // NOTE 1: Can be improved by only copying the entity Dom. But we need an overloaded UpdateContainerEntityInDomFromRoot
            //   for entity DOM and template info as input.
            // NOTE 2: PrefabDom focusedOrRootTemplateDomCopy(&entityDom.GetAllocator()) does not seem to be a good idea. From rapidjson code,
            //   when it goes out of scope, it destroys the allocator.
            // NOTE 3: Eventually, the whole template DOM copy can be avoided if we later cache container entity DOM in focus handler
            //   as suggested by Ram earlier in PR.
            PrefabDom focusedOrRootTemplateDomCopy;
            focusedOrRootTemplateDomCopy.CopyFrom(
                m_prefabSystemComponentInterface->FindTemplateDom(focusedOrRootInstance->GetTemplateId()),
                focusedOrRootTemplateDomCopy.GetAllocator());

            // If it is the focused container entity, then replace the entity DOM's transform data (from template)
            // with the one seen from root.
            if (entityId == focusedInstance->get().GetContainerEntityId())
            {
                UpdateContainerEntityInDomFromRoot(focusedOrRootTemplateDomCopy, focusedInstance->get());
            }

            PrefabDomValue* entityDomFromTemplateCopy = relativeDomPathFromTopToEntity.Get(focusedOrRootTemplateDomCopy);
            if (!entityDomFromTemplateCopy)
            {
                AZ_Warning("Prefab", false, "InstanceDomGenerator::GenerateEntityDom - "
                    "The entity DOM cannot be found in the template DOM. Returns false.");
                return false;
            }

            entityDom.CopyFrom(*entityDomFromTemplateCopy, entityDom.GetAllocator());

            // [Other flow]
            //   1. Defines PrefabDom outputEntityDom
            //   2. Uses outputEntityDom.GetAllocator() throughout this function
            //   3. Calls entityDom.Swap(outputEntityDom) at last OR entityDom = AZStd::move(outputEntityDom);
            // Why good? All memory is allocated by one allocator.
            // Why bad?  If we do something on entityDom before calling this function, the changes will be lost.
            // Ref: https://github.com/Tencent/rapidjson/issues/285

            return true;
        }

        void InstanceDomGenerator::UpdateContainerEntityInDomFromRoot(PrefabDom& instanceDom, const Instance& instance) const
        {
            // TODO: Modifies the function so it updates the transform only.

            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance, nullptr);
            const Instance* rootInstancePtr = climbUpResult.m_reachedInstance;
            AZStd::string relativePathFromRoot = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);

            // No need to update the instance DOM if the given instance is the root instance.
            if (rootInstancePtr == &instance)
            {
                return;
            }

            // Creates a path from the root instance to container entity of the given insance.
            const AZStd::string pathToContainerEntity =
                AZStd::string::format("%s/%s", relativePathFromRoot.c_str(), PrefabDomUtils::ContainerEntityName);
            PrefabDomPath domPathToContainerEntity(pathToContainerEntity.c_str());

            // Retrieves the container entity DOM of the root template DOM.
            {
                // only valid in this scope
                const PrefabDom& rootTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom((*rootInstancePtr).GetTemplateId());
                const PrefabDomValue* containerEntityDom = domPathToContainerEntity.Get(rootTemplateDom);

                if (containerEntityDom)
                {
                    // IMPROVEMENT: AVOIDED A COPY

                    // Sets the container entity DOM in the given instance DOM.
                    const AZStd::string containerEntityName = AZStd::string::format("/%s", PrefabDomUtils::ContainerEntityName);
                    PrefabDomPath containerEntityPath(containerEntityName.c_str());

                    // Makes a copy using the output allocator
                    containerEntityPath.Set(instanceDom,
                        PrefabDomValue(*containerEntityDom, instanceDom.GetAllocator()).Move());
                }
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
