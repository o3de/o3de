/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::IO
{
    class IStreamerProfiler
    {
    public:
        AZ_RTTI(AZ::IO::IStreamerProfiler, "{7BAE4936-B88C-4C5B-AF54-3ACD99AD0A2F}");

        IStreamerProfiler() = default;
        virtual ~IStreamerProfiler() = default;

        virtual void DrawStatistics(bool& keepDrawing) = 0;
    };

    using StreamerProfiler = AZ::Interface<IStreamerProfiler>;
} // namespace AZ::IO
