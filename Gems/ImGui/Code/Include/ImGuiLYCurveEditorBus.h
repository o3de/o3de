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