/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/UiLayoutCellBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiLayoutCellInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiLayoutCellInterface() {}

    //! Get the overridden min width. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetMinWidth() = 0;

    //! Set the overridden min width. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetMinWidth(float width) = 0;

    //! Get the overridden min height. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetMinHeight() = 0;

    //! Set the overridden min height. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetMinHeight(float height) = 0;

    //! Get the overridden target width. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetTargetWidth() = 0;

    //! Set the overridden target width. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetTargetWidth(float width) = 0;

    //! Get the overridden target height. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetTargetHeight() = 0;

    //! Set the overridden target height. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetTargetHeight(float height) = 0;

    //! Get the overridden max width. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetMaxWidth() = 0;

    //! Set the overridden max width. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetMaxWidth(float width) = 0;

    //! Get the overridden max height. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetMaxHeight() = 0;

    //! Set the overridden max height. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetMaxHeight(float height) = 0;

    //! Get the overridden extra width ratio. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetExtraWidthRatio() = 0;

    //! Set the overridden extra width ratio. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetExtraWidthRatio(float width) = 0;

    //! Get the overridden extra height ratio. LyShine::UiLayoutCellUnspecifiedSize means it has not been overridden
    virtual float GetExtraHeightRatio() = 0;

    //! Set the overridden extra height ratio. LyShine::UiLayoutCellUnspecifiedSize means don't override
    virtual void SetExtraHeightRatio(float height) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiLayoutCellInterface> UiLayoutCellBus;
