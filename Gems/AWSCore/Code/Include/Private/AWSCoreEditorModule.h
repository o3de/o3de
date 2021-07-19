/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace AWSCore
{
    class AWSCoreEditorModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSCoreEditorModule, "{C1C9B898-848B-4C2F-A7AA-69642D12BCB5}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSCoreEditorModule, AZ::SystemAllocator, 0);

        AWSCoreEditorModule();
        ~AWSCoreEditorModule() override = default;
        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}

