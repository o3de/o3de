/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/NativeUI/NativeUISystemComponent.h>

namespace AZ::NativeUI
{
    AZStd::string NativeUISystem::DisplayBlockingDialog([[maybe_unused]] const AZStd::string& title, [[maybe_unused]] const AZStd::string& message,
        [[maybe_unused]] const AZStd::vector<AZStd::string>& options) const
    {
        return {};
    }
}
