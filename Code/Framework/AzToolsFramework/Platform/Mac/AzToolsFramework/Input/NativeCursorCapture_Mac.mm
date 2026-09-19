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
    //! macOS relative mouse mode.
    //!
    //! QCursor::setPos on macOS does not warp the cursor, it posts a synthetic HID mouse-moved event through
    //! CGEventPost which is applied asynchronously. Every hardware event that arrives between posting the warp
    //! and it taking effect is measured from the stale anchor position, so the motion gets counted more than
    //! once (and the more events per frame the device produces - trackpads, high polling rate mice - the worse
    //! it gets). Instead we decouple the cursor from the mouse entirely with CGAssociateMouseAndMouseCursorPosition
    //! (the cursor simply stops moving, no warping needed) and read the raw motion from NSEvent.deltaX/deltaY,
    //! which AppKit keeps delivering while the cursor is decoupled. This is the same approach SDL uses.
    //!
    //! The cursor association is process wide while an instance exists per viewport, so it is tracked with a
    //! static active count: the cursor is decoupled by the first capture to begin and restored by the last one to end.
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
        }

        void Begin() override
        {
            if (m_active)
            {
                return;
            }

            AZ_Warning(
                "NativeCursorCapture", s_activeCaptureCount == 0,
                "Another viewport already has the cursor captured, the cursor is only released once every capture has ended");

            m_active = true;
            m_accumulatedDeltaX = 0.0;
            m_accumulatedDeltaY = 0.0;

            if (auto* dispatcher = QAbstractEventDispatcher::instance())
            {
                dispatcher->installNativeEventFilter(this);
            }

            if (++s_activeCaptureCount == 1)
            {
                SetCursorAssociated(false);
            }
        }

        void End() override
        {
            if (!m_active)
            {
                return;
            }

            m_active = false;

            if (auto* dispatcher = QAbstractEventDispatcher::instance())
            {
                dispatcher->removeNativeEventFilter(this);
            }

            if (--s_activeCaptureCount == 0)
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
                    // deltas are fractional (pointer acceleration), so carry the remainder over to the next event
                    // rather than truncating it, otherwise very slow movement would be lost entirely
                    m_accumulatedDeltaX += nsEvent.deltaX;
                    m_accumulatedDeltaY += nsEvent.deltaY;

                    const QPoint delta(static_cast<int>(AZStd::trunc(m_accumulatedDeltaX)), static_cast<int>(AZStd::trunc(m_accumulatedDeltaY)));
                    m_accumulatedDeltaX -= delta.x();
                    m_accumulatedDeltaY -= delta.y();

                    // the callback runs the whole input chain synchronously (from inside NSApplication sendEvent),
                    // which may end the capture or destroy its owner, so do not touch 'this' after invoking it
                    if (const MotionDeltaFn motionDeltaFn = m_motionDeltaFn; motionDeltaFn && !delta.isNull())
                    {
                        motionDeltaFn(delta);
                    }
                }
                break;
            case NSEventTypeAppKitDefined:
                {
                    // never leave the cursor decoupled from the mouse while another application is in front,
                    // the user would be unable to move the cursor at all
                    if (nsEvent.subtype == NSEventSubtypeApplicationDeactivated)
                    {
                        SetCursorAssociated(true);
                    }
                    else if (nsEvent.subtype == NSEventSubtypeApplicationActivated && s_activeCaptureCount > 0)
                    {
                        SetCursorAssociated(false);
                    }
                }
                break;
            default:
                break;
            }

            return false;
        }

    private:
        static void SetCursorAssociated(const bool associated)
        {
            if (associated != s_cursorAssociated)
            {
                s_cursorAssociated = associated;
                CGAssociateMouseAndMouseCursorPosition(associated);
            }
        }

        static inline int s_activeCaptureCount = 0; //!< Captures currently active across all viewports.
        static inline bool s_cursorAssociated = true; //!< Is the cursor currently following the mouse (process wide).

        MotionDeltaFn m_motionDeltaFn;
        double m_accumulatedDeltaX = 0.0; //!< Fractional motion not yet reported.
        double m_accumulatedDeltaY = 0.0;
        bool m_active = false;
    };

    AZStd::unique_ptr<NativeCursorCapture> NativeCursorCapture::Create(MotionDeltaFn motionDeltaFn)
    {
        // Only the cocoa platform plugin delivers NSEvents. With the offscreen/minimal plugins (unit tests) there is
        // nothing to read the motion from, and decoupling the cursor would affect the developer's real mouse.
        if (QGuiApplication::platformName() != QLatin1String("cocoa"))
        {
            return nullptr;
        }

        return AZStd::make_unique<NativeCursorCaptureMac>(AZStd::move(motionDeltaFn));
    }
} // namespace AzToolsFramework
