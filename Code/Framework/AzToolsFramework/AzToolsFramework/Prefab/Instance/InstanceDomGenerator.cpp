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

        void InstanceDomGenerator::GetInstanceDomFromTemplate(PrefabDom& instanceDom, const Instance& instance) const
        {
            AZ_Assert(instanceDom.IsNull(), "GetInstanceDomFromTemplate must be called with an empty instance to generate into.");

            // Retrieves the focused instance.
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (!prefabFocusInterface)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GetInstanceDomFromTemplate - "
                    "PrefabFocusInterface could not be found.");
                return;
            }

            InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);
            if (!focusedInstance.has_value())
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GetInstanceDomFromTemplate - "
                    "Could not get the focused instance. It should not be null.");
                return;
            }

            // Climbs up from the given instance to root instance, but stops at the focused instance if they can meet.
            const InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance, &(focusedInstance->get()));
            const Instance* focusedOrRootInstance = climbUpResult.m_reachedInstance;
            if (!focusedOrRootInstance)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GetInstanceDomFromTemplate - "
                    "Could not get the focused or root instance. It should not be null.");
                return;
            }

            // Copies the instance DOM, that is stored in the focused or root template DOM, into the output instance DOM.
            AZStd::string relativePathFromTop = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);
            PrefabDomPath relativeDomPath(relativePathFromTop.c_str());
            const PrefabDom& sourceDom = m_prefabSystemComponentInterface->FindTemplateDom(focusedOrRootInstance->GetTemplateId());
            const PrefabDomValue* instanceDomFromTemplate = relativeDomPath.Get(sourceDom);
            if (!instanceDomFromTemplate)
            {
                AZ_Assert(
                    false,
                    "Prefab - InstanceDomGenerator::GetInstanceDomFromTemplate - "
                    "Could not get the instance DOM stored in the focused or root template DOM.");
                return;
            }

            instanceDom.CopyFrom(*instanceDomFromTemplate, instanceDom.GetAllocator());

            // Additional processing on focused instance DOM...

            // If the focused instance is the given instance, then update the container entity in focused instance DOM
            // (i.e. the given instance DOM) with the one seen by the root.
            if (&instance == &(focusedInstance->get()))
            {
                UpdateContainerEntityInDomFromHighestAncestor(instanceDom, focusedInstance->get());
            }
            // If the focused instance is a descendant of the given instance, then update the corresponding
            // portion in the output instance DOM with the focused template DOM. Also, updates the container entity
            // in focused instance DOM with the one seen by the root.
            else if (PrefabInstanceUtils::IsDescendantInstance(focusedInstance->get(), instance))
            {
                TemplateReference focusedTemplate = m_prefabSystemComponentInterface->FindTemplate(focusedInstance->get().GetTemplateId());

                if (!focusedTemplate.has_value())
                {
                    AZ_Assert(
                        false,
                        "Prefab - InstanceDomGenerator::GetInstanceDomFromTemplate - A focused instance was found but there is no "
                        "corresponding prefab template associated with it.");
                    return;
                }
                
                // use instanceDom's allocator, because ultimately we will be setting this data back into
                // instanceDom with move semantics.
                PrefabDom focusedTemplateDomCopy(&instanceDom.GetAllocator());

                focusedTemplateDomCopy.CopyFrom(focusedTemplate->get().GetPrefabDom(), instanceDom.GetAllocator());

                UpdateContainerEntityInDomFromHighestAncestor(focusedTemplateDomCopy, focusedInstance->get());

                // Stores the focused DOM into the instance DOM.
                AZStd::string relativePathToFocus = PrefabInstanceUtils::GetRelativePathBetweenInstances(instance, focusedInstance->get());
                PrefabDomPath relativeDomPathToFocus(relativePathToFocus.c_str());

                // because focusedTemplateDomCopy is an non-const reference, its memory will be adopted into instanceDom without copying
                relativeDomPathToFocus.Set(instanceDom, focusedTemplateDomCopy);
            }
            // Skips additional processing if the focused instance is a proper ancestor of the given instance, or
            // the focused instance has no hierarchy relation with the given instance.
        }

        void InstanceDomGenerator::GetEntityDomFromTemplate(PrefabDom& entityDom, const AZ::Entity& entity) const
        {
            AZ_Assert(entityDom.IsNull(), "GetEntityDomFromTemplate must be called with an empty entityDom to fill.");
            // Retrieves the focused instance.
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (!prefabFocusInterface)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GetEntityDomFromTemplate - "
                    "PrefabFocusInterface could not be found.");
                return;
            }

            InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);
            if (!focusedInstance.has_value())
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GetEntityDomFromTemplate - "
                    "Could not get the focused instance. It should not be null.");
                return;
            }

            const AZ::EntityId entityId = entity.GetId();
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance.has_value())
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GetEntityDomFromTemplate - "
                    "Could not get the owning instance for the given entity id.");
                return;
            }

            // Climbs up from the owning instance to root instance, but stops at the focused instance if they can meet.
            const InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(
                owningInstance->get(), &(focusedInstance->get()));
            const Instance* focusedOrRootInstance = climbUpResult.m_reachedInstance;
            if (!focusedOrRootInstance)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GetEntityDomFromTemplate - "
                    "Could not get the focused or root instance from climb-up. It should not be null.");
                return;
            }

            // Prepares the path from the focused (or root) instance to the entity.
            AZStd::string relativePathFromTop = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);
            relativePathFromTop.append(m_instanceToTemplateInterface->GenerateEntityAliasPath(entityId));
            PrefabDomPath relativeDomPathFromTopToEntity(relativePathFromTop.c_str());

            // If it is the focused container entity, then replace the entity DOM's transform data (from template)
            // with the one seen from root.
            if (entityId == focusedInstance->get().GetContainerEntityId())
            {
                GenerateContainerEntityDomFromHighestAncestorOrSelf(entityDom, focusedInstance->get());
            }
            else
            {
                // Retrieves the entity DOM from the template of the focused or root instance.
                const PrefabDom& focusedTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom(focusedOrRootInstance->GetTemplateId());
                const PrefabDomValue* entityDomFromTemplate = relativeDomPathFromTopToEntity.Get(focusedTemplateDom);
                if (!entityDomFromTemplate)
                {
                    AZ_Warning(
                        "Prefab",
                        false,
                        "InstanceDomGenerator::GetEntityDomFromTemplate - "
                        "The entity DOM cannot be found in the template DOM. Output DOM will be null.");
                    return;
                }

                entityDom.CopyFrom(*entityDomFromTemplate, entityDom.GetAllocator());
            }
        }

        void InstanceDomGenerator::UpdateContainerEntityInDomFromHighestAncestor(PrefabDom& instanceDom, const Instance& instance) const
        {
            // TODO: Modifies the function so it updates the transform only.

            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance, nullptr);

            // No need to update the instance DOM if the given instance is the root instance.
            if (climbUpResult.m_reachedInstance == &instance)
            {
                return;
            }

            // use instanceDom's allocator, because ultimately we will be setting this data back into
            // instanceDom with move semantics.
            PrefabDom containerEntityDom(&(instanceDom.GetAllocator()));
            GenerateContainerEntityDomFromClimbUpResult(containerEntityDom, climbUpResult);
            if (containerEntityDom.IsObject())
            {
                // Sets the container entity DOM in the given instance DOM.
                const AZStd::string containerEntityName = AZStd::string::format("/%s", PrefabDomUtils::ContainerEntityName);
                PrefabDomPath containerEntityPath(containerEntityName.c_str());
                // because containerEntityDom is an non-const reference, its memory will be adopted into instanceDom without copying
                containerEntityPath.Set(instanceDom, containerEntityDom);
            }
            else
            {
                AZ_Assert(
                    false,
                    "Prefab - InstanceDomGenerator::UpdateContainerEntityInDomFromHighestAncestor - "
                    "Couldn't find container entity DOM in any ancestor.");
            }
        }

        void InstanceDomGenerator::GenerateContainerEntityDomFromHighestAncestorOrSelf(PrefabDom& containerEntityDom, const Instance& instance) const
        {
            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance, nullptr);
            GenerateContainerEntityDomFromClimbUpResult(containerEntityDom, climbUpResult);
        }

        void InstanceDomGenerator::GenerateContainerEntityDomFromClimbUpResult(PrefabDom& containerEntityDom, const InstanceClimbUpResult& climbUpResult) const
        {
            // This function replaces the entire container entity DOM instead of just the transform component because right now, there is
            // no good way to identify the transform component directly in a DOM. But essentially all we need to update here is the
            // transform data and parent information in the transform component.

            AZ_Assert(climbUpResult.m_reachedInstance,
                "Prefab - InstanceDomGenerator::GenerateContainerEntityDomFromClimbUpResult - "
                "Called with no intance.");

            // Try and retrieve the instance's container entity DOM from the top-most ancestor that has it.
            // An ancestor DOM may not have the data if it has deleted the instance as an override
            if (!climbUpResult.m_climbedInstances.empty())
            {
                const AZStd::vector<const Instance*>& climbedInstances = climbUpResult.m_climbedInstances;
                const Instance* topInstance = climbUpResult.m_reachedInstance;
                const Instance* const bottomInstance = climbedInstances.front();
                for (auto instanceIter = climbedInstances.crbegin(); instanceIter != climbedInstances.crend(); ++instanceIter)
                {
                    // Create a path from the current top instance to the container entity of the bottom instance.
                    AZStd::string relativePathFromTopInstance =
                        PrefabInstanceUtils::GetRelativePathBetweenInstances(*topInstance, *bottomInstance);
                    const AZStd::string pathToContainerEntity =
                        AZStd::string::format("%s/%s", relativePathFromTopInstance.c_str(), PrefabDomUtils::ContainerEntityName);
                    PrefabDomPath domPathToContainerEntity(pathToContainerEntity.c_str());
                    const PrefabDom& topInstanceTemplateDom =
                        m_prefabSystemComponentInterface->FindTemplateDom((*topInstance).GetTemplateId());
                    // DOM reference can't be relied upon if the original DOM gets modified after reference creation.
                    // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
                    {
                        const PrefabDomValue* containerEntityDomFromTop = domPathToContainerEntity.Get(topInstanceTemplateDom);
                        if (containerEntityDomFromTop)
                        {
                            containerEntityDom.CopyFrom(*containerEntityDomFromTop, containerEntityDom.GetAllocator());
                            break;
                        }
                    }

                    topInstance = *instanceIter;
                }
            }

            // Check if the container entity DOM has been found in an ancestor.
            // Otherwise, retrieve the container entity DOM from the instance itself.
            // The container entity DOM may not be found if the instance has no ancestors,
            // or if all ancestors have deleted the instance as an override
            if (!containerEntityDom.IsObject())
            {
                const AZStd::string containerEntityName = AZStd::string::format("/%s", PrefabDomUtils::ContainerEntityName);
                PrefabDomPath containerEntityPath(containerEntityName.c_str());
                {
                    // DOM reference can't be relied upon if the original DOM gets modified after reference creation.
                    // This scope is added to limit their usage and ensure DOM is not modified when it is being used.
                    const PrefabDom& instanceTemplateDom =
                        m_prefabSystemComponentInterface->FindTemplateDom((*climbUpResult.m_reachedInstance).GetTemplateId());
                    containerEntityDom.CopyFrom(*containerEntityPath.Get(instanceTemplateDom), containerEntityDom.GetAllocator());
                }
            }

            AZ_Assert(
                containerEntityDom.IsObject(),
                "Prefab - InstanceDomGenerator::GenerateContainerEntityDomFromClimbUpResult - "
                "Couldn't find container entity DOM.");
        }
    } // namespace Prefab
} // namespace AzToolsFramework
