/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    struct DynamicNodeConfig;
}

namespace MaterialCanvas
{
    // Convert the node title into a filename for saving test data
    AZStd::string GetTestDataPathForNodeConfig(const AtomToolsFramework::DynamicNodeConfig& nodeConfig);

    // Create and save several test node configurations that will be loaded by the dynamic node manager
    void CreateTestNodes();
} // namespace MaterialCanvas
