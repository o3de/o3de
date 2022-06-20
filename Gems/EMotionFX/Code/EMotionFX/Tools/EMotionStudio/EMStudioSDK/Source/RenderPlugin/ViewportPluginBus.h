/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace EMStudio
{
    class ViewportPluginRequests 
        : public AZ::EBusTraits
    {
    public:
        virtual AZ::s32 GetViewportId() const = 0;
    };

    using ViewportPluginRequestBus = AZ::EBus<ViewportPluginRequests>;
}
