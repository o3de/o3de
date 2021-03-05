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

#include <AzCore/EBus/EBus.h>

#include <AzCore/Math/Vector2.h>

class UiCursorInterface
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    // Public functions

    //! Increment the UI Cursor visible counter.
    //! Should be paired with a call to DecrementVisibleCounter.
    virtual void IncrementVisibleCounter() = 0;

    //! Decrement the UI Cursor visible counter.
    //! Should be paired with a call to IncrementVisibleCounter.
    virtual void DecrementVisibleCounter() = 0;

    //! Query whether the UI Cursor is currently visible
    virtual bool IsUiCursorVisible() = 0;

    //! Set the UI cursor image.
    virtual void SetUiCursor(const char* cursorImagePath) = 0;

    //! Get the UI cursor position (in pixels) relative
    //! to the top-left corner of the UI overlay viewport
    virtual AZ::Vector2 GetUiCursorPosition() = 0;
};

using UiCursorBus = AZ::EBus<UiCursorInterface>;

