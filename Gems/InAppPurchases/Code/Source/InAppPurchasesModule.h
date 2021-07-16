/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>
#include <IGem.h>

namespace InAppPurchases
{
    class InAppPurchasesModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(InAppPurchasesModule, "{19EC24B6-87E8-44AE-AC33-C280F67FD3F7}", AZ::Module);
        
        InAppPurchasesModule();
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
