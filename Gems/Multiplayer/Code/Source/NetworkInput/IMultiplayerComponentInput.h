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

#include <Source/MultiplayerTypes.h>
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
        virtual NetComponentId GetComponentId() const = 0;
        virtual bool Serialize(AzNetworking::ISerializer& serializer) = 0;
    };

    using MultiplayerComponentInputVector = AZStd::vector<AZStd::unique_ptr<IMultiplayerComponentInput>>;
}
