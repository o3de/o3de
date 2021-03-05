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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

namespace WhiteBox
{
    //! Returns the index of the largest element in the vector
    int FindLargestElement(const AZ::Vector3& vector);

    //! Creates UV coordinates from vertex positioning on closest basis vector p
    AZ::Vector2 CreatePlanarUVFromVertex(
        const AZ::Vector3& normal, const AZ::Vector3& position, const AZ::Vector2& offset = AZ::Vector2(0.5f, 0.5f),
        const AZ::Vector2& scale = AZ::Vector2(-1.0f, -1.0f));

} // namespace WhiteBox
