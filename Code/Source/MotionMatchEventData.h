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

#include <EMotionFX/Source/EventData.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace MotionMatching
    {
        class EMFX_API MotionMatchEventData
            : public EventData
        {
        public:       
            AZ_RTTI(MotionMatchEventData, "{25499823-E611-4958-85B7-476BC1918744}", EventData);
            AZ_CLASS_ALLOCATOR_DECL

            MotionMatchEventData() = default;
            ~MotionMatchEventData() override = default;

            static void Reflect(AZ::ReflectContext* context);

            bool GetDiscardFrames() const;

            bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const override;

        private:
            AZStd::string m_tag;
        };
    } // namespace MotionMatching
} // namespace EMotionFX
