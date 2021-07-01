/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <MCore/Source/Quaternion.h>
#include <MCore/Source/Compare.h>
#include <Tests/Printers.h>
#include <AzCore/std/string/string.h>

inline testing::PolymorphicMatcher<testing::internal::StrEqualityMatcher<AZStd::string> >
StrEq(const AZStd::string& str)
{
    return ::testing::MakePolymorphicMatcher(testing::internal::StrEqualityMatcher<AZStd::string>(
      str, true, true));
}

AZ_PUSH_DISABLE_WARNING(4100, "-Wmissing-declarations") // 'result_listener': unreferenced formal parameter
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

    return false;
}

template<>
template<>
inline bool IsCloseMatcherP<MCore::Quaternion>::gmock_Impl<const MCore::Quaternion&>::MatchAndExplain(const MCore::Quaternion& arg, ::testing::MatchResultListener* result_listener) const
{
    const MCore::Quaternion compareQuat = (expected.Dot(arg) < 0.0f) ? -arg : arg;
    const AZ::Vector4 compareVec4(compareQuat.x, compareQuat.y, compareQuat.z, compareQuat.w);

    if (::testing::ExplainMatchResult(IsClose(AZ::Vector4(expected.x, expected.y, expected.z, expected.w)), compareVec4, result_listener))
    {
        return true;
    }

    AZ::Vector3 gotAxis;
    AZ::Vector3 expectedAxis;
    float gotAngle;
    float expectedAngle;

    // convert to an axis and angle representation
    expected.ToAxisAngle(&expectedAxis, &expectedAngle);
    compareQuat.ToAxisAngle(&gotAxis, &gotAngle);

    *result_listener << "\n     Got Axis: ";
    PrintTo(gotAxis, result_listener->stream());
    *result_listener << ", Got Angle: " << gotAngle << "\n";
    *result_listener << "Expected Axis: ";
    PrintTo(expectedAxis, result_listener->stream());
    *result_listener << ", Expected Angle: " << expectedAngle;

    return false;
}

template<>
template<>
inline bool IsCloseMatcherP<EMotionFX::Transform>::gmock_Impl<const EMotionFX::Transform&>::MatchAndExplain(const EMotionFX::Transform& arg, ::testing::MatchResultListener* result_listener) const
{
#ifndef EMFX_SCALE_DISABLED
    return ::testing::ExplainMatchResult(IsClose(expected.mPosition), arg.mPosition, result_listener)
        && ::testing::ExplainMatchResult(IsClose(expected.mRotation), arg.mRotation, result_listener)
        && ::testing::ExplainMatchResult(IsClose(expected.mScale), arg.mScale, result_listener);
#else
    return ::testing::ExplainMatchResult(IsClose(expected.mPosition), arg.mPosition, result_listener)
        && ::testing::ExplainMatchResult(IsClose(expected.mRotation), arg.mRotation, result_listener);
#endif
}

template<>
template<>
inline bool IsCloseMatcherP<MCore::Matrix>::gmock_Impl<const MCore::Matrix&>::MatchAndExplain(const MCore::Matrix& arg, ::testing::MatchResultListener* result_listener) const
{
    using ::testing::FloatEq;
    using ::testing::ExplainMatchResult;
    return ExplainMatchResult(FloatEq(expected.m16[0]), arg.m16[0], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[1]), arg.m16[1], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[2]), arg.m16[2], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[3]), arg.m16[3], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[4]), arg.m16[4], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[5]), arg.m16[5], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[6]), arg.m16[6], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[7]), arg.m16[7], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[8]), arg.m16[8], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[9]), arg.m16[9], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[10]), arg.m16[10], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[11]), arg.m16[11], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[12]), arg.m16[12], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[13]), arg.m16[13], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[14]), arg.m16[14], result_listener)
        && ExplainMatchResult(FloatEq(expected.m16[15]), arg.m16[15], result_listener);
}
