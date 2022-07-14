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

            m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            AZ_Assert(m_prefabFocusInterface != nullptr,
                "Prefab - InstanceDomGenerator::Initialize - "
                "Prefab Focus Interface could not be found. "
                "Check that it is being correctly initialized.");

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                "Prefab - InstanceDomGenerator::Initialize - "
                "Prefab System Component Interface could not be found. "
                "Check that it is being correctly initialized.");
        }

        void InstanceDomGenerator::UnregisterInstanceDomGeneratorInterface()
        {
            m_prefabSystemComponentInterface = nullptr;
            m_prefabFocusInterface = nullptr;

            AZ::Interface<InstanceDomGeneratorInterface>::Unregister(this);
        }

        bool InstanceDomGenerator::GenerateInstanceDom(const Instance* instance, PrefabDom& instanceDom)
        {
            // Retrieve focused instance
            auto focusedInstance = m_prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);
            Instance* targetInstance = nullptr;
            if (focusedInstance.has_value())
            {
                targetInstance = &(focusedInstance->get());
            }

            auto climbUpToDomSourceInstanceeResult = PrefabInstanceUtils::ClimbUpToTargetInstance(instance, targetInstance);
            auto domSourceInstance = climbUpToDomSourceInstanceeResult.first;
            AZStd::string& relativePathToDomSourceInstance = climbUpToDomSourceInstanceeResult.second;

            PrefabDomPath domSourcePath(relativePathToDomSourceInstance.c_str());
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
            if (domSourceInstance != targetInstance)
            {
                auto climbUpToFocusedInstanceAncestorResult =
                    PrefabInstanceUtils::ClimbUpToTargetInstance(targetInstance, instance);
                auto focusedInstanceAncestor = climbUpToFocusedInstanceAncestorResult.first;
                AZStd::string& relativePathToFocusedInstanceAncestor = climbUpToFocusedInstanceAncestorResult.second;

                // If the focused instance is a descendant (or the instance itself), we need to replace its portion of the dom with the template one.
                if (focusedInstanceAncestor != nullptr && focusedInstanceAncestor == instance)
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
                    PrefabDomPath domSourceToFocusPath(relativePathToFocusedInstanceAncestor.c_str());
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

        void InstanceDomGenerator::ReplaceFocusedContainerTransformAccordingToRoot(
            const Instance* focusedInstance, PrefabDom& focusedInstanceDom) const
        {
            // Climb from the focused instance to the root and store the path.
            auto climbUpToFocusedInstanceResult = PrefabInstanceUtils::ClimbUpToTargetInstance(focusedInstance, nullptr);
            auto rootInstance = climbUpToFocusedInstanceResult.first;
            AZStd::string& rootToFocusedInstance = climbUpToFocusedInstanceResult.second;

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
    }
}
