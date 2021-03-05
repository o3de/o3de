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
