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
