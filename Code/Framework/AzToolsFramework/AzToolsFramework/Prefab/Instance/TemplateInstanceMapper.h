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

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapperInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class TemplateInstanceMapper final
            : public TemplateInstanceMapperInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(TemplateInstanceMapper, AZ::SystemAllocator, 0);
            AZ_RTTI(TemplateInstanceMapper, "{28EA46C7-F107-4D41-A008-960BED6371FB}", TemplateInstanceMapperInterface);

            TemplateInstanceMapper();
            ~TemplateInstanceMapper() override;

            InstanceSetConstReference FindInstancesOwnedByTemplate(const TemplateId& templateId) const override;

            bool RegisterTemplate(const TemplateId& templateId);
            bool UnregisterTemplate(const TemplateId& templateId);

        protected:
            bool RegisterInstanceToTemplate(Instance& instance) override;
            bool UnregisterInstance(Instance& instance) override;

        private:
            AZStd::unordered_map<TemplateId, InstanceSet> m_templateIdToInstancesMap;
        };
    }
}
