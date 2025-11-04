/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <gmock/gmock.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Transform.h>
#include <MCore/Source/Compare.h>
#include <Tests/Printers.h>
#include <AzCore/std/string/string.h>

inline testing::PolymorphicMatcher<testing::internal::StrEqualityMatcher<AZStd::string> >
StrEq(const AZStd::string& str)
{
    return ::testing::MakePolymorphicMatcher(testing::internal::StrEqualityMatcher<AZStd::string>(
      str, true, true));
}

AZ_PUSH_DISABLE_WARNING(4100 4324, "-Wmissing-declarations") // 'result_listener': unreferenced formal parameter, structure was padded due to alignment specifier
MATCHER(StrEq, "")
{
    const auto& lhs = testing::get<0>(arg);
    const auto& rhs = testing::get<1>(arg);
    return ::testing::ExplainMatchResult(StrEq(AZStd::string(rhs.data(), rhs.size())), lhs, result_listener);
}

MATCHER(IsClose, "")
{
    using ::testing::get;
    return get<0>(arg).IsClose(get<1>(arg), 0.001f);
}

MATCHER_P(IsClose, expected, "")
{
    return arg.IsClose(expected, 0.001f);
}
AZ_POP_DISABLE_WARNING

template<>
template<>
inline bool IsCloseMatcherP<AZ::Quaternion>::gmock_Impl<const AZ::Quaternion&>::MatchAndExplain(const AZ::Quaternion& arg, ::testing::MatchResultListener* result_listener) const
{
    const AZ::Quaternion compareQuat = (expected.Dot(arg) < 0.0f) ? -arg : arg;
    const AZ::Vector4 compareVec4(compareQuat.GetX(), compareQuat.GetY(), compareQuat.GetZ(), compareQuat.GetW());

    if (::testing::ExplainMatchResult(IsClose(AZ::Vector4(expected.GetX(), expected.GetY(), expected.GetZ(), expected.GetW())), compareVec4, result_listener))
    {
        return true;
    }

    // its possible for the stream to be NULL if the result listener "is not interested" in explanation.
    if ((result_listener) && (result_listener->IsInterested()))
    {
        AZ::Vector3 gotAxis;
        AZ::Vector3 expectedAxis;
        float gotAngle;
        float expectedAngle;

        // convert to an axis and angle representation
        expected.ConvertToAxisAngle(expectedAxis, expectedAngle);
        compareQuat.ConvertToAxisAngle(gotAxis, gotAngle);

        *result_listener << "\n     Got Axis: ";
        PrintTo(gotAxis, result_listener->stream());
        *result_listener << ", Got Angle: " << gotAngle << "\n";
        *result_listener << "Expected Axis: ";
        PrintTo(expectedAxis, result_listener->stream());
        *result_listener << ", Expected Angle: " << expectedAngle;
    }
    return false;
}

template<>
template<>
inline bool IsCloseMatcherP<EMotionFX::Transform>::gmock_Impl<const EMotionFX::Transform&>::MatchAndExplain(const EMotionFX::Transform& arg, ::testing::MatchResultListener* result_listener) const
{
#ifndef EMFX_SCALE_DISABLED
    return ::testing::ExplainMatchResult(IsClose(expected.m_position), arg.m_position, result_listener)
        && ::testing::ExplainMatchResult(IsClose(expected.m_rotation), arg.m_rotation, result_listener)
        && ::testing::ExplainMatchResult(IsClose(expected.m_scale), arg.m_scale, result_listener);
#else
    return ::testing::ExplainMatchResult(IsClose(expected.m_position), arg.m_position, result_listener)
        && ::testing::ExplainMatchResult(IsClose(expected.m_rotation), arg.m_rotation, result_listener);
#endif
}
