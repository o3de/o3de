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
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>
#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AzFramework::EntityContextId InstanceUpdateExecutor::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

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
            // Get EditorEntityContextId
            EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            AZ_Assert(m_prefabFocusInterface != nullptr,
                "Prefab - InstanceUpdateExecutor::RegisterInstanceUpdateExecutorInterface - "
                "Prefab Focus Interface could not be found. "
                "Check that it is being correctly initialized.");

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
            PropertyEditorGUIMessages::Bus::Handler::BusConnect();
        }

        void InstanceUpdateExecutor::UnregisterInstanceUpdateExecutorInterface()
        {
            PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
            m_GameModeEventHandler.Disconnect();
            AZ::Interface<InstanceUpdateExecutorInterface>::Unregister(this);
        }

        void InstanceUpdateExecutor::AddInstanceToQueue(InstanceOptionalReference instance)
        {
            if (instance == AZStd::nullopt)
            {
                AZ_Warning(
                    "Prefab", false,
                    "InstanceUpdateExecutor::AddInstanceToQueue - "
                    "Caller tried to insert null instance into the queue.");

                return;
            }

            m_instancesUpdateQueue.emplace_back(&instance->get());
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
                }
            );
        }

        const void InstanceUpdateExecutor::ReplaceFocusedContainerTransformAccordingToRoot(
            const Instance* focusedInstance, PrefabDom& focusedInstanceDom) const
        {
            // Climb from the focused instance to the root and store the path.
            AZStd::string rootToFocusedInstance;
            auto rootInstance = PrefabDomUtils::ClimbUpToTargetInstance(focusedInstance, nullptr, rootToFocusedInstance);

            if (rootInstance != focusedInstance)
            {
                // Create the path from the root instance to the container entity of the focused instance.
                AZStd::string rootToFocusedInstanceContainer =
                    AZStd::string::format("%s/%s", rootToFocusedInstance.c_str(), PrefabDomUtils::ContainerEntityName);
                PrefabDomPath rootToFocusedInstanceContainerPath(rootToFocusedInstanceContainer.c_str());

                // Retrieve the dom of the root instance.
                PrefabDom rootDom;
                rootDom.CopyFrom(
                    m_prefabSystemComponentInterface->FindTemplateDom(rootInstance->GetTemplateId()), focusedInstanceDom.GetAllocator());

                PrefabDom containerDom;
                containerDom.CopyFrom(*rootToFocusedInstanceContainerPath.Get(rootDom), focusedInstanceDom.GetAllocator());

                // Paste the focused instance container dom as seen from the root into instanceDom.
                AZStd::string containerName =
                    AZStd::string::format("/%s", PrefabDomUtils::ContainerEntityName);
                PrefabDomPath containerPath(containerName.c_str());
                containerPath.Set(focusedInstanceDom, containerDom, focusedInstanceDom.GetAllocator());
            }
        }

        bool InstanceUpdateExecutor::GenerateInstanceDomAccordingToCurrentFocus(const Instance* instance, PrefabDom& instanceDom)
        {
            // Retrieve focused instance
            auto focusedInstance = m_prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);

            AZStd::string aliasPath;
            auto domSourceInstance = PrefabDomUtils::ClimbUpToTargetInstance(instance, &focusedInstance->get(), aliasPath);
            
            PrefabDomPath domSourcePath(aliasPath.c_str());
            PrefabDom partialInstanceDom;
            partialInstanceDom.CopyFrom(
                m_prefabSystemComponentInterface->FindTemplateDom(domSourceInstance->GetTemplateId()), instanceDom.GetAllocator());

            auto instanceDomValueFromSource = domSourcePath.Get(partialInstanceDom);
            if (!instanceDomValueFromSource)
            {
                return false;
            }

            instanceDom.CopyFrom(*instanceDomValueFromSource, instanceDom.GetAllocator());

            // If the focused instance is not an ancestor of our instance, verify if it's a descendant.
            if (domSourceInstance != &focusedInstance->get())
            {
                AZStd::string aliasPathToFocus;
                auto focusedInstanceAncestor = PrefabDomUtils::ClimbUpToTargetInstance(&focusedInstance->get(), instance, aliasPathToFocus);

                // If the focused instance is a descendant (or the instance itself), we need to replace its portion of the dom with the template one.
                if (focusedInstanceAncestor == instance)
                {
                    // Get the dom for the focused instance from its template.
                    PrefabDom focusedInstanceDom;
                    focusedInstanceDom.CopyFrom(
                        m_prefabSystemComponentInterface->FindTemplateDom(focusedInstance->get().GetTemplateId()),
                        instanceDom.GetAllocator());

                    // Replace the container entity with the one as seen by the root
                    // TODO - this function should only replace the transform!
                    ReplaceFocusedContainerTransformAccordingToRoot(&focusedInstance->get(), focusedInstanceDom);
                    
                    // Copy the focused instance dom inside the dom that will be used to refresh the instance.
                    PrefabDomPath domSourceToFocusPath(aliasPathToFocus.c_str());
                    domSourceToFocusPath.Set(instanceDom, focusedInstanceDom, instanceDom.GetAllocator());

                    // Force a deep copy
                    PrefabDom instanceDomCopy;
                    instanceDomCopy.CopyFrom(instanceDom, instanceDom.GetAllocator());

                    instanceDom.CopyFrom(instanceDomCopy, instanceDom.GetAllocator());
                }
            }
            // If our instance is the focused instance, fix the container
            else if (&focusedInstance->get() == instance)
            {
                // Replace the container entity with the one as seen by the root
                // TODO - this function should only replace the transform!
                ReplaceFocusedContainerTransformAccordingToRoot(instance, instanceDom);
            }

            PrefabDomValueReference instanceDomFromRoot = instanceDom;
            if (!instanceDomFromRoot.has_value())
            {
                return false;
            }

            return true;
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
                    PrefabDom instanceDomAccordingToFocus;

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

                        EntityList newEntities;

                        // TODO - Add relevant comment.
                        bool instanceDomGenerated = GenerateInstanceDomAccordingToCurrentFocus(instanceToUpdate, instanceDomAccordingToFocus);
                        if (!instanceDomGenerated)
                        {
                            AZ_Assert(
                                false,
                                "InstanceUpdateExecutor::UpdateTemplateInstancesInQueue - "
                                "Could not load Instance DOM from the top level ancestor's DOM.");

                            isUpdateSuccessful = false;
                            continue;
                        }

                        if (PrefabDomUtils::LoadInstanceFromPrefabDom(*instanceToUpdate, newEntities, instanceDomAccordingToFocus))
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
                }

                m_updatingTemplateInstancesInQueue = false;
            }

            return isUpdateSuccessful;
        }

        void InstanceUpdateExecutor::RequestWrite(QWidget*)
        {
            m_shouldPausePropagation = true;
        }

        void InstanceUpdateExecutor::OnEditingFinished(QWidget*)
        {
            m_shouldPausePropagation = false;
        }

        void InstanceUpdateExecutor::QueueRootPrefabLoadedNotificationForNextPropagation()
        {
            m_isRootPrefabInstanceLoaded = false;

            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
            m_rootPrefabInstanceSourcePath =
                prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()->get().GetTemplateSourcePath();
        }
    }
}
