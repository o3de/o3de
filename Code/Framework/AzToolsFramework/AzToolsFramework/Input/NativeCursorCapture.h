/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <QPoint>

namespace AzToolsFramework
{
    //! Platform relative-mouse mode for captured viewport cursors.
    //! Implementations keep the cursor stationary and report relative movement.
    //! Unsupported platforms return nullptr and the caller falls back to cursor warping.
    class AZTF_API NativeCursorCapture
    {
    public:
        //! Callback invoked on the main thread with relative cursor movement in logical pixels.
        using MotionDeltaFn = AZStd::function<void(const QPoint& delta)>;

        //! Create the platform implementation, or nullptr when unsupported.
        static AZStd::unique_ptr<NativeCursorCapture> Create(MotionDeltaFn motionDeltaFn);

        virtual ~NativeCursorCapture() = default;

        //! Keep the cursor stationary and start reporting relative movement.
        //! Returns false when capture cannot begin.
        virtual bool Begin() = 0;
        //! Release the cursor and stop reporting motion deltas.
        virtual void End() = 0;
        //! Is capture active.
        virtual bool IsActive() const = 0;
    };
} // namespace AzToolsFramework
