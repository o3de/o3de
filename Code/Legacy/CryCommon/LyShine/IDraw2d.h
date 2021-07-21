/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

// Forward declarations
struct IFFont;

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

    enum : int
    {
        //! Constant that indicates the built-in default value should be used
        UseDefault = -1
    };

    //! Struct used to pass additional image options.
    //
    //! If this is not passed then the defaults below are used
    struct ImageOptions
    {
        int         blendMode;      //!< default is GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA
        AZ::Vector3 color;          //!< default is (1,1,1)
        Rounding    pixelRounding;  //!< default is Rounding::Nearest
        int         baseState;      //!< Additional flags for SetState. Default is GS_NODEPTHTEST
    };

    //! Struct used to pass additional text options - mostly ones that do not change from call to call.
    //
    //! If this is not passed then the defaults below are used
    struct TextOptions
    {
        AZStd::string   fontName;               //!< default is "default"
        unsigned int    effectIndex;            //!< default is 0
        AZ::Vector3     color;                  //!< default is (1,1,1)
        HAlign          horizontalAlignment;    //!< default is HAlign::Left
        VAlign          verticalAlignment;      //!< default is VAlign::Top
        AZ::Vector2     dropShadowOffset;       //!< default is (0,0), zero offset means no drop shadow is drawn
        AZ::Color       dropShadowColor;        //!< default is (0,0,0,0), zero alpha means no drop shadow is drawn
        float           rotation;               //!< default is 0
        int             baseState;              //!< Additional flags for SetState. Default is GS_NODEPTHTEST
    };

    //! Used to pass in arrays of vertices (e.g. to DrawQuad)
    struct VertexPosColUV
    {
        VertexPosColUV(){}
        VertexPosColUV(const AZ::Vector2& inPos, const AZ::Color& inColor, const AZ::Vector2& inUV)
        {
            position = inPos;
            color = inColor;
            uv = inUV;
        }

        AZ::Vector2         position;   //!< 2D position of vertex
        AZ::Color           color;      //!< Float color
        AZ::Vector2         uv;         //!< Texture coordinate
    };

public: // member functions

    //! Implement virtual destructor just for safety.
    virtual ~IDraw2d() {}
};
