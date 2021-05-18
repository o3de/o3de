/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        void InstanceUpdateExecutor::AddTemplateInstancesToQueue(TemplateId instanceTemplateId)
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

            for (auto instance : findInstancesResult->get())
            {
                m_instancesUpdateQueue.emplace(instance);
            }
        }

        bool InstanceUpdateExecutor::UpdateTemplateInstancesInQueue()
        {
            bool isUpdateSuccessful = true;
            if (!m_updatingTemplateInstancesInQueue)
            {
                m_updatingTemplateInstancesInQueue = true;

                const int instanceCountToUpdateInBatch =
                    m_instanceCountToUpdateInBatch == 0 ? m_instancesUpdateQueue.size() : m_instanceCountToUpdateInBatch;
                TemplateId currentTemplateId = InvalidTemplateId;
                TemplateReference currentTemplateReference = AZStd::nullopt;

                if (instanceCountToUpdateInBatch > 0)
                {
                    // Notify Propagation has begun
                    PrefabPublicNotificationBus::Broadcast(&PrefabPublicNotifications::OnPrefabInstancePropagationBegin);

                    EntityIdList selectedEntityIds;
                    ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, EntityIdList());

                    for (int i = 0; i < instanceCountToUpdateInBatch; ++i)
                    {
                        Instance* instanceToUpdate = m_instancesUpdateQueue.front();
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
                                m_instancesUpdateQueue.pop();
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
                            m_instancesUpdateQueue.pop();
                            continue;
                        }

                        Template& currentTemplate = currentTemplateReference->get();
                        Instance::EntityList newEntities;
                        if (PrefabDomUtils::LoadInstanceFromPrefabDom(*instanceToUpdate, newEntities, currentTemplate.GetPrefabDom()))
                        {
                            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                                &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, newEntities);
                        }
                        else
                        {
                            AZ_Error(
                                "Prefab", false,
                                "InstanceUpdateExecutor::UpdateTemplateInstancesInQueue - "
                                "Could not load Instance from Prefab DOM of Template with Id '%llu' on file path '%s'.",
                                currentTemplateId, currentTemplate.GetFilePath().c_str());

                            isUpdateSuccessful = false;
                        }

                        m_instancesUpdateQueue.pop();
                    }

                    for (auto entityIdIterator = selectedEntityIds.begin(); entityIdIterator != selectedEntityIds.end(); entityIdIterator++)
                    {
                        // Since entities get recreated during propagation, we need to check whether the entities correspoding to the list
                        // of selected entity ids are present or not.
                        AZ::Entity* entity = GetEntityById(*entityIdIterator);
                        if (entity == nullptr)
                        {
                            selectedEntityIds.erase(entityIdIterator--);
                        }
                    }
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectedEntityIds);

                    // Notify Propagation has ended
                    PrefabPublicNotificationBus::Broadcast(&PrefabPublicNotifications::OnPrefabInstancePropagationEnd);
                }

                m_updatingTemplateInstancesInQueue = false;
            }

            return isUpdateSuccessful;
        }
    }
}
