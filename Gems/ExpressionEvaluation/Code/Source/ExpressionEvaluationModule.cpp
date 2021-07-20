/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <ExpressionEvaluationSystemComponent.h>

namespace ExpressionEvaluation
{
    class ExpressionEvaluationModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ExpressionEvaluationModule, "{3183322D-3AE1-4B8B-86D7-870DA60DC175}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ExpressionEvaluationModule, AZ::SystemAllocator, 0);

        ExpressionEvaluationModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ExpressionEvaluationSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ExpressionEvaluationSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_ExpressionEvaluation, ExpressionEvaluation::ExpressionEvaluationModule)
