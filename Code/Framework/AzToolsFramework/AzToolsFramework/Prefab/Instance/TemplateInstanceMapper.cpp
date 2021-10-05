/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapper.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        TemplateInstanceMapper::TemplateInstanceMapper()
        {
            AZ::Interface<TemplateInstanceMapperInterface>::Register(this);
        }

        TemplateInstanceMapper::~TemplateInstanceMapper()
        {
            AZ::Interface<TemplateInstanceMapperInterface>::Unregister(this);
        }


        bool TemplateInstanceMapper::RegisterTemplate(TemplateId templateId)
        {
            const bool result = m_templateIdToInstancesMap.emplace(templateId, InstanceSet()).second;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RegisterTemplate - "
                "Failed to register Template '%llu' to Template to Instances mapper. "
                "Template may never have been registered or was unregistered early.",
                templateId);

            return result;
        }

        bool TemplateInstanceMapper::UnregisterTemplate(TemplateId templateId)
        {
            const bool result = m_templateIdToInstancesMap.erase(templateId) != 0;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::UnregisterTemplate - "
                "Failed to unregister Template '%llu' from Template to Instances mapper. "
                "This Template is likely already registered.",
                templateId);

            return result;
        }

        bool TemplateInstanceMapper::RegisterInstanceToTemplate(Instance& instance)
        {
            TemplateId templateId = instance.GetTemplateId();
            if (templateId == InvalidTemplateId)
            {
                return false; 
            }
            else
            {
                auto found = m_templateIdToInstancesMap.find(templateId);
                return found != m_templateIdToInstancesMap.end() &&
                    found->second.emplace(&instance).second;
            }
        }

        bool TemplateInstanceMapper::UnregisterInstance(Instance& instance)
        {
            // The InstanceUpdateExecutor queries the TemplateInstanceMapper for a list of instances related to a template.
            // Consequently, if an instance gets unregistered for a template, we need to notify the InstanceUpdateExecutor as well
            // so that it clears any internal associations that it might have in its queue.
            AZ_Assert(AZ::Interface<InstanceUpdateExecutorInterface>::Get() != nullptr, "InstanceUpdateExecutor doesn't exist");
            AZ::Interface<InstanceUpdateExecutorInterface>::Get()->RemoveTemplateInstanceFromQueue(&instance);

            auto found = m_templateIdToInstancesMap.find(instance.GetTemplateId());
            return found != m_templateIdToInstancesMap.end() &&
                found->second.erase(&instance) != 0;
        }

        InstanceSetConstReference TemplateInstanceMapper::FindInstancesOwnedByTemplate(TemplateId templateId) const
        {
            auto found = m_templateIdToInstancesMap.find(templateId);

            if (found != m_templateIdToInstancesMap.end())
            {
                return found->second;
            }
            else
            {
                return AZStd::nullopt;
            }
        }
    }
}
