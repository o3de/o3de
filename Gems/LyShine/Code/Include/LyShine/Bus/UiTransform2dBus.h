/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <CryCommon/Cry_Color.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTransform2dInterface
    : public AZ::ComponentBus
{
public: // types

    // Anchors members are always in range 0-1, they are normalized positions within the parent elements bounds
    struct Anchors
    {
        AZ_TYPE_INFO(Anchors, "{65D4346C-FB16-4CB0-9BDC-1185B122C4A9}")

        Anchors()
            : m_left(0.5f)
            , m_top(0.5f)
            , m_right(0.5f)
            , m_bottom(0.5f) {}

        Anchors(float left, float top, float right, float bottom)
            : m_left(left)
            , m_top(top)
            , m_right(right)
            , m_bottom(bottom) {}

        float   m_left;
        float   m_top;
        float   m_right;
        float   m_bottom;

        void UnitClamp()
        {
            m_left = AZStd::clamp(m_left, 0.0f, 1.0f);
            m_top = AZStd::clamp(m_top, 0.0f, 1.0f);
            m_right = AZStd::clamp(m_right, 0.0f, 1.0f);
            m_bottom = AZStd::clamp(m_bottom, 0.0f, 1.0f);
        }

        bool operator==(const Anchors& rhs) const
        {
            return (m_left == rhs.m_left &&
                    m_right == rhs.m_right &&
                    m_top == rhs.m_top &&
                    m_bottom == rhs.m_bottom);
        }

        bool operator!=(const Anchors& rhs) const
        {
            return !(*this == rhs);
        }
    };

    // Offsets are in pixels or physical units and are offsets from the anchors
    // The left offset is the offset from the left anchor to the left edge of this UiElement
    struct Offsets
    {
        AZ_TYPE_INFO(Offsets, "{F681BA9D-245C-4630-B20E-05DD752FAD57}")

        Offsets()
            : m_left(-50)
            , m_top(-50)
            , m_right(50)
            , m_bottom(50) {}

        Offsets(float left, float top, float right, float bottom)
            : m_left(left)
            , m_top(top)
            , m_right(right)
            , m_bottom(bottom) {}

        float   m_left;
        float   m_top;
        float   m_right;
        float   m_bottom;

        Offsets& operator+=(const UiTransformInterface::RectPoints& rhs)
        {
            m_left += rhs.TopLeft().GetX();
            m_right += rhs.BottomRight().GetX();
            m_top += rhs.TopLeft().GetY();
            m_bottom += rhs.BottomRight().GetY();

            return *this;
        }

        Offsets operator+(const UiTransformInterface::RectPoints& rhs) const
        {
            Offsets result(*this);

            result.m_left += rhs.TopLeft().GetX();
            result.m_right += rhs.BottomRight().GetX();
            result.m_top += rhs.TopLeft().GetY();
            result.m_bottom += rhs.BottomRight().GetY();

            return result;
        }

        Offsets& operator+=(const AZ::Vector2& rhs)
        {
            m_left += rhs.GetX();
            m_right += rhs.GetX();
            m_top += rhs.GetY();
            m_bottom += rhs.GetY();

            return *this;
        }

        Offsets operator+(const AZ::Vector2& rhs) const
        {
            Offsets result(*this);

            result.m_left += rhs.GetX();
            result.m_right += rhs.GetX();
            result.m_top += rhs.GetY();
            result.m_bottom += rhs.GetY();

            return result;
        }

        bool operator==(const Offsets& rhs) const
        {
            return (m_left == rhs.m_left &&
                    m_right == rhs.m_right &&
                    m_top == rhs.m_top &&
                    m_bottom == rhs.m_bottom);
        }

        bool operator!=(const Offsets& rhs) const
        {
            return !(*this == rhs);
        }
    };

public: // member functions

    virtual ~UiTransform2dInterface() {}

    //! Get the anchors for the element
    virtual Anchors GetAnchors() = 0;

    //! Set the anchors for the element
    //! \param adjustOffsets    If true the offsets are adjusted to keep the rect in the same position
    //! \param allowPush        Only has effect if the anchors are invalid. If true changing an anchor
    //                          to overlap its opposite anchor, will move the opposite anchor
    virtual void SetAnchors(Anchors anchors, bool adjustOffsets, bool allowPush) = 0;

    //! Get the offsets for the element
    virtual Offsets GetOffsets() = 0;

    //! Set the offsets for the element
    virtual void SetOffsets(Offsets offsets) = 0;

    //! Set the pivot and adjust the offsets to stay in same place
    virtual void SetPivotAndAdjustOffsets(AZ::Vector2 pivot) = 0;

    //! Modify left and right offsets relative the element's anchors
    virtual void SetLocalWidth(float width) = 0;

    //! Get the height of the element based off it's offsets
    virtual float GetLocalWidth() = 0;

    //! Modify top and bottom offsets relative the element's anchors
    virtual void SetLocalHeight(float height) = 0;

    //! Get the height of the element based off it's offsets
    virtual float GetLocalHeight() = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiTransform2dInterface> UiTransform2dBus;
