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
