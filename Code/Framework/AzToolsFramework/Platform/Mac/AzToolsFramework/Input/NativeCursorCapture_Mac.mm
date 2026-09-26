/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Input/NativeCursorCapture.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/math.h>

#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>
#include <QGuiApplication>

#import <Cocoa/Cocoa.h>

namespace AzToolsFramework
{
    // QCursor::setPos posts an asynchronous event on macOS.
    // Movement before the warp completes can be counted twice.
    // Decouple the cursor instead and read relative movement from NSEvent.
    // Cursor association is process-wide, so only one viewport may capture it.
    class NativeCursorCaptureMac final
        : public NativeCursorCapture
        , public QAbstractNativeEventFilter
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeCursorCaptureMac, AZ::SystemAllocator);

        explicit NativeCursorCaptureMac(MotionDeltaFn motionDeltaFn)
            : m_motionDeltaFn(AZStd::move(motionDeltaFn))
        {
        }

        ~NativeCursorCaptureMac() override
        {
            End();

            // Retry a failed restoration before the last recovery opportunity is lost.
            if (!s_activeCapture)
            {
                SetCursorAssociated(true);
            }
        }

        bool Begin() override
        {
            if (m_active)
            {
                return true;
            }

            if (s_activeCapture)
            {
                AZ_Warning("NativeCursorCapture", false, "Another viewport already has the cursor captured");
                return false;
            }

            if (!SetCursorAssociated(false))
            {
                return false;
            }

            m_active = true;
            m_accumulatedDeltaX = 0.0;
            m_accumulatedDeltaY = 0.0;
            if (auto* dispatcher = QAbstractEventDispatcher::instance())
            {
                dispatcher->installNativeEventFilter(this);
            }

            s_activeCapture = this;
            return true;
        }

        void End() override
        {
            if (m_active)
            {
                ClearActiveCapture();
            }

            // Inactive owners may retry a restoration that failed during an earlier End call.
            if (!s_activeCapture)
            {
                SetCursorAssociated(true);
            }
        }

        bool IsActive() const override
        {
            return m_active;
        }

        // QAbstractNativeEventFilter overrides ...
        bool nativeEventFilter([[maybe_unused]] const QByteArray& eventType, void* message, [[maybe_unused]] qintptr* result) override
        {
            if (!m_active)
            {
                return false;
            }

            NSEvent* nsEvent = static_cast<NSEvent*>(message);
            switch (nsEvent.type)
            {
            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeOtherMouseDragged:
                {
                    // Preserve fractional movement so slow input is not lost to integer truncation.
                    m_accumulatedDeltaX += nsEvent.deltaX;
                    m_accumulatedDeltaY += nsEvent.deltaY;

                    const QPoint delta(
                        static_cast<int>(AZStd::trunc(m_accumulatedDeltaX)),
                        static_cast<int>(AZStd::trunc(m_accumulatedDeltaY)));
                    m_accumulatedDeltaX -= delta.x();
                    m_accumulatedDeltaY -= delta.y();

                    // The callback may end capture or destroy its owner, so do not access this object afterward.
                    if (const MotionDeltaFn motionDeltaFn = m_motionDeltaFn; motionDeltaFn && !delta.isNull())
                    {
                        motionDeltaFn(delta);
                    }
                }
                break;
            case NSEventTypeAppKitDefined:
                {
                    // Keep the cursor usable while another application is active.
                    if (nsEvent.subtype == NSEventSubtypeApplicationDeactivated)
                    {
                        SetCursorAssociated(true);
                    }
                    else if (nsEvent.subtype == NSEventSubtypeApplicationActivated && !SetCursorAssociated(false))
                    {
                        // Fall back to cursor warping if native capture cannot be restored.
                        ClearActiveCapture();
                    }
                }
                break;
            default:
                break;
            }

            return false;
        }

    private:
        void ClearActiveCapture()
        {
            m_active = false;

            if (auto* dispatcher = QAbstractEventDispatcher::instance())
            {
                dispatcher->removeNativeEventFilter(this);
            }

            AZ_Assert(s_activeCapture == this, "The active native cursor capture owner changed unexpectedly");
            s_activeCapture = nullptr;
        }

        static bool SetCursorAssociated(const bool associated)
        {
            if (associated == s_cursorAssociated)
            {
                return true;
            }

            const CGError result = CGAssociateMouseAndMouseCursorPosition(associated);
            AZ_Warning(
                "NativeCursorCapture",
                result == kCGErrorSuccess,
                "Failed to change mouse cursor association (CGError %d)",
                result);
            if (result != kCGErrorSuccess)
            {
                return false;
            }

            s_cursorAssociated = associated;
            return true;
        }

        static inline NativeCursorCaptureMac* s_activeCapture = nullptr; //!< Process-wide capture owner.
        static inline bool s_cursorAssociated = true; //!< Is the cursor currently following the mouse (process wide).

        MotionDeltaFn m_motionDeltaFn;
        double m_accumulatedDeltaX = 0.0; //!< Fractional motion not yet reported.
        double m_accumulatedDeltaY = 0.0;
        bool m_active = false;
    };

    AZStd::unique_ptr<NativeCursorCapture> NativeCursorCapture::Create(MotionDeltaFn motionDeltaFn)
    {
        // Non-Cocoa plugins cannot provide NSEvents and must not affect the real cursor during tests.
        if (QGuiApplication::platformName() != QLatin1String("cocoa"))
        {
            return nullptr;
        }

        return AZStd::make_unique<NativeCursorCaptureMac>(AZStd::move(motionDeltaFn));
    }
} // namespace AzToolsFramework
