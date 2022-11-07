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
#include <AzCore/Memory/AllocatorManager.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceDomGeneratorInterface.h>
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
            , m_GameModeEventHandler(
                  [this](GameModeState state)
                  {
                      m_shouldPausePropagation = (state == GameModeState::Started);
                  })
        {
        }

        void InstanceUpdateExecutor::RegisterInstanceUpdateExecutorInterface()
        {
            AZ::Interface<InstanceUpdateExecutorInterface>::Register(this);

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

            m_instanceDomGeneratorInterface = AZ::Interface<InstanceDomGeneratorInterface>::Get();
            AZ_Assert(m_instanceDomGeneratorInterface != nullptr,
                "Prefab - InstanceUpdateExecutor::RegisterInstanceUpdateExecutorInterface - "
                "Instance Dom Generator Interface could not be found. "
                "Check that it is being correctly initialized.");
        }

        void InstanceUpdateExecutor::UnregisterInstanceUpdateExecutorInterface()
        {
            m_GameModeEventHandler.Disconnect();

            m_instanceDomGeneratorInterface = nullptr;
            m_templateInstanceMapperInterface = nullptr;
            m_prefabSystemComponentInterface = nullptr;

            AZ::Interface<InstanceUpdateExecutorInterface>::Unregister(this);
        }

        void InstanceUpdateExecutor::AddInstanceToQueue(InstanceOptionalReference instance)
        {
            if (!instance.has_value())
            {
                AZ_Warning(
                    "Prefab", false, "InstanceUpdateExecutor::AddInstanceToQueue - "
                    "Caller tried to insert null instance into the queue.");

                return;
            }

            AddInstanceToQueue(&(instance->get()));
        }

        void InstanceUpdateExecutor::AddInstanceToQueue(Instance* instance)
        {
            // Skip the insertion into queue if the instance is already present in the set.
            if (m_uniqueInstancesForPropagation.emplace(instance).second)
            {
                m_instancesUpdateQueue.emplace_back(instance);
            }
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
                    AddInstanceToQueue(instance);
                }
            }
        }

        void InstanceUpdateExecutor::RemoveTemplateInstanceFromQueue(Instance* instance)
        {
            // Skip the removal from the queue if the instance is not present in the set.
            if (m_uniqueInstancesForPropagation.erase(instance))
            {
                AZStd::erase_if(
                    m_instancesUpdateQueue,
                    [instance](Instance* entry)
                    {
                        return entry == instance;
                    });
            }
        }

        void InstanceUpdateExecutor::LazyConnectGameModeEventHandler()
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            
            if (!m_GameModeEventHandler.IsConnected() && prefabEditorEntityOwnershipInterface)
            {
                prefabEditorEntityOwnershipInterface->RegisterGameModeEventHandler(m_GameModeEventHandler);
            }
        }

        bool InstanceUpdateExecutor::UpdateTemplateInstancesInQueue()
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            LazyConnectGameModeEventHandler();

            bool isUpdateSuccessful = true;
            if (!m_updatingTemplateInstancesInQueue && !m_shouldPausePropagation)
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

                    // Process all instances in the queue, capped to the batch size.
                    // Even though we potentially initialized the batch size to the queue, it's possible for the queue size to shrink
                    // during instance processing if the instance gets deleted and it was queued multiple times.  To handle this, we
                    // make sure to end the loop once the queue is empty, regardless of what the initial size was.
                    for (int i = 0; (i < instanceCountToUpdateInBatch) && !m_instancesUpdateQueue.empty(); ++i)
                    {
                        Instance* instanceToUpdate = m_instancesUpdateQueue.front();
                        m_instancesUpdateQueue.pop_front();
                        AZ_Assert(instanceToUpdate != nullptr, "Invalid instance on update queue.");
                        m_uniqueInstancesForPropagation.erase(instanceToUpdate);

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

                        // Generates instance DOM for a given instance object from focused or root prefab template.
                        PrefabDom instanceDom;
                        m_instanceDomGeneratorInterface->GenerateInstanceDom(instanceDom, *instanceToUpdate);

                        if (!instanceDom.IsObject())
                        {
                            AZ_Assert(
                                false,
                                "InstanceUpdateExecutor::UpdateTemplateInstancesInQueue - "
                                "Could not load Instance DOM from the given Instance.");

                            isUpdateSuccessful = false;
                            continue;
                        }

                        // Loads instance object from the generated instance DOM.
                        EntityList newEntities;
                        if (PrefabDomUtils::LoadInstanceFromPrefabDom(*instanceToUpdate, newEntities, instanceDom,
                            PrefabDomUtils::LoadFlags::UseSelectiveDeserialization))
                        {
                            Template& currentTemplate = currentTemplateReference->get();
                            instanceToUpdate->GetNestedInstances([&](AZStd::unique_ptr<Instance>& nestedInstance) 
                            {
                                if (!nestedInstance || nestedInstance->GetLinkId() != InvalidLinkId)
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

                            if (!m_isRootPrefabInstanceLoaded &&
                                instanceToUpdate->GetTemplateSourcePath() == m_rootPrefabInstanceSourcePath)
                            {
                                PrefabPublicNotificationBus::Broadcast(&PrefabPublicNotifications::OnRootPrefabInstanceLoaded);
                                m_isRootPrefabInstanceLoaded = true;
                            }
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

                    // after an instance update operation finishes, garbage collect to reclaim memory.
                    AZ::AllocatorManager::Instance().GarbageCollect();
                    AZ_MALLOC_TRIM(0);
                }

                m_updatingTemplateInstancesInQueue = false;
            }

            return isUpdateSuccessful;
        }

        void InstanceUpdateExecutor::QueueRootPrefabLoadedNotificationForNextPropagation()
        {
            m_isRootPrefabInstanceLoaded = false;

            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            m_rootPrefabInstanceSourcePath =
                prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetTemplateSourcePath();
        }

        void InstanceUpdateExecutor::SetShouldPauseInstancePropagation(bool shouldPausePropagation)
        {
            m_shouldPausePropagation = shouldPausePropagation;
        }
    }
}
