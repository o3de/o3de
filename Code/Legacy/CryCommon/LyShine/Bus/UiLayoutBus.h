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
#include <LyShine/IDraw2d.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiLayoutInterface
    : public AZ::ComponentBus
{
public: // types

    //! Horizontal order used by layout components
    enum class HorizontalOrder
    {
        LeftToRight,
        RightToLeft
    };

    //! Vertical order used by layout components
    enum class VerticalOrder
    {
        TopToBottom,
        BottomToTop
    };

    //! Padding (in pixels) inside the edges of an element
    struct Padding
    {
        AZ_TYPE_INFO(Padding, "{DE5C18B0-4214-4A37-B590-8D45CC450A96}")

        Padding()
            : m_left(0)
            , m_top(0)
            , m_right(0)
            , m_bottom(0) {}

        int m_left;
        int m_right;
        int m_top;
        int m_bottom;
    };

public: // member functions

    virtual ~UiLayoutInterface() {}

    //! Get whether this layout component uses layout cells to calculate its layout
    virtual bool IsUsingLayoutCellsToCalculateLayout() = 0;

    //! Get whether this layout component should bypass the default layout cell values calculated by its children
    virtual bool GetIgnoreDefaultLayoutCells() = 0;

    //! Set whether this layout component should bypass the default layout cell values calculated by its children
    virtual void SetIgnoreDefaultLayoutCells(bool ignoreDefaultLayoutCells) = 0;

    //! Get the horizontal child alignment
    virtual IDraw2d::HAlign GetHorizontalChildAlignment() = 0;

    //! Set the horizontal child alignment
    virtual void SetHorizontalChildAlignment(IDraw2d::HAlign alignment) = 0;

    //! Get the vertical child alignment
    virtual IDraw2d::VAlign GetVerticalChildAlignment() = 0;

    //! Set the vertical child alignment
    virtual void SetVerticalChildAlignment(IDraw2d::VAlign alignment) = 0;

    //! Find out whether this layout component is currently overriding the transform of the specified element.
    virtual bool IsControllingChild(AZ::EntityId childId) = 0;

    //! Get the size the element needs to be to fit a specified number of child elements of a certain size
    virtual AZ::Vector2 GetSizeToFitChildElements(const AZ::Vector2& childElementSize, int numChildElements) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiLayoutInterface> UiLayoutBus;
