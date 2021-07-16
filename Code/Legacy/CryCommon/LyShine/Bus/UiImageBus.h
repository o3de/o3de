/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>

class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiImageInterface
    : public AZ::ComponentBus
{
public: // types

    enum class ImageType : int32_t
    {
        Stretched,      //!< the texture is stretched to fit the rect without maintaining aspect ratio
        Sliced,         //!< the texture is sliced such that center stretches and the edges do not
        Fixed,          //!< the texture is not stretched at all
        Tiled,          //!< the texture is tiled (repeated)
        StretchedToFit, //!< the texture is scaled to fit the rect while maintaining aspect ratio
        StretchedToFill //!< the texture is scaled to fill the rect while maintaining aspect ratio
    };

    enum class SpriteType : int32_t
    {
        SpriteAsset,
        RenderTarget,
    };

    enum class FillType : int32_t
    {
        None,           //!< the image is displayed fully filled
        Linear,         //!< the image is filled linearly from one edge to the opposing edge
        Radial,         //!< the image is filled radially around the center
        RadialCorner,   //!< the image is filled radially around a corner
        RadialEdge,     //!< the image is filled radially around the midpoint of an edge
    };

    enum class FillCornerOrigin : int32_t
    {
        TopLeft,
        TopRight,
        BottomRight,
        BottomLeft,
    };

    enum class FillEdgeOrigin : int32_t
    {
        Left,
        Top,
        Right,
        Bottom,
    };

public: // member functions

    virtual ~UiImageInterface() {}

    //! Gets the image color tint
    virtual AZ::Color GetColor() = 0;

    //! Sets the image color tint
    virtual void SetColor(const AZ::Color& color) = 0;

    //! Gets the image color alpha
    virtual float GetAlpha() = 0;

    //! Sets the image color alpha
    virtual void SetAlpha(float color) = 0;

    //! Gets the sprite for this element
    virtual ISprite* GetSprite() = 0;

    //! Sets the sprite for this element
    virtual void SetSprite(ISprite* sprite) = 0;

    //! Gets the source location of the image to be displayed by the element
    virtual AZStd::string GetSpritePathname() = 0;
    
    //! Sets the source location of the image to be displayed by the element
    virtual void SetSpritePathname(AZStd::string spritePath) = 0;

    //! Sets the source location of the image to be displayed by the element only
    //! if the sprite asset exists. Otherwise, the current sprite remains unchanged.
    //! Returns whether the sprite changed
    virtual bool SetSpritePathnameIfExists(AZStd::string spritePath) = 0;

    //! Gets the name of the render target
    virtual AZStd::string GetRenderTargetName() = 0;

    //! Sets the name of the render target
    virtual void SetRenderTargetName(AZStd::string renderTargetName) = 0;

    //! Gets whether the render target is in sRGB color space
    virtual bool GetIsRenderTargetSRGB() = 0;

    //! Sets whether the render target is in sRGB color space
    virtual void SetIsRenderTargetSRGB(bool isSRGB) = 0;

    //! Gets the type of the sprite
    virtual SpriteType GetSpriteType() = 0;

    //! Sets the type of the sprite
    virtual void SetSpriteType(SpriteType spriteType) = 0;

    //! Gets the type of the image
    virtual ImageType GetImageType() = 0;

    //! Sets the type of the image
    virtual void SetImageType(ImageType imageType) = 0;
    
    //! Gets the fill type for the image
    virtual FillType GetFillType() = 0;

    //! Sets the fill type for the image
    virtual void SetFillType(FillType fillType) = 0;

    //! Gets the fill amount for the image in the range [0,1]
    virtual float GetFillAmount() = 0;

    //! Sets the fill amount for the image in the range [0,1]
    virtual void SetFillAmount(float fillAmount) = 0;

    //! Gets the start angle for radial fill, measured clockwise in degrees from straight up
    virtual float GetRadialFillStartAngle() = 0;

    //! Sets the start angle for radial fill, measured clockwise in degrees from straight up
    virtual void SetRadialFillStartAngle(float radialFillStartAngle) = 0;

    //! Gets the corner fill origin
    virtual FillCornerOrigin GetCornerFillOrigin() = 0;

    //! Sets the corner fill origin
    virtual void SetCornerFillOrigin(FillCornerOrigin cornerOrigin) = 0;

    //! Gets the edge fill origin
    virtual FillEdgeOrigin GetEdgeFillOrigin() = 0;

    //! Sets the edge fill origin
    virtual void SetEdgeFillOrigin(FillEdgeOrigin edgeOrigin) = 0;

    //! Gets whether the image is filled clockwise
    virtual bool GetFillClockwise() = 0;

    //! Sets whether the image is filled clockwise
    virtual void SetFillClockwise(bool fillClockwise) = 0;

    //! Gets whether the center of a sliced image is filled
    virtual bool GetFillCenter() = 0;

    //! Sets whether the center of a sliced image is filled
    virtual void SetFillCenter(bool fillCenter) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiImageInterface> UiImageBus;

