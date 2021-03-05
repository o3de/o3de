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

namespace Multiplayer
{
    class MultiplayerModule
        : public AZ::Module
    {
    public:

        AZ_RTTI(MultiplayerModule, "{497FF057-6CE1-43D5-9A9F-D2B7ABF6D3A7}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MultiplayerModule, AZ::SystemAllocator, 0);

        MultiplayerModule();
        ~MultiplayerModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
