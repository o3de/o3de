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

#include <LyShine/ISprite.h>
#include <platform.h>
#include <StlUtils.h>

#include "TextureAtlas/TextureAtlas.h"
#include "TextureAtlas/TextureAtlasBus.h"
#include "TextureAtlas/TextureAtlasNotificationBus.h"

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

    const string& GetPathname() const override;
    const string& GetTexturePathname() const override;
    Borders GetBorders() const override;
    void SetBorders(Borders borders) override;
    void SetCellBorders(int cellIndex, Borders borders) override;
    ITexture* GetTexture() override;
    void Serialize(TSerialize ser) override;
    bool SaveToXml(const string& pathname) override;
    bool AreBordersZeroWidth() const override;
    bool AreCellBordersZeroWidth(int index) const override;
    AZ::Vector2 GetSize() override;
    AZ::Vector2 GetCellSize(int cellIndex) override;
    const SpriteSheetCellContainer& GetSpriteSheetCells() const override;
    virtual void SetSpriteSheetCells(const SpriteSheetCellContainer& cells);
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

public: // static member functions

    static void Initialize();
    static void Shutdown();
    static CSprite* LoadSprite(const string& pathname);
    static CSprite* CreateSprite(const string& renderTargetName);
    static bool DoesSpriteTextureAssetExist(const AZStd::string& pathname);

    //! Replaces baseSprite with newSprite with proper ref-count handling and null-checks.
    static void ReplaceSprite(ISprite** baseSprite, ISprite* newSprite);

private:
    static bool LoadTexture(const string& texturePathname, const string& pathname, ITexture*& texture);
    static void ReleaseTexture(ITexture*& texture);

protected: // member functions

    bool CellIndexWithinRange(int cellIndex) const;

private: // types
    typedef AZStd::unordered_map<string, CSprite*, stl::hash_string_caseless<string>, stl::equality_string_caseless<string> > CSpriteHashMap;

private: // member functions
    bool LoadFromXmlFile();

    void NotifyChanged();

    AZ_DISABLE_COPY_MOVE(CSprite);

private: // data

    SpriteSheetCellContainer m_spriteSheetCells;  //!< Stores information for each cell defined within the sprite-sheet.

    string m_pathname;
    string m_texturePathname;
    Borders m_borders;
    ITexture* m_texture;
    int m_numSpriteSheetCellTags;                       //!< Number of Cell child-tags in sprite XML; unfortunately needed to help with serialization.

    //! Texture atlas data
    const TextureAtlasNamespace::TextureAtlas* m_atlas;
    TextureAtlasNamespace::AtlasCoordinates m_atlasCoordinates;

private: // static data

    //! Used to keep track of all loaded sprites. Sprites are refcounted.
    static CSpriteHashMap* s_loadedSprites;

    static AZStd::string s_emptyString;
};
