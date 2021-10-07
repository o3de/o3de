/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <Tests/Printers.h>

#include <string>
#include <ostream>

namespace AZ
{
    void PrintTo(const AZ::Vector3& vector, ::std::ostream* os)
    {
        *os << '(' << vector.GetX() << ", " << vector.GetY() << ", " << vector.GetZ() << ')';
    }

    void PrintTo(const AZ::Quaternion& quaternion, ::std::ostream* os)
    {
        *os << '(' << quaternion.GetX() << ", " << quaternion.GetY() << ", " << quaternion.GetZ() << ", " << quaternion.GetW() << ')';
    }
} // namespace AZ

namespace AZStd
{
    void PrintTo(const string& string, ::std::ostream* os)
    {
        *os << '"' << std::string(string.data(), string.size()) << '"';
    }
} // namespace AZStd

namespace EMotionFX
{
    void PrintTo(const Transform& transform, ::std::ostream* os)
    {
        *os << "(pos: ";
        PrintTo(transform.m_position, os);
        *os << ", rot: ";
        PrintTo(transform.m_rotation, os);
#if !defined(EMFX_SCALE_DISABLED)
        *os << ", scale: ";
        PrintTo(transform.m_scale, os);
#endif
        *os << ")";
    }
} // namespace EMotionFX
