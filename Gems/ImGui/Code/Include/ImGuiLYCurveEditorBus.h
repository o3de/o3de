/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace ImGui
{
    /// Bus for sending events and getting state from the ImGui manager.
    class IImGuiCurveEditorRequests : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiCurveEditorRequests"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiCurveEditorRequests>;
    };

    using ImGuiCurveEditorRequestBus = AZ::EBus<IImGuiCurveEditorRequests>;

} // namespace ImGui
