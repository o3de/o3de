/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AtomToolsFramework
{
    //! AssetStatusReporterState current state of assets reported from asset reporter
    enum class AssetStatusReporterState : int
    {
        Invalid = -1,
        Failed,
        Processing,
        Succeeded
    };
} // namespace AtomToolsFramework
