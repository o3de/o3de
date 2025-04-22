/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphUtil.h>

namespace AtomToolsFramework
{
    AZStd::string GetStringValueFromSlot(GraphModel::ConstSlotPtr slot, const AZStd::string& defaultValue)
    {
        const auto& value = slot ? slot->GetValue<AZStd::string>() : "";
        return !value.empty() ? value : defaultValue;
    }
} // namespace AtomToolsFramework
