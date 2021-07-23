/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Gestures/IGestureRecognizer.h>

namespace Gestures
{
    class RecognizerMock
        : public IRecognizer
    {
    public:
        MOCK_CONST_METHOD0(GetPriority,
            int32_t());
        MOCK_METHOD2(OnPressedEvent,
            bool(const Vec2&screenPositionPixels, uint32_t pointerIndex));
        MOCK_METHOD2(OnDownEvent,
            bool(const Vec2&screenPositionPixels, uint32_t pointerIndex));
        MOCK_METHOD2(OnReleasedEvent,
            bool(const Vec2&screenPositionPixels, uint32_t pointerIndex));
    };
}
