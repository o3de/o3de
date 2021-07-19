/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
