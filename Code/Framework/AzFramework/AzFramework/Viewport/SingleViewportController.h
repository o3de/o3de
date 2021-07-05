/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportControllerInterface.h>

namespace AzFramework
{
    //! SingleViewportController defines a viewport controller interface that supports only registering
    //! one viewport at any given time. This can be used for simple cases in which the viewport controller
    //! will not be shared in a ViewportControllerList between multiple viewports.
    class SingleViewportController
        : public ViewportControllerInterface
    {
    public:
        ViewportId GetViewportId() const;

        // ViewportControllerInterface ...
        void RegisterViewportContext(ViewportId viewport) override;
        void UnregisterViewportContext(ViewportId viewport) override;

    private:
        ViewportId m_viewportId = InvalidViewportId;
    };
} //namespace AzFramework
