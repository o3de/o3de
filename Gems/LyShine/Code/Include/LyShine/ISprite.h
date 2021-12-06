/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SerializeFwd.h>
#include <smartptr.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! A sprite is a texture with extra information about how it behaves for 2D drawing
//! Currently a sprite exists on disk as a side car file next to the texture file.
class ISprite
    : public _i_reference_target<int>
{
public: // types

    // The borders define the areas of the sprite that stretch
    // Border's members are always in range 0-1, they are normalized positions within the texture bounds
    struct Borders
    {
        Borders()
            : m_left(0.0f)
            , m_right(1.0f)
            , m_top(0.0f)
            , m_bottom(1.0f) {}

        Borders(float left, float right, float top, float bottom)
            : m_left(left)
            , m_right(right)
            , m_top(top)
            , m_bottom(bottom) {}

        float   m_left;
        float   m_right;
        float   m_top;
        float   m_bottom;
    };

    //! \brief Defines the UV-extents of a particular "cell" in a sprite-sheet.
    //! 9-slice information for the cell is also stored.
    struct SpriteSheetCell
    {
        AZStd::string alias;
        UiTransformInterface::RectPoints uvCellCoords;
        Borders borders;
    };

    using SpriteSheetCellContainer = AZStd::vector<SpriteSheetCell>;

public: // member functions

    //! Deleting a sprite will release its texture
    virtual ~ISprite() {}

    //! Get the pathname of this sprite
    virtual const AZStd::string& GetPathname() const = 0;

    //! Get the pathname of the texture of this sprite
    virtual const AZStd::string& GetTexturePathname() const = 0;

    //! Get the borders of this sprite
    virtual Borders GetBorders() const = 0;

    //! Set the borders of this sprite
    virtual void SetBorders(Borders borders) = 0;

    //! Set the borders of a given cell within the sprite-sheet.
    virtual void SetCellBorders(int cellIndex, Borders borders) = 0;

    //! Serialize this object for save/load
    virtual void Serialize(TSerialize ser) = 0;

    //! Save this sprite data to disk
    virtual bool SaveToXml(const AZStd::string& pathname) = 0;

    //! Test if this sprite has any borders
    virtual bool AreBordersZeroWidth() const = 0;

    //! Tests if the sprite-sheet cell has borders
    virtual bool AreCellBordersZeroWidth(int cellIndex) const = 0;

    //! Get the dimensions of the sprite
    virtual AZ::Vector2 GetSize() = 0;

    //! Gets the dimensions of a specific cell texture within a sprite-sheet
    virtual AZ::Vector2 GetCellSize(int cellIndex) = 0;

    //! Gets cell info for each of the cells within the sprite-sheet.
    virtual const SpriteSheetCellContainer& GetSpriteSheetCells() const = 0;

    //! Sets the sprite's sprite-sheet cells.
    virtual void SetSpriteSheetCells(const SpriteSheetCellContainer& cells) = 0;

    //! Removes all sprite-sheet cell info for this sprite.
    virtual void ClearSpriteSheetCells() = 0;

    //! Defines a new SpriteSheetCell for this sprite.
    virtual void AddSpriteSheetCell(const SpriteSheetCell& spriteSheetCell) = 0;

    //! Gets the dimensions of a specific cell (in a sprite-sheet) in UV coords (UV range).
    virtual AZ::Vector2 GetCellUvSize(int cellIndex) const = 0;

    //! Gets the UV coords associated for a given cell in a sprite-sheet.
    virtual UiTransformInterface::RectPoints GetCellUvCoords(int cellIndex) const = 0;

    //! Gets the UV coords associated for a given cell in a sprite-sheet in a way that ignores texture atlases
    virtual UiTransformInterface::RectPoints GetSourceCellUvCoords(int cellIndex) const = 0;

    //! Gets the sliced border info for a given cell within a sprite-sheet.
    //!
    //! The returned UV borders are in "cell space" relative to the given indexed spell. For example,
    //! a top-left border of (0.5, 0.5) would be the center of the given cell.
    virtual Borders GetCellUvBorders(int cellIndex) const = 0;

    //! Gets the sliced border UV coordinates in texture space for a given cell within a sprite-sheet.
    //!
    //! The returned UV border coordinates are in texture space. For example, a top-left border of (0.5, 0.5) 
    //! would be the center of the sprite-sheet.
    virtual Borders GetTextureSpaceCellUvBorders(int cellIndex) const = 0;

    //! Gets the string alias associated with the given cell in a sprite-sheet.
    virtual const AZStd::string& GetCellAlias(int cellIndex) const = 0;

    //! Sets the string alias associated with the given cell in a sprite-sheet.
    virtual void SetCellAlias(int cellIndex, const AZStd::string& cellAlias) = 0;

    //! Returns the sprite-sheet cell index that corresponds to the given string alias.
    virtual int GetCellIndexFromAlias(const AZStd::string& cellAlias) const = 0;
    
    //! Returns true if this sprite is configured as a sprite-sheet, false otherwise
    virtual bool IsSpriteSheet() const = 0;
};
