/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(Module, AZ::SystemAllocator);

            Module();

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    }
} // namespace AZ
