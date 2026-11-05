/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::RPI
{
    class Pass;
}

namespace AZ::Render
{
    //! Global tracing selection. Existing scenes default to Hardware; Auto selects software only without hardware RT.
    bool IsDiffuseProbeGridSoftwareTracing();
    bool IsDiffuseProbeGridOffscreenSkinningRequested();
    //! Update consumers must not read ray data before their software producer has valid resources.
    bool IsDiffuseProbeGridSoftwareTracingReady(const RPI::Pass& consumer);
}
