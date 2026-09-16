/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Input/NativeCursorCapture.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/math.h>

#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>

#import <AppKit/NSApplication.h>
#import <AppKit/NSEvent.h>
#include <CoreGraphics/CoreGraphics.h>

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

            m_active = true;
            m_accumulatedDelta = CGPointZero;

            if (auto* dispatcher = QAbstractEventDispatcher::instance())
            {
                dispatcher->installNativeEventFilter(this);
            }

            SetCursorAssociated(false);
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

            SetCursorAssociated(true);
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
                    m_accumulatedDelta.x += nsEvent.deltaX;
                    m_accumulatedDelta.y += nsEvent.deltaY;

                    const QPoint delta(
                        static_cast<int>(AZStd::trunc(m_accumulatedDelta.x)), static_cast<int>(AZStd::trunc(m_accumulatedDelta.y)));
                    m_accumulatedDelta.x -= delta.x();
                    m_accumulatedDelta.y -= delta.y();

                    if (!delta.isNull() && m_motionDeltaFn)
                    {
                        m_motionDeltaFn(delta);
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
                    else if (nsEvent.subtype == NSEventSubtypeApplicationActivated)
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
        void SetCursorAssociated(const bool associated)
        {
            if (associated != m_cursorAssociated)
            {
                m_cursorAssociated = associated;
                CGAssociateMouseAndMouseCursorPosition(associated);
            }
        }

        MotionDeltaFn m_motionDeltaFn;
        CGPoint m_accumulatedDelta = CGPointZero;
        bool m_active = false;
        bool m_cursorAssociated = true;
    };

    AZStd::unique_ptr<NativeCursorCapture> NativeCursorCapture::Create(MotionDeltaFn motionDeltaFn)
    {
        return AZStd::make_unique<NativeCursorCaptureMac>(AZStd::move(motionDeltaFn));
    }
} // namespace AzToolsFramework
