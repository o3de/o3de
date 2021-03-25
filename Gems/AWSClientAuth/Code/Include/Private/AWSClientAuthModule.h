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

#include <AzCore/Module/Module.h>

namespace AWSClientAuth
{
    //! Entry point for the Gem.
    class AWSClientAuthModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSClientAuthModule, "{85AD4C5F-A40A-4503-9202-4B8BE6AF0DCD}", AZ::Module);

        AWSClientAuthModule();
        virtual ~AWSClientAuthModule() override = default;
        virtual AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
