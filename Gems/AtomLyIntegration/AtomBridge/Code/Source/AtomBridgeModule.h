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
#include <AzCore/Module/Module.h>

#include <FlyCameraInputComponent.h>
#include <AtomBridgeSystemComponent.h>

namespace AZ
{
    namespace AtomBridge
    {
        class Module
            : public AZ::Module
        {
        public:
            AZ_RTTI(Module, "{92196B90-6DF5-479D-8746-296AF56F0ABA}", AZ::Module);
            AZ_CLASS_ALLOCATOR(Module, AZ::SystemAllocator, 0);

            Module();

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    }
} // namespace AZ
