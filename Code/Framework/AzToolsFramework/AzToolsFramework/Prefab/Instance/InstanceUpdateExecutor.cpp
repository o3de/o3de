/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutor.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>
#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        InstanceUpdateExecutor::InstanceUpdateExecutor(int instanceCountToUpdateInBatch)
            : m_instanceCountToUpdateInBatch(instanceCountToUpdateInBatch)
        { 
        }

        void InstanceUpdateExecutor::RegisterInstanceUpdateExecutorInterface()
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                "Prefab - InstanceUpdateExecutor::RegisterInstanceUpdateExecutorInterface - "
                "Prefab System Component Interface could not be found. "
                "Check that it is being correctly initialized.");

            m_templateInstanceMapperInterface = AZ::Interface<TemplateInstanceMapperInterface>::Get();
            AZ_Assert(m_templateInstanceMapperInterface != nullptr,
                "Prefab - InstanceUpdateExecutor::RegisterInstanceUpdateExecutorInterface - "
                "Template Instance Mapper Interface could not be found. "
                "Check that it is being correctly initialized.");

            AZ::Interface<InstanceUpdateExecutorInterface>::Register(this);
        }

        void InstanceUpdateExecutor::UnregisterInstanceUpdateExecutorInterface()
        {
            AZ::Interface<InstanceUpdateExecutorInterface>::Unregister(this);
        }

        void InstanceUpdateExecutor::AddTemplateInstancesToQueue(TemplateId instanceTemplateId, InstanceOptionalConstReference instanceToExclude)
        {
            auto findInstancesResult =
                m_templateInstanceMapperInterface->FindInstancesOwnedByTemplate(instanceTemplateId);
            if (!findInstancesResult.has_value())
            {
                AZ_Warning("Prefab", false,
                    "InstanceUpdateExecutor::AddTemplateInstancesToQueue - "
                    "Could not find Template with Id '%llu' in Template Instance Mapper.",
                    instanceTemplateId);

                return;
            }

            const Instance* instanceToExcludePtr = nullptr;
            if (instanceToExclude.has_value())
            {
                instanceToExcludePtr = &(instanceToExclude->get());
            }

            for (auto instance : findInstancesResult->get())
            {
                if (instance != instanceToExcludePtr)
                {
                    m_instancesUpdateQueue.emplace_back(instance);
                }
            }
        }

        void InstanceUpdateExecutor::RemoveTemplateInstanceFromQueue(const Instance* instance)
        {
            AZStd::erase_if(m_instancesUpdateQueue, [instance](Instance* entry)
            {
                return entry == instance;
            });
        }

        bool InstanceUpdateExecutor::UpdateTemplateInstancesInQueue()
        {
            bool isUpdateSuccessful = true;
            if (!m_updatingTemplateInstancesInQueue)
            {
                m_updatingTemplateInstancesInQueue = true;

                const int instanceCountToUpdateInBatch =
                    m_instanceCountToUpdateInBatch == 0 ? static_cast<int>(m_instancesUpdateQueue.size()) : m_instanceCountToUpdateInBatch;
                TemplateId currentTemplateId = InvalidTemplateId;
                TemplateReference currentTemplateReference = AZStd::nullopt;

                if (instanceCountToUpdateInBatch > 0)
                {
                    // Notify Propagation has begun
                    PrefabPublicNotificationBus::Broadcast(&PrefabPublicNotifications::OnPrefabInstancePropagationBegin);

                    EntityIdList selectedEntityIds;
                    ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);
                    PrefabDom instanceDomFromRootDocument;

                    // Process all instances in the queue, capped to the batch size.
                    // Even though we potentially initialized the batch size to the queue, it's possible for the queue size to shrink
                    // during instance processing if the instance gets deleted and it was queued multiple times.  To handle this, we
                    // make sure to end the loop once the queue is empty, regardless of what the initial size was.
                    for (int i = 0; (i < instanceCountToUpdateInBatch) && !m_instancesUpdateQueue.empty(); ++i)
                    {
                        Instance* instanceToUpdate = m_instancesUpdateQueue.front();
                        m_instancesUpdateQueue.pop_front();
                        AZ_Assert(instanceToUpdate != nullptr, "Invalid instance on update queue.");

                        TemplateId instanceTemplateId = instanceToUpdate->GetTemplateId();
                        if (currentTemplateId != instanceTemplateId)
                        {
                            currentTemplateId = instanceTemplateId;
                            currentTemplateReference = m_prefabSystemComponentInterface->FindTemplate(currentTemplateId);
                            if (!currentTemplateReference.has_value())
                            {
                                AZ_Error(
                                    "Prefab", false,
                                    "InstanceUpdateExecutor::UpdateTemplateInstancesInQueue - "
                                    "Could not find Template using Id '%llu'. Unable to update Instance.",
                                    currentTemplateId);

                                // Remove the instance from update queue if its corresponding template couldn't be found
                                isUpdateSuccessful = false;
                                continue;
                            }
                        }

                        auto findInstancesResult = m_templateInstanceMapperInterface->FindInstancesOwnedByTemplate(instanceTemplateId);
                        AZ_Assert(
                            findInstancesResult.has_value(), "Prefab Instances corresponding to template with id %llu couldn't be found.",
                            instanceTemplateId);

                        if (findInstancesResult == AZStd::nullopt ||
                            findInstancesResult->get().find(instanceToUpdate) == findInstancesResult->get().end())
                        {
                            // Since nested instances get reconstructed during propagation, remove any nested instance that no longer
                            // maps to a template.
                            isUpdateSuccessful = false;
                            continue;
                        }

                        Instance::EntityList newEntities;

                        // Climb up to the root of the instance hierarchy from this instance
                        InstanceOptionalConstReference rootInstance = *instanceToUpdate;
                        AZStd::vector<InstanceOptionalConstReference> pathOfInstances;

                        while (rootInstance->get().GetParentInstance() != AZStd::nullopt)
                        {
                            pathOfInstances.emplace_back(rootInstance);
                            rootInstance = rootInstance->get().GetParentInstance();
                        }

                        AZStd::string aliasPathResult = "";
                        for (auto instanceIter = pathOfInstances.rbegin(); instanceIter != pathOfInstances.rend(); ++instanceIter)
                        {
                            aliasPathResult.append("/Instances/");
                            aliasPathResult.append((*instanceIter)->get().GetInstanceAlias());
                        }

                        PrefabDomPath rootPrefabDomPath(aliasPathResult.c_str());

                        PrefabDom& rootPrefabTemplateDom =
                            m_prefabSystemComponentInterface->FindTemplateDom(rootInstance->get().GetTemplateId());

                        auto instanceDomFromRootValue = rootPrefabDomPath.Get(rootPrefabTemplateDom);
                        if (!instanceDomFromRootValue)
                        {
                            AZ_Assert(
                                false,
                                "InstanceUpdateExecutor::UpdateTemplateInstancesInQueue - "
                                "Could not load Instance DOM from the top level ancestor's DOM.");

                            isUpdateSuccessful = false;
                            continue;
                        }

                        PrefabDomValueReference instanceDomFromRoot = *instanceDomFromRootValue;
                        if (!instanceDomFromRoot.has_value())
                        {
                            AZ_Assert(
                                false,
                                "InstanceUpdateExecutor::UpdateTemplateInstancesInQueue - "
                                "Could not load Instance DOM from the top level ancestor's DOM.");

                            isUpdateSuccessful = false;
                            continue;
                        }

                        // If a link was created for a nested instance before the changes were propagated,
                        // then we associate it correctly here
                        instanceDomFromRootDocument.CopyFrom(instanceDomFromRoot->get(), instanceDomFromRootDocument.GetAllocator());
                        if (PrefabDomUtils::LoadInstanceFromPrefabDom(*instanceToUpdate, newEntities, instanceDomFromRootDocument))
                        {
                            Template& currentTemplate = currentTemplateReference->get();
                            instanceToUpdate->GetNestedInstances([&](AZStd::unique_ptr<Instance>& nestedInstance) 
                            {
                                if (nestedInstance->GetLinkId() != InvalidLinkId)
                                {
                                    return;
                                }

                                for (auto linkId : currentTemplate.GetLinks())
                                {
                                    LinkReference nestedLink = m_prefabSystemComponentInterface->FindLink(linkId);
                                    if (!nestedLink.has_value())
                                    {
                                        continue;
                                    }

                                    if (nestedLink->get().GetInstanceName() == nestedInstance->GetInstanceAlias())
                                    {
                                        nestedInstance->SetLinkId(linkId);
                                        break;
                                    }
                                }
                            });

                            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                                &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, newEntities);
                        }
                    }
                    for (auto entityIdIterator = selectedEntityIds.begin(); entityIdIterator != selectedEntityIds.end(); entityIdIterator++)
                    {
                        // Since entities get recreated during propagation, we need to check whether the entities
                        // corresponding to the list of selected entity ids are present or not.
                        AZ::Entity* entity = GetEntityById(*entityIdIterator);
                        if (entity == nullptr)
                        {
                            selectedEntityIds.erase(entityIdIterator--);
                        }
                    }

                    // Notify Propagation has ended, then update selection (which is frozen during propagation, so this order matters)
                    PrefabPublicNotificationBus::Broadcast(&PrefabPublicNotifications::OnPrefabInstancePropagationEnd);
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectedEntityIds);
                }

                m_updatingTemplateInstancesInQueue = false;
            }

            return isUpdateSuccessful;
        }
    }
}
