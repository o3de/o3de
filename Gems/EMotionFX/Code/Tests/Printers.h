/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <iosfwd>

#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <EMotionFX/Source/Transform.h>

namespace AZ
{
    void PrintTo(const AZ::Vector3& vector, ::std::ostream* os);
    void PrintTo(const AZ::Quaternion& quaternion, ::std::ostream* os);
} // namespace AZ

namespace AZStd
{
    void PrintTo(const string& string, ::std::ostream* os);
} // namespace AZStd

namespace EMotionFX
{
    void PrintTo(const Transform& transform, ::std::ostream* os);
} // namespace EMotionFX
