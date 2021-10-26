/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Class for 2D drawing in screen space
//
//! The IDraw2d interface allows drawing images and text in 2D.
//! Positions and sizes are specified in pixels in the current 2D viewport.
//! The BeginDraw2d method should be called before calling the Draw methods to enter 2D mode
//! and the EndDraw2d method should be called after calling the Draw methods to exit 2D mode.
//! There is a helper class Draw2dHelper that encapsulates this in its constructor and destructor.
class IDraw2d
{
public: // types

    //! Horizontal alignment can be used for both text and image drawing
    enum class HAlign
    {
        Left,
        Center,
        Right,
    };

    //! Vertical alignment can be used for both text and image drawing
    enum class VAlign
    {
        Top,
        Center,
        Bottom,
    };

    //! Used for specifying how to round positions to an exact pixel position for pixel-perfect rendering
    enum class Rounding
    {
        None,
        Nearest,
        Down,
        Up
    };

    enum
    {
        //! Limit imposed by FFont. This is the max number of characters including the null terminator.
        MAX_TEXT_STRING_LENGTH = 1024,
    };

public: // member functions

    //! Implement virtual destructor just for safety.
    virtual ~IDraw2d() {}
};
