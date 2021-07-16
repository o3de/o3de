/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

// Forward Declares
struct ImDrawData;

namespace ImGui
{
    class OtherActiveImGuiRequests : public AZ::EBusTraits
    {
    public:
        virtual ~OtherActiveImGuiRequests() = default;

        virtual void RenderImGuiBuffers(const ImDrawData& drawData) = 0;
    };

    //! ImGuiManager hands over drawing of ImGui content via this bus.
    using OtherActiveImGuiRequestBus = AZ::EBus<OtherActiveImGuiRequests>;

}
