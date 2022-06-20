/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Module/Module.h>
#include <Multiplayer/IMultiplayerTools.h>

namespace Multiplayer
{
    class MultiplayerToolsSystemComponent final
        : public AZ::Component
        , public IMultiplayerTools
    {
    public:
        AZ_COMPONENT(MultiplayerToolsSystemComponent, "{65AF5342-0ECE-423B-B646-AF55A122F72B}");

        static void Reflect(AZ::ReflectContext* context);

        MultiplayerToolsSystemComponent() = default;
        ~MultiplayerToolsSystemComponent() override = default;

        /// AZ::Component overrides.
        void Activate() override;
        void Deactivate() override;

        bool DidProcessNetworkPrefabs() override;

    private:
        void SetDidProcessNetworkPrefabs(bool didProcessNetPrefabs) override;

        bool m_didProcessNetPrefabs = false;
    };
} // namespace Multiplayer

