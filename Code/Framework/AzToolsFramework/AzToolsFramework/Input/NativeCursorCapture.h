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
    //! Platform specific 'relative mouse mode' used while the viewport cursor is captured (e.g. camera look).
    //! While active, the platform keeps the system cursor pinned in place and reports the raw motion deltas it
    //! receives through the supplied callback. This avoids having to warp the cursor back after every move
    //! event, which on some platforms (macOS) is asynchronous and produces bogus (double counted) deltas.
    //! Not every platform provides an implementation, in which case Create returns nullptr and the caller
    //! must fall back to warping the cursor itself.
    class AZTF_API NativeCursorCapture
    {
    public:
        //! Callback invoked (on the main thread) with the raw cursor motion delta in logical pixels.
        using MotionDeltaFn = AZStd::function<void(const QPoint& delta)>;

        //! Create the platform implementation, or nullptr if the platform (or the current Qt platform plugin, e.g. the
        //! offscreen one used by tests) does not support it.
        static AZStd::unique_ptr<NativeCursorCapture> Create(MotionDeltaFn motionDeltaFn);

        virtual ~NativeCursorCapture() = default;

        //! Pin the cursor in place and start reporting motion deltas.
        virtual void Begin() = 0;
        //! Release the cursor and stop reporting motion deltas.
        virtual void End() = 0;
        //! Is the capture currently active.
        virtual bool IsActive() const = 0;
    };
} // namespace AzToolsFramework
