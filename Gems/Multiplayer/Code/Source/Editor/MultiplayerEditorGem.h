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
