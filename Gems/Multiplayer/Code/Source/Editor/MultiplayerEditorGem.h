/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "MultiplayerGem.h"

namespace Multiplayer
{
    class MultiplayerEditorModule: public MultiplayerModule
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(MultiplayerEditorModule, "{50273605-2BBF-4D43-A42D-ED140B92B4D4}", MultiplayerModule);

        MultiplayerEditorModule();
        ~MultiplayerEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace Multiplayer
