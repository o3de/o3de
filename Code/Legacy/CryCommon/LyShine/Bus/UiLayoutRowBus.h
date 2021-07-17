/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/Bus/UiLayoutBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiLayoutRowInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiLayoutRowInterface() {}

    //! Get the padding (in pixels) inside the edges of the element
    virtual UiLayoutInterface::Padding GetPadding() = 0;

    //! Set the padding (in pixels) inside the edges of the element
    virtual void SetPadding(UiLayoutInterface::Padding padding) = 0;

    //! Get the spacing (in pixels) between child elements
    virtual float GetSpacing() = 0;

    //! Set the spacing (in pixels) between child elements
    virtual void SetSpacing(float spacing) = 0;

    //! Get the horizontal order for this layout
    virtual UiLayoutInterface::HorizontalOrder GetOrder() = 0;

    //! Set the horizontal order for this layout
    virtual void SetOrder(UiLayoutInterface::HorizontalOrder order) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiLayoutRowInterface> UiLayoutRowBus;

