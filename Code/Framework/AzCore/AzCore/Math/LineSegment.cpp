/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/LineSegment.h>

namespace O3DE
{
    LineSegment::LineSegment(const AZ::Vector3& start, const AZ::Vector3& end):
        m_start(start),
        m_end(end)
    {
    }

    const AZ::Vector3& LineSegment::GetStart() const
    {
        return m_start;
    }
    const AZ::Vector3& LineSegment::GetEnd() const
    {
        return m_end;
    }

        
    LineSegment& LineSegment::operator=(const LineSegment& rhs) {
        m_start = rhs.m_start;
        m_end = rhs.m_end;
        return (*this);
    }

    bool LineSegment::operator==(const LineSegment& rhs) const
    {
        return m_start == rhs.m_start && m_end == rhs.m_end;
    }
    bool LineSegment::operator!=(const LineSegment& rhs) const
    {
        return m_start != rhs.m_start && m_end != rhs.m_end;
    }

} // namespace O3DE
