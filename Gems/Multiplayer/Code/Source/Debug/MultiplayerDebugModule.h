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
    class MultiplayerDebugModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(MultiplayerDebugModule, "{9E1460FA-4513-4B5E-86B4-9DD8ADEFA714}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MultiplayerDebugModule, AZ::SystemAllocator);

        MultiplayerDebugModule();
        ~MultiplayerDebugModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
