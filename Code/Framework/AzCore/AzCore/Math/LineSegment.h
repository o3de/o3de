/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace O3DE
{
    class LineSegment
    {
    public:
        AZ_TYPE_INFO(LineSegment, "{7557da1e-cc20-11ec-9d64-0242ac120002}");

        explicit LineSegment(const AZ::Vector3& start, const AZ::Vector3& end);

        const AZ::Vector3& GetStart() const;
        const AZ::Vector3& getEnd() const;

        const AZ::Vector3 Get();
        
        LineSegment& operator=(const LineSegment& rhs);
        bool operator==(const LineSegment& rhs) const;
        bool operator!=(const LineSegment& rhs) const;

    private:
        AZ::Vector3 m_start;
        AZ::Vector3 m_end;
    };

} // namespace O3DE