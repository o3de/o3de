/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ::RPI
{
    class Pass;
}

namespace AZ::Render
{
    enum class DiffuseProbeGridMode : uint8_t;

    //! Project-wide GI mode selected by the quality system.
    DiffuseProbeGridMode GetDiffuseProbeGridMode();
    //! Auto uses hardware tracing when available and software otherwise.
    bool IsDiffuseProbeGridSoftwareTracing();
    bool IsDiffuseProbeGridOffscreenSkinningRequested();
    //! Update consumers must not read ray data before their software producer has valid resources.
    bool IsDiffuseProbeGridSoftwareTracingReady(const RPI::Pass& consumer);
}
