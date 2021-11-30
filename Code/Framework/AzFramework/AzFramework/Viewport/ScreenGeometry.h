/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/base.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace AzFramework
{
    //! A wrapper around an X and Y screen position.
    struct ScreenPoint
    {
        AZ_TYPE_INFO(ScreenPoint, "{8472B6C2-527F-44FC-87F8-C226B1A57A97}");
        ScreenPoint() = default;

        constexpr ScreenPoint(int x, int y)
            : m_x(x)
            , m_y(y)
        {
        }

        int m_x; //!< X screen position.
        int m_y; //!< Y screen position.
    };

    //! A wrapper around an X and Y screen vector.
    //! A ScreenVector only represents a movement (delta). ScreenVectors can be added to
    //! ScreenPoints to return a new ScreenPoint, and ScreenPoints can be subtracted from
    //! each other to give a ScreenVector, but two ScreenPoints cannot be added together.
    struct ScreenVector
    {
        AZ_TYPE_INFO(ScreenVector, "{1EAA2C62-8FDB-4A28-9FE3-1FA4F1418894}");
        ScreenVector() = default;

        constexpr ScreenVector(int x, int y)
            : m_x(x)
            , m_y(y)
        {
        }

        int m_x; //!< X screen delta.
        int m_y; //!< Y screen delta.
    };

    //! A wrapper around a screen width and height.
    struct ScreenSize
    {
        AZ_TYPE_INFO(ScreenSize, "{26D28916-6E8E-44B8-83F9-C44BCDA370E2}");
        ScreenSize() = default;

        constexpr ScreenSize(int width, int height)
            : m_width(width)
            , m_height(height)
        {
        }

        int m_width; //!< Screen size width.
        int m_height; //!< Screen size height.
    };

    void ScreenGeometryReflect(AZ::ReflectContext* context);

    inline const ScreenVector operator-(const ScreenPoint& lhs, const ScreenPoint& rhs)
    {
        return ScreenVector(lhs.m_x - rhs.m_x, lhs.m_y - rhs.m_y);
    }

    inline ScreenPoint& operator+=(ScreenPoint& lhs, const ScreenVector& rhs)
    {
        lhs.m_x += rhs.m_x;
        lhs.m_y += rhs.m_y;
        return lhs;
    }

    inline ScreenPoint& operator-=(ScreenPoint& lhs, const ScreenVector& rhs)
    {
        lhs.m_x -= rhs.m_x;
        lhs.m_y -= rhs.m_y;
        return lhs;
    }

    inline const ScreenPoint operator+(const ScreenPoint& lhs, const ScreenVector& rhs)
    {
        ScreenPoint result{ lhs };
        result += rhs;
        return result;
    }

    inline const ScreenPoint operator-(const ScreenPoint& lhs, const ScreenVector& rhs)
    {
        ScreenPoint result{ lhs };
        result -= rhs;
        return result;
    }

    inline ScreenVector& operator+=(ScreenVector& lhs, const ScreenVector& rhs)
    {
        lhs.m_x += rhs.m_x;
        lhs.m_y += rhs.m_y;
        return lhs;
    }

    inline ScreenVector& operator-=(ScreenVector& lhs, const ScreenVector& rhs)
    {
        lhs.m_x -= rhs.m_x;
        lhs.m_y -= rhs.m_y;
        return lhs;
    }

    inline const ScreenVector operator+(const ScreenVector& lhs, const ScreenVector& rhs)
    {
        ScreenVector result{ lhs };
        result += rhs;
        return result;
    }

    inline const ScreenVector operator-(const ScreenVector& lhs, const ScreenVector& rhs)
    {
        ScreenVector result{ lhs };
        result -= rhs;
        return result;
    }

    inline const bool operator==(const ScreenPoint& lhs, const ScreenPoint& rhs)
    {
        return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y;
    }

    inline const bool operator!=(const ScreenPoint& lhs, const ScreenPoint& rhs)
    {
        return !operator==(lhs, rhs);
    }

    inline const bool operator==(const ScreenVector& lhs, const ScreenVector& rhs)
    {
        return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y;
    }

    inline const bool operator!=(const ScreenVector& lhs, const ScreenVector& rhs)
    {
        return !operator==(lhs, rhs);
    }

    inline const bool operator==(const ScreenSize& lhs, const ScreenSize& rhs)
    {
        return lhs.m_width == rhs.m_width && lhs.m_height == rhs.m_height;
    }

    inline const bool operator!=(const ScreenSize& lhs, const ScreenSize& rhs)
    {
        return !operator==(lhs, rhs);
    }

    inline ScreenVector& operator*=(ScreenVector& lhs, const float rhs)
    {
        lhs.m_x = aznumeric_cast<int>(AZStd::lround(aznumeric_cast<float>(lhs.m_x) * rhs));
        lhs.m_y = aznumeric_cast<int>(AZStd::lround(aznumeric_cast<float>(lhs.m_y) * rhs));
        return lhs;
    }

    inline const ScreenVector operator*(const ScreenVector& lhs, const float rhs)
    {
        ScreenVector result{ lhs };
        result *= rhs;
        return result;
    }

    inline ScreenSize& operator*=(ScreenSize& lhs, const float rhs)
    {
        lhs.m_width = aznumeric_cast<int>(AZStd::lround(aznumeric_cast<float>(lhs.m_width) * rhs));
        lhs.m_height = aznumeric_cast<int>(AZStd::lround(aznumeric_cast<float>(lhs.m_height) * rhs));
        return lhs;
    }

    inline const ScreenSize operator*(const ScreenSize& lhs, const float rhs)
    {
        ScreenSize result{ lhs };
        result *= rhs;
        return result;
    }

    inline float ScreenVectorLength(const ScreenVector& screenVector)
    {
        return aznumeric_cast<float>(AZStd::sqrt(screenVector.m_x * screenVector.m_x + screenVector.m_y * screenVector.m_y));
    }

    //! Return an AZ::Vector2 from a ScreenPoint.
    inline AZ::Vector2 Vector2FromScreenPoint(const ScreenPoint& screenPoint)
    {
        return AZ::Vector2(aznumeric_cast<float>(screenPoint.m_x), aznumeric_cast<float>(screenPoint.m_y));
    }

    //! Return an AZ::Vector2 from a ScreenVector.
    inline AZ::Vector2 Vector2FromScreenVector(const ScreenVector& screenVector)
    {
        return AZ::Vector2(aznumeric_cast<float>(screenVector.m_x), aznumeric_cast<float>(screenVector.m_y));
    }

    //! Return an AZ::Vector2 from a ScreenSize.
    inline AZ::Vector2 Vector2FromScreenSize(const ScreenSize& screenSize)
    {
        return AZ::Vector2(aznumeric_cast<float>(screenSize.m_width), aznumeric_cast<float>(screenSize.m_height));
    }

    //! Return a ScreenPoint from an AZ::Vector2.
    inline ScreenPoint ScreenPointFromVector2(const AZ::Vector2& vector2)
    {
        return ScreenPoint(aznumeric_cast<int>(AZStd::lround(vector2.GetX())), aznumeric_cast<int>(AZStd::lround(vector2.GetY())));
    }

    //! Return a ScreenVector from an AZ::Vector2.
    inline ScreenVector ScreenVectorFromVector2(const AZ::Vector2& vector2)
    {
        return ScreenVector(aznumeric_cast<int>(AZStd::lround(vector2.GetX())), aznumeric_cast<int>(AZStd::lround(vector2.GetY())));
    }

    //! Return a ScreenSize from an AZ::Vector2.
    inline ScreenSize ScreenSizeFromVector2(const AZ::Vector2& vector2)
    {
        return ScreenSize(aznumeric_cast<int>(AZStd::lround(vector2.GetX())), aznumeric_cast<int>(AZStd::lround(vector2.GetY())));
    }
} // namespace AzFramework
