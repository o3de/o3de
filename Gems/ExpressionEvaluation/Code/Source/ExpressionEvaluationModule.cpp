/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ_CLASS_ALLOCATOR(ExpressionEvaluationModule, AZ::SystemAllocator);

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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ExpressionEvaluation::ExpressionEvaluationModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ExpressionEvaluation, ExpressionEvaluation::ExpressionEvaluationModule)
#endif
