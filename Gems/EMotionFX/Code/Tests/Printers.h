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

#include <iosfwd>

#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <MCore/Source/Quaternion.h>
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

namespace MCore
{
    void PrintTo(const Quaternion& quaternion, ::std::ostream* os);
} // namespace MCore

namespace EMotionFX
{
    void PrintTo(const Transform& transform, ::std::ostream* os);
} // namespace EMotionFX
