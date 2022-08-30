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

namespace AzToolsFramework
{
    namespace Prefab
    {
        AzFramework::EntityContextId InstanceDomGenerator::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        void InstanceDomGenerator::RegisterInstanceDomGeneratorInterface()
        {
            AZ::Interface<InstanceDomGeneratorInterface>::Register(this);

            // Get EditorEntityContextId
            EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                "Prefab - InstanceDomGenerator::Initialize - "
                "Prefab System Component Interface could not be found. "
                "Check that it is being correctly initialized.");
        }

        void InstanceDomGenerator::UnregisterInstanceDomGeneratorInterface()
        {
            m_prefabSystemComponentInterface = nullptr;

            AZ::Interface<InstanceDomGeneratorInterface>::Unregister(this);
        }

        bool InstanceDomGenerator::GenerateInstanceDom(PrefabDom& instanceDom, const Instance& instance) const
        {
            // Retrieves the focused instance.
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (!prefabFocusInterface)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Prefab Focus Interface couldn not be found.");
                return false;
            }

            InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);
            if (!focusedInstance.has_value())
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Could not get the focused instance. It should not be null.");
                return false;
            }
            const Instance* focusedInstancePtr = &(focusedInstance->get());

            // Climbs up from the given instance to root instance, but stops at the focused instance if they can meet.
            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance, focusedInstancePtr);
            const Instance* focusedOrRootInstancePtr = climbUpResult.m_reachedInstance;
            if (!focusedOrRootInstancePtr) 
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Could not get the focused or root instance. It should not be null.");
                return false;
            }

            // Copies the instance DOM, that is stored in the focused or root template DOM, into the output instance DOM.
            AZStd::string relativePathFromTop = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);
            PrefabDomPath relativeDomPath(relativePathFromTop.c_str());
            PrefabDom focusedOrRootTemplateDom;
            focusedOrRootTemplateDom.CopyFrom(
                m_prefabSystemComponentInterface->FindTemplateDom((*focusedOrRootInstancePtr).GetTemplateId()),
                instanceDom.GetAllocator());
            auto instanceDomFromTemplate = relativeDomPath.Get(focusedOrRootTemplateDom);
            if (!instanceDomFromTemplate)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Could not get the instance DOM stored in the focused or root template DOM.");
                return false;
            }

            instanceDom.CopyFrom(*instanceDomFromTemplate, instanceDom.GetAllocator());

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
                    instanceDom.GetAllocator());

                UpdateContainerEntityInDomFromRoot(focusedTemplateDom, focusedInstance->get());

                // Forces a hard copy using the instanceDom's allocator.
                PrefabDom focusedTemplateDomCopy;
                focusedTemplateDomCopy.CopyFrom(focusedTemplateDom, instanceDom.GetAllocator());

                // Stores the focused DOM into the instance DOM.
                AZStd::string relativePathToFocus = PrefabInstanceUtils::GetRelativePathBetweenInstances(
                    instance, focusedInstance->get());
                PrefabDomPath relativeDomPathToFocus(relativePathToFocus.c_str());

                relativeDomPathToFocus.Set(instanceDom, focusedTemplateDomCopy, instanceDom.GetAllocator());
            }
            // Skips additional processing if the focused instance is a proper ancestor of the given instance, or
            // the focused instance has no hierarchy relation with the given instance.

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
            const PrefabDom& rootTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom((*rootInstancePtr).GetTemplateId());
            PrefabDom containerEntityDom;
            containerEntityDom.CopyFrom(*domPathToContainerEntity.Get(rootTemplateDom), instanceDom.GetAllocator());

            // Sets the container entity DOM in the given instance DOM.
            const AZStd::string containerEntityName = AZStd::string::format("/%s", PrefabDomUtils::ContainerEntityName);
            PrefabDomPath containerEntityPath(containerEntityName.c_str());
            containerEntityPath.Set(instanceDom, containerEntityDom, instanceDom.GetAllocator());
        }
    } // namespace Prefab
} // namespace AzToolsFramework
