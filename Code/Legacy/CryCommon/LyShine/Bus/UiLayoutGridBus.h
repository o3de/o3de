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
#include <LyShine/Bus/UiLayoutBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiLayoutGridInterface
    : public AZ::ComponentBus
{
public: // types

    //! Used to determine which direction to start the layout with
    enum class StartingDirection
    {
        HorizontalOrder,
        VerticalOrder
    };

public: // member functions

    virtual ~UiLayoutGridInterface() {}

    //! Get the padding (in pixels) inside the edges of the element
    virtual UiLayoutInterface::Padding GetPadding() = 0;

    //! Set the padding (in pixels) inside the edges of the element
    virtual void SetPadding(UiLayoutInterface::Padding padding) = 0;

    //! Get the spacing (in pixels) between child elements
    virtual AZ::Vector2 GetSpacing() = 0;

    //! Set the spacing (in pixels) between child elements
    virtual void SetSpacing(AZ::Vector2 spacing) = 0;

    //! Get the size (in pixels) of a child element in this layout
    virtual AZ::Vector2 GetCellSize() = 0;

    //! Set the size (in pixels) of a child element in this layout
    virtual void SetCellSize(AZ::Vector2 size) = 0;

    //! Get the horizontal order for this layout
    virtual UiLayoutInterface::HorizontalOrder GetHorizontalOrder() = 0;

    //! Set the horizontal order for this layout
    virtual void SetHorizontalOrder(UiLayoutInterface::HorizontalOrder order) = 0;

    //! Get the vertical order for this layout
    virtual UiLayoutInterface::VerticalOrder GetVerticalOrder() = 0;

    //! Set the vertical order for this layout
    virtual void SetVerticalOrder(UiLayoutInterface::VerticalOrder order) = 0;

    //! Get the starting direction for this layout
    virtual StartingDirection GetStartingDirection() = 0;

    //! Set the starting direction for this layout
    virtual void SetStartingDirection(StartingDirection direction) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiLayoutGridInterface> UiLayoutGridBus;

