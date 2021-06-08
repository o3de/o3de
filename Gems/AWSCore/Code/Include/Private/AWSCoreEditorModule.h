/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

