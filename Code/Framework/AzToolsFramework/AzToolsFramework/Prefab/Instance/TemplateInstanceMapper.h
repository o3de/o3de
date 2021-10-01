/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            InstanceSetConstReference FindInstancesOwnedByTemplate(TemplateId templateId) const override;

            bool RegisterTemplate(TemplateId templateId);
            bool UnregisterTemplate(TemplateId templateId);

        protected:
            bool RegisterInstanceToTemplate(Instance& instance) override;
            bool UnregisterInstance(Instance& instance) override;

        private:
            AZStd::unordered_map<TemplateId, InstanceSet> m_templateIdToInstancesMap;
        };
    }
}
