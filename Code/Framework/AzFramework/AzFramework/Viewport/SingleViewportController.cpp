/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SingleViewportController.h"

namespace AzFramework
{
    ViewportId SingleViewportController::GetViewportId() const
    {
        return m_viewportId;
    }

    void SingleViewportController::RegisterViewportContext(ViewportId viewport)
    {
        AZ_Assert(m_viewportId == InvalidViewportId, "Attempted to register an additional viewport to a single-viewport controller");
        m_viewportId = viewport;
    }

    void SingleViewportController::UnregisterViewportContext([[maybe_unused]]ViewportId viewport)
    {
        m_viewportId = InvalidViewportId;
    }
} //namespace AzFramework
