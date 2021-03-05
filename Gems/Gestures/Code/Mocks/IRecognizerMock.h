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