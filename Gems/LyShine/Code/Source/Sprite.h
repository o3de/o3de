/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/ISprite.h>
#include <platform.h>
#include <StlUtils.h>

#include "TextureAtlas/TextureAtlas.h"
#include "TextureAtlas/TextureAtlasBus.h"
#include "TextureAtlas/TextureAtlasNotificationBus.h"

#include <Atom/RPI.Reflect/Image/Image.h>
#include <AtomCore/Instance/Instance.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class CSprite
    : public ISprite
    , public TextureAtlasNamespace::TextureAtlasNotificationBus::Handler
{
public: // member functions

    //! Construct an empty sprite
    CSprite();

    // ISprite

    ~CSprite() override;

    const AZStd::string& GetPathname() const override;
    const AZStd::string& GetTexturePathname() const override;
    Borders GetBorders() const override;
    void SetBorders(Borders borders) override;
    void SetCellBorders(int cellIndex, Borders borders) override;
    void Serialize(TSerialize ser) override;
    bool SaveToXml(const AZStd::string& pathname) override;
    bool AreBordersZeroWidth() const override;
    bool AreCellBordersZeroWidth(int index) const override;
    AZ::Vector2 GetSize() override;
    AZ::Vector2 GetCellSize(int cellIndex) override;
    const SpriteSheetCellContainer& GetSpriteSheetCells() const override;
    void SetSpriteSheetCells(const SpriteSheetCellContainer& cells) override;
    void ClearSpriteSheetCells() override;
    void AddSpriteSheetCell(const SpriteSheetCell& spriteSheetCell) override;
    AZ::Vector2 GetCellUvSize(int cellIndex) const override;
    UiTransformInterface::RectPoints GetCellUvCoords(int cellIndex) const override;
    UiTransformInterface::RectPoints GetSourceCellUvCoords(int cellIndex) const override;
    Borders GetCellUvBorders(int cellIndex) const override;
    Borders GetTextureSpaceCellUvBorders(int cellIndex) const override;
    const AZStd::string& GetCellAlias(int cellIndex) const override;
    void SetCellAlias(int cellIndex, const AZStd::string& cellAlias) override;
    int GetCellIndexFromAlias(const AZStd::string& cellAlias) const override;
    bool IsSpriteSheet() const override;

    // ~ISprite

    // TextureAtlasNotifications

    void OnAtlasLoaded(const TextureAtlasNamespace::TextureAtlas* atlas) override;
    void OnAtlasUnloaded(const TextureAtlasNamespace::TextureAtlas* atlas) override;

    // ~TextureAtlasNotifications

    AZ::Data::Instance<AZ::RPI::Image> GetImage();

public: // static member functions

    static void Initialize();
    static void Shutdown();
    static CSprite* LoadSprite(const AZStd::string& pathname);
    static CSprite* CreateSprite(const AZStd::string& renderTargetName);
    static bool DoesSpriteTextureAssetExist(const AZStd::string& pathname);

    //! Replaces baseSprite with newSprite with proper ref-count handling and null-checks.
    static void ReplaceSprite(ISprite** baseSprite, ISprite* newSprite);

    //! Pathname allows any of the following:
    //! 1. image source/product path (will use pathname to look for an existing .sprite sidecar file)
    //! 2. .sprite source/product path (will use pathname to look for an existing image file with a supported extension)
    //! 3. legacy .dds product path (will use pathname to look for an existing texture file with a supported extension)
    static bool FixUpSourceImagePathFromUserDefinedPath(const AZStd::string& userDefinedPath, AZStd::string& sourceImagePath);

    static AZStd::string GetImageSourcePathFromProductPath(const AZStd::string& productPathname);

private:
    static bool LoadImage(const AZStd::string& nameTex, AZ::Data::Instance<AZ::RPI::Image>& image);
    static void ReleaseImage(AZ::Data::Instance<AZ::RPI::Image>& image);

protected: // member functions

    bool CellIndexWithinRange(int cellIndex) const;

private: // types
    using CSpriteHashMap = AZStd::unordered_map<AZStd::string, CSprite*, stl::hash_string_caseless<AZStd::string>, stl::equality_string_caseless<AZStd::string> >;

private: // member functions
    bool LoadFromXmlFile();

    void NotifyChanged();

    AZ_DISABLE_COPY_MOVE(CSprite);

private: // data

    SpriteSheetCellContainer m_spriteSheetCells;  //!< Stores information for each cell defined within the sprite-sheet.

    AZStd::string m_pathname;
    AZStd::string m_texturePathname;
    Borders m_borders;
    AZ::Data::Instance<AZ::RPI::Image> m_image;
    int m_numSpriteSheetCellTags;                       //!< Number of Cell child-tags in sprite XML; unfortunately needed to help with serialization.

    //! Texture atlas data
    const TextureAtlasNamespace::TextureAtlas* m_atlas;
    TextureAtlasNamespace::AtlasCoordinates m_atlasCoordinates;

private: // static data

    //! Used to keep track of all loaded sprites. Sprites are refcounted.
    static CSpriteHashMap* s_loadedSprites;

    static AZStd::string s_emptyString;
};
