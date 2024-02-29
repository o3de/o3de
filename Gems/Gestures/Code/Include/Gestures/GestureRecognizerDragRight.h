/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// Add the DragRight gesture
#if defined(CARBONATED)

#include "IGestureRecognizer.h"

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Time/ITime.h>

#include "GestureRecognizerDrag.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class RecognizerDragRight : public RecognizerDrag
    {
    public:
        static float GetDefaultMinSecondsHeld() { return 0.0f; }
        static float GetDefaultMinPixelsMoved() { return 20.0f; }
        static uint32_t GetDefaultPointerIndex() { return 1; }
        static int32_t GetDefaultPriority() { return AzFramework::InputChannelEventListener::GetPriorityUI() + 1; }

        struct Config : public RecognizerDrag::Config
        {
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator);
            AZ_RTTI(Config, "{E8D7DAF7-5D96-4255-9FF1-CCD05F902AC4}", RecognizerDrag::Config);
            static void Reflect(AZ::ReflectContext* context);

            Config()
            {
                minSecondsHeld = GetDefaultMinSecondsHeld();
                minPixelsMoved = GetDefaultMinPixelsMoved();
                pointerIndex = GetDefaultPointerIndex();
                priority = GetDefaultPriority();
            }
            virtual ~Config() = default;
        };
        static const Config& GetDefaultConfig() { static Config s_cfg; return s_cfg; }

        AZ_CLASS_ALLOCATOR(RecognizerDragRight, AZ::SystemAllocator);
        AZ_RTTI(RecognizerDragRight, "{98764FEB-B996-4EA3-BA24-B360F4038B8E}", RecognizerDrag);

        explicit RecognizerDragRight(const Config& config = GetDefaultConfig());
        ~RecognizerDragRight() override;

    private:
        Config m_config;
    };
}

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <Gestures/GestureRecognizerDragRight.inl>

#endif
