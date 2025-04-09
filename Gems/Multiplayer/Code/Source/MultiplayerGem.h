/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(MultiplayerModule, AZ::SystemAllocator);

        MultiplayerModule();
        ~MultiplayerModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
