/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/MultiplayerTypes.h>
#include <AzNetworking/DataStructures/FixedSizeBitset.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzNetworking
{
    class ISerializer;
}

namespace Multiplayer
{
    class IMultiplayerComponentInput
    {
    public:
        virtual ~IMultiplayerComponentInput() = default;
        virtual NetComponentId GetNetComponentId() const = 0;
        virtual bool Serialize(AzNetworking::ISerializer& serializer) = 0;
        virtual IMultiplayerComponentInput& operator= (const IMultiplayerComponentInput&) { return *this; }
    };

    using MultiplayerComponentInputVector = AZStd::vector<AZStd::unique_ptr<IMultiplayerComponentInput>>;
}
