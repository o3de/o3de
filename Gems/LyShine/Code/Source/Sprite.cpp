/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Sprite.h"
#include <CryPath.h>
#include <ISerialize.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <LyShine/Bus/Sprite/UiSpriteBus.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace
{
    const char* const spriteExtension = "sprite";
    const char* const streamingImageExtension = "streamingimage";

    // Increment this when the Sprite Serialize(TSerialize) function
    // changes to be incompatible with previous data
    uint32 spriteFileVersionNumber = 2;
    const char* spriteVersionNumberTag = "versionNumber";

    const char* allowedSpriteTextureExtensions[] = {
        "tif", "jpg", "jpeg", "tga", "bmp", "png", "gif", "dds"
    };
    const int numAllowedSpriteTextureExtensions = AZ_ARRAY_SIZE(allowedSpriteTextureExtensions);

    bool IsValidImageExtension(const AZStd::string& extension)
    {
        for (int i = 0; i < numAllowedSpriteTextureExtensions; ++i)
        {
            if (extension.compare(allowedSpriteTextureExtensions[i]) == 0)
            {
                return true;
            }
        }

        return false;
    }

    bool IsImageProductPath(const AZStd::string& pathname)
    {
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(pathname.c_str(), extension, false);
        return (extension.compare(streamingImageExtension) == 0);
    }

    // Check if a file exists. This does not go through the AssetCatalog so that it can identify files that exist but aren't processed yet,
    // and so that it will work before the AssetCatalog has loaded
    bool CheckIfFileExists(const AZStd::string& sourceRelativePath, const AZStd::string& cacheRelativePath)
    {
        // If the file exists, it has already been processed and does not need to be modified
        bool fileExists = AZ::IO::FileIOBase::GetInstance()->Exists(cacheRelativePath.c_str());

        if (!fileExists)
        {
            // If the texture doesn't exist check if it's queued or being compiled.
            AzFramework::AssetSystem::AssetStatus status;
            AzFramework::AssetSystemRequestBus::BroadcastResult(status, &AzFramework::AssetSystemRequestBus::Events::GetAssetStatus, sourceRelativePath);

            switch (status)
            {
            case AzFramework::AssetSystem::AssetStatus_Queued:
            case AzFramework::AssetSystem::AssetStatus_Compiling:
            case AzFramework::AssetSystem::AssetStatus_Compiled:
            case AzFramework::AssetSystem::AssetStatus_Failed:
            {
                // The file is queued, in progress, or finished processing after the initial FileIO check
                fileExists = true;
                break;
            }
            case AzFramework::AssetSystem::AssetStatus_Unknown:
            case AzFramework::AssetSystem::AssetStatus_Missing:
            default:
            {
                // The file does not exist
                fileExists = false;
                break;
            }
            }
        }

        return fileExists;
    }

    bool GetSourceAssetPaths(const AZStd::string& pathname, AZStd::string& spritePath, AZStd::string& texturePath)
    {
        // Remove product extension from the texture path if it exists
        AZStd::string sourcePathname(pathname);
        if (IsImageProductPath(pathname))
        {
            sourcePathname = CSprite::GetImageSourcePathFromProductPath(pathname);
        }

        // the input string could be in any form. So make it normalized (forward slashes and lower case)
        // NOTE: it should not be a full path at this point. If called from the UI editor it will
        // have been transformed to a game path. If being called with a hard coded path it should be a
        // game path already - it is not good for code to be using full paths.
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, sourcePathname);

        // check the extension and work out the pathname of the sprite file and the texture file
        // currently it works if the input path is either a sprite file or a texture file
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(sourcePathname.c_str(), extension, false);

        if (extension.compare(spriteExtension) == 0)
        {
            // The .sprite file has been specified
            spritePath = sourcePathname;

            // look for a texture file with the same name
            if (!CSprite::FixUpSourceImagePathFromUserDefinedPath(spritePath, texturePath))
            {
                gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
                    spritePath.c_str(), "No texture file found for sprite: %s, no sprite will be used", spritePath.c_str());
                return false;
            }
        }
        else if (IsValidImageExtension(extension))
        {
            texturePath = sourcePathname;
            spritePath = sourcePathname;
            AzFramework::StringFunc::Path::ReplaceExtension(spritePath, spriteExtension);
        }
        else
        {
            gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
                pathname.c_str(), "Invalid file extension for sprite: %s, no sprite will be used", pathname.c_str());
            return false;
        }

        return true;
    }

    //! \brief Reads a Vec2 tuple (as a string) into an AZ::Vector2
    //!
    //! Example XML string data: "1.0 2.0"
    void SerializeAzVector2(TSerialize ser, const char* attributeName, AZ::Vector2& azVec2)
    {
        if (ser.IsReading())
        {
            AZStd::string stringVal;
            ser.Value(attributeName, stringVal);
            AZ::StringFunc::Replace(stringVal, ',', ' ');
            char* pEnd = nullptr;
            float uVal = strtof(stringVal.c_str(), &pEnd);
            float vVal = strtof(pEnd, nullptr);
            azVec2.Set(uVal, vVal);
        }
        else
        {
            Vec2 legacyVec2(azVec2.GetX(), azVec2.GetY());
            ser.Value(attributeName, legacyVec2);
        }
    }

    //! \return The number of child <Cell> tags off the <SpriteSheet> parent tag.
    int GetNumSpriteSheetCellTags(const XmlNodeRef& root)
    {
        int numCellTags = 0;
        XmlNodeRef spriteSheetTag = root->findChild("SpriteSheet");
        if (spriteSheetTag)
        {
            numCellTags = spriteSheetTag->getChildCount();
        }
        return numCellTags;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// STATIC MEMBER DATA
////////////////////////////////////////////////////////////////////////////////////////////////////

CSprite::CSpriteHashMap* CSprite::s_loadedSprites;
AZStd::string CSprite::s_emptyString;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite::CSprite()
    : m_numSpriteSheetCellTags(0)
    , m_atlas(nullptr)
{
    AddRef();
    TextureAtlasNamespace::TextureAtlasNotificationBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite::~CSprite()
{
    s_loadedSprites->erase(m_pathname);
    TextureAtlasNamespace::TextureAtlasNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZStd::string& CSprite::GetPathname() const
{
    return m_pathname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZStd::string& CSprite::GetTexturePathname() const
{
    return m_texturePathname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite::Borders CSprite::GetBorders() const
{
    return m_borders;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::SetBorders(Borders borders)
{
    m_borders = borders;
    NotifyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::SetCellBorders(int cellIndex, Borders borders)
{
    if (CellIndexWithinRange(cellIndex))
    {
        m_spriteSheetCells[cellIndex].borders = borders;
        NotifyChanged();
    }
    else
    {
        SetBorders(borders);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Data::Instance<AZ::RPI::Image> CSprite::GetImage()
{
    // Prioritize usage of an atlas
#ifdef LYSHINE_ATOM_TODO // texture atlas conversion to use Atom
    if (m_atlas)
    {
        return m_atlas->GetTexture();
    }
#endif

    return m_image;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::Serialize(TSerialize ser)
{
    // When reading, get sprite-sheet info from XML tag data, otherwise get
    // it from this sprite object directly.
    const bool hasSpriteSheetCells = ser.IsReading() ? m_numSpriteSheetCellTags > 0 : !GetSpriteSheetCells().empty();

    if (!hasSpriteSheetCells && ser.BeginOptionalGroup("Sprite", true))
    {
        ser.Value("m_left", m_borders.m_left);
        ser.Value("m_right", m_borders.m_right);
        ser.Value("m_top", m_borders.m_top);
        ser.Value("m_bottom", m_borders.m_bottom);

        ser.EndGroup();
    }

    if (hasSpriteSheetCells && ser.BeginOptionalGroup("SpriteSheet", true))
    {
        const int numSpriteSheetCells = static_cast<int>(ser.IsReading() ? m_numSpriteSheetCellTags : GetSpriteSheetCells().size());
        for (int i = 0; i < numSpriteSheetCells; ++i)
        {
            ser.BeginOptionalGroup("Cell", true);

            if (ser.IsReading())
            {
                m_spriteSheetCells.push_back(SpriteSheetCell());
            }

            AZStd::string aliasTemp;
            if (ser.IsReading())
            {
                ser.Value("alias", aliasTemp);
                m_spriteSheetCells[i].alias = aliasTemp.c_str();
            }
            else
            {
                aliasTemp = m_spriteSheetCells[i].alias.c_str();
                ser.Value("alias", aliasTemp);
            }

            SerializeAzVector2(ser, "topLeft", m_spriteSheetCells[i].uvCellCoords.TopLeft());
            SerializeAzVector2(ser, "topRight", m_spriteSheetCells[i].uvCellCoords.TopRight());
            SerializeAzVector2(ser, "bottomRight", m_spriteSheetCells[i].uvCellCoords.BottomRight());
            SerializeAzVector2(ser, "bottomLeft", m_spriteSheetCells[i].uvCellCoords.BottomLeft());

            if (ser.BeginOptionalGroup("Sprite", true))
            {
                ser.Value("m_left", m_spriteSheetCells[i].borders.m_left);
                ser.Value("m_right", m_spriteSheetCells[i].borders.m_right);
                ser.Value("m_top", m_spriteSheetCells[i].borders.m_top);
                ser.Value("m_bottom", m_spriteSheetCells[i].borders.m_bottom);

                ser.EndGroup();
            }

            ser.EndGroup();
        }

        ser.EndGroup();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::SaveToXml(const AZStd::string& pathname)
{
    // NOTE: The input pathname has to be a path that can used to save - so not an Asset ID
    // because of this we do not store the pathname

    XmlNodeRef root =  GetISystem()->CreateXmlNode("Sprite");
    std::unique_ptr<IXmlSerializer> pSerializer(GetISystem()->GetXmlUtils()->CreateXmlSerializer());
    ISerialize* pWriter = pSerializer->GetWriter(root);
    TSerialize ser = TSerialize(pWriter);

    ser.Value(spriteVersionNumberTag, spriteFileVersionNumber);
    Serialize(ser);

    return root->saveToFile(pathname.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::AreBordersZeroWidth() const
{
    return (m_borders.m_left == 0 && m_borders.m_right == 1 && m_borders.m_top == 0 && m_borders.m_bottom == 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::AreCellBordersZeroWidth(int cellIndex) const
{
    if (CellIndexWithinRange(cellIndex))
    {
        return m_spriteSheetCells[cellIndex].borders.m_left == 0
               && m_spriteSheetCells[cellIndex].borders.m_right == 1
               && m_spriteSheetCells[cellIndex].borders.m_top == 0
               && m_spriteSheetCells[cellIndex].borders.m_bottom == 1;
    }
    else
    {
        return AreBordersZeroWidth();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CSprite::GetSize()
{
    AZ::Data::Instance<AZ::RPI::Image> image = GetImage();
    if (image)
    {
        if (m_atlas)
        {
            return AZ::Vector2(static_cast<float>(m_atlasCoordinates.GetWidth()), static_cast<float>(m_atlasCoordinates.GetHeight()));
        }

        AZ::RHI::Size size = image->GetRHIImage()->GetDescriptor().m_size;
        return AZ::Vector2(static_cast<float>(size.m_width), static_cast<float>(size.m_height));
    }
    else
    {
        return AZ::Vector2(0.0f, 0.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CSprite::GetCellSize(int cellIndex)
{
    AZ::Vector2 textureSize(GetSize());

    if (CellIndexWithinRange(cellIndex))
    {
        // Assume top width is same as bottom width
        const float normalizedCellWidth =
            m_spriteSheetCells[cellIndex].uvCellCoords.TopRight().GetX() -
            m_spriteSheetCells[cellIndex].uvCellCoords.TopLeft().GetX();

        // Similar, assume height of cell is same for left and right sides
        const float normalizedCellHeight =
            m_spriteSheetCells[cellIndex].uvCellCoords.BottomLeft().GetY() -
            m_spriteSheetCells[cellIndex].uvCellCoords.TopLeft().GetY();

        textureSize.SetX(textureSize.GetX() * normalizedCellWidth);
        textureSize.SetY(textureSize.GetY() * normalizedCellHeight);
    }

    return textureSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const ISprite::SpriteSheetCellContainer& CSprite::GetSpriteSheetCells() const
{
    return m_spriteSheetCells;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::SetSpriteSheetCells(const SpriteSheetCellContainer& cells)
{
    m_spriteSheetCells = cells;
    NotifyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::ClearSpriteSheetCells()
{
    m_spriteSheetCells.clear();
    NotifyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::AddSpriteSheetCell(const SpriteSheetCell& spriteSheetCell)
{
    m_spriteSheetCells.push_back(spriteSheetCell);
    NotifyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CSprite::GetCellUvSize(int cellIndex) const
{
    AZ::Vector2 result(1.0f, 1.0f);

    if (CellIndexWithinRange(cellIndex))
    {
        result.SetX(m_spriteSheetCells[cellIndex].uvCellCoords.TopRight().GetX() - m_spriteSheetCells[cellIndex].uvCellCoords.TopLeft().GetX());
        result.SetY(m_spriteSheetCells[cellIndex].uvCellCoords.BottomLeft().GetY() - m_spriteSheetCells[cellIndex].uvCellCoords.TopLeft().GetY());
    }
    if (m_atlas)
    {
        result.SetX(result.GetX() * m_atlasCoordinates.GetWidth() / m_atlas->GetWidth());
        result.SetY(result.GetY() * m_atlasCoordinates.GetHeight() / m_atlas->GetHeight());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransformInterface::RectPoints CSprite::GetCellUvCoords(int cellIndex) const
{
    if (CellIndexWithinRange(cellIndex))
    {
        if (m_atlas)
        {
            return UiTransformInterface::RectPoints(
                static_cast<float>(m_atlasCoordinates.GetLeft() + (m_spriteSheetCells[cellIndex].uvCellCoords.TopLeft().GetX() * m_atlasCoordinates.GetWidth()))
                / m_atlas->GetWidth(),
                static_cast<float>(m_atlasCoordinates.GetLeft() + (m_spriteSheetCells[cellIndex].uvCellCoords.TopRight().GetX() * m_atlasCoordinates.GetWidth()))
                / m_atlas->GetWidth(),
                static_cast<float>(m_atlasCoordinates.GetTop() + (m_spriteSheetCells[cellIndex].uvCellCoords.TopLeft().GetY() * m_atlasCoordinates.GetHeight()))
                / m_atlas->GetHeight(),
                static_cast<float>(m_atlasCoordinates.GetTop() + (m_spriteSheetCells[cellIndex].uvCellCoords.BottomLeft().GetY() * m_atlasCoordinates.GetHeight()))
                / m_atlas->GetHeight());
        }
        return m_spriteSheetCells[cellIndex].uvCellCoords;
    }
    else if (m_atlas)
    {
        return UiTransformInterface::RectPoints(
            static_cast<float>(m_atlasCoordinates.GetLeft()) / m_atlas->GetWidth(),
            static_cast<float>(m_atlasCoordinates.GetRight()) / m_atlas->GetWidth(),
            static_cast<float>(m_atlasCoordinates.GetTop()) / m_atlas->GetHeight(),
            static_cast<float>(m_atlasCoordinates.GetBottom()) / m_atlas->GetHeight());
    }

    static UiTransformInterface::RectPoints rectPoints(0.0f, 1.0f, 0.0f, 1.0f);
    return rectPoints;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTransformInterface::RectPoints CSprite::GetSourceCellUvCoords(int cellIndex) const
{
    if (CellIndexWithinRange(cellIndex))
    {
        return m_spriteSheetCells[cellIndex].uvCellCoords;
    }

    static UiTransformInterface::RectPoints rectPoints(0.0f, 1.0f, 0.0f, 1.0f);
    return rectPoints;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite::Borders CSprite::GetCellUvBorders(int cellIndex) const
{
    if (CellIndexWithinRange(cellIndex))
    {
        return m_spriteSheetCells[cellIndex].borders;
    }

    return m_borders;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite::Borders CSprite::GetTextureSpaceCellUvBorders(int cellIndex) const
{
    Borders textureSpaceBorders(m_borders);

    if (CellIndexWithinRange(cellIndex))
    {
        const float cellWidth = GetCellUvSize(cellIndex).GetX();
        const float cellNormalizedLeftBorder = GetCellUvBorders(cellIndex).m_left * cellWidth;
        textureSpaceBorders.m_left = cellNormalizedLeftBorder;

        const float cellNormalizedRightBorder = GetCellUvBorders(cellIndex).m_right * cellWidth;
        textureSpaceBorders.m_right = cellNormalizedRightBorder;

        const float cellHeight = GetCellUvSize(cellIndex).GetY();
        const float cellNormalizedTopBorder = GetCellUvBorders(cellIndex).m_top * cellHeight;
        textureSpaceBorders.m_top = cellNormalizedTopBorder;

        const float cellNormalizedBottomBorder = GetCellUvBorders(cellIndex).m_bottom * cellHeight;
        textureSpaceBorders.m_bottom = cellNormalizedBottomBorder;
    }
    return textureSpaceBorders;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZStd::string& CSprite::GetCellAlias(int cellIndex) const
{
    if (CellIndexWithinRange(cellIndex))
    {
        return m_spriteSheetCells[cellIndex].alias;
    }

    return s_emptyString;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::SetCellAlias(int cellIndex, const AZStd::string& cellAlias)
{
    if (CellIndexWithinRange(cellIndex))
    {
        m_spriteSheetCells[cellIndex].alias = cellAlias;
        NotifyChanged();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::IsSpriteSheet() const
{
    return m_spriteSheetCells.size() > 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::OnAtlasLoaded(const TextureAtlasNamespace::TextureAtlas* atlas)
{
    if (!m_atlas)
    {
        m_atlasCoordinates = atlas->GetAtlasCoordinates(m_pathname.c_str());
        if (m_atlasCoordinates.GetWidth() > 0)
        {
            m_atlas = atlas;
            // Release the non-atlas version of the texture
            ReleaseImage(m_image);
            NotifyChanged();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::OnAtlasUnloaded(const TextureAtlasNamespace::TextureAtlas* atlas)
{
    if (atlas == m_atlas)
    {
        m_atlas = nullptr;
        TextureAtlasNamespace::TextureAtlasRequestBus::BroadcastResult(m_atlas, &TextureAtlasNamespace::TextureAtlasRequests::FindAtlasContainingImage, m_pathname.c_str());
        if (m_atlas)
        {
            m_atlasCoordinates = m_atlas->GetAtlasCoordinates(m_pathname.c_str());
        }
        else
        {
            // No replacement atlas found
            // load the texture file
            LoadImage(m_texturePathname.c_str(), m_image);
        }
    }
    NotifyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int CSprite::GetCellIndexFromAlias(const AZStd::string& cellAlias) const
{
    int result = 0;
    int indexIter = 0;
    for (auto spriteCell : m_spriteSheetCells)
    {
        if (cellAlias == spriteCell.alias)
        {
            result = indexIter;
            break;
        }
        ++indexIter;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::Initialize()
{
    s_loadedSprites = new CSpriteHashMap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::Shutdown()
{
    delete s_loadedSprites;
    s_loadedSprites = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite* CSprite::LoadSprite(const AZStd::string& pathname)
{
    AZStd::string spritePath;
    AZStd::string texturePath;
    bool validAssetPaths = GetSourceAssetPaths(pathname.c_str(), spritePath, texturePath);
    
    if (!validAssetPaths)
    {
        return nullptr;
    }

    // test if the sprite is already loaded, if so return loaded sprite
    auto result = s_loadedSprites->find(spritePath.c_str());
    CSprite* loadedSprite = (result == s_loadedSprites->end()) ? nullptr : result->second;

    if (loadedSprite)
    {
        loadedSprite->AddRef();
        return loadedSprite;
    }

    // Try to use a texture atlas instead
    TextureAtlasNamespace::TextureAtlas* atlas = nullptr;
    TextureAtlasNamespace::AtlasCoordinates atlasCoordinates;
    TextureAtlasNamespace::TextureAtlasRequestBus::BroadcastResult(atlas, &TextureAtlasNamespace::TextureAtlasRequests::FindAtlasContainingImage, texturePath.c_str());
    AZ::Data::Instance<AZ::RPI::Image> image;
    if (atlas)
    {
        atlasCoordinates = atlas->GetAtlasCoordinates(texturePath.c_str());
    }
    else
    {
        // load the texture file
        if (!LoadImage(texturePath, image))
        {
            return nullptr;
        }
    }

    // create Sprite object
    CSprite* sprite = new CSprite;

    sprite->m_image = image;
    sprite->m_pathname = spritePath.c_str();
    sprite->m_texturePathname = texturePath.c_str();
    sprite->m_atlas = atlas;
    sprite->m_atlasCoordinates = atlasCoordinates;

    // try to load the sprite side-car file if it exists, it is optional and if it does not
    // exist we just stay with default values
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (fileIO && fileIO->Exists(sprite->m_pathname.c_str()))
    {
        sprite->LoadFromXmlFile();
    }
    // add sprite to list of loaded sprites
    (*s_loadedSprites)[sprite->m_pathname] = sprite;

    return sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite* CSprite::CreateSprite(const AZStd::string& renderTargetName)
{
    // test if the sprite is already loaded, if so return loaded sprite
    auto result = s_loadedSprites->find(renderTargetName);
    CSprite* loadedSprite = (result == s_loadedSprites->end()) ? nullptr : result->second;

    if (loadedSprite)
    {
        loadedSprite->AddRef();
        return loadedSprite;
    }

    // create Sprite object
    CSprite* sprite = new CSprite;

#ifdef LYSHINE_ATOM_TODO // render target converstion to use ATom
    // the render target texture may not exist yet in which case we will need to load it later
    sprite->m_texture = gEnv->pRenderer->EF_GetTextureByName(renderTargetName.c_str());
    if (sprite->m_texture)
    {
        // increase the reference count on this render target texture so it doesn't get deleted
        // while we are using it
        sprite->m_texture->AddRef();
    }
#endif
    sprite->m_pathname = renderTargetName;
    sprite->m_texturePathname.clear();

    // add sprite to list of loaded sprites
    (*s_loadedSprites)[sprite->m_pathname] = sprite;

    return sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::DoesSpriteTextureAssetExist(const AZStd::string& pathname)
{
    AZStd::string spritePath;
    AZStd::string texturePath;
    bool validAssetPaths = GetSourceAssetPaths(pathname.c_str(), spritePath, texturePath);

    if (!validAssetPaths)
    {
        return false;
    }

    // Check if the sprite is already loaded
    auto result = s_loadedSprites->find(spritePath.c_str());
    CSprite* loadedSprite = (result == s_loadedSprites->end()) ? nullptr : result->second;
    if (loadedSprite)
    {
        return true;
    }

    // Try to use a texture atlas instead
    TextureAtlasNamespace::TextureAtlas* atlas = nullptr;
    TextureAtlasNamespace::AtlasCoordinates atlasCoordinates;
    TextureAtlasNamespace::TextureAtlasRequestBus::BroadcastResult(atlas, &TextureAtlasNamespace::TextureAtlasRequests::FindAtlasContainingImage, texturePath.c_str());
    if (atlas)
    {
        return true;
    }

    // Check if the texture asset exists
    bool textureExists = CheckIfFileExists(spritePath, texturePath);
    return textureExists;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::ReplaceSprite(ISprite** baseSprite, ISprite* newSprite)
{
    if (baseSprite)
    {
        if (newSprite)
        {
            newSprite->AddRef();
        }

        SAFE_RELEASE(*baseSprite);

        *baseSprite = newSprite;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::FixUpSourceImagePathFromUserDefinedPath(const AZStd::string& userDefinedPath, AZStd::string& sourceImagePath)
{
    static const char* textureExtensions[] = { "png", "tif", "tiff", "tga", "jpg", "jpeg", "bmp", "gif" };

    AZStd::string sourceRelativePath(userDefinedPath);
    AZStd::string cacheRelativePath = AZStd::string::format("%s.%s", sourceRelativePath.c_str(), streamingImageExtension);
    bool textureExists = CheckIfFileExists(sourceRelativePath, cacheRelativePath);

    if (textureExists)
    {
        sourceImagePath = userDefinedPath;
        return true;
    }

    AZStd::string curSourceImagePath(userDefinedPath);
    for (const char* extensionReplacement : textureExtensions)
    {
        AzFramework::StringFunc::Path::ReplaceExtension(curSourceImagePath, extensionReplacement);
        cacheRelativePath = AZStd::string::format("%s.%s", curSourceImagePath.c_str(), streamingImageExtension);
        textureExists = CheckIfFileExists(curSourceImagePath, cacheRelativePath);

        if (textureExists)
        {
            sourceImagePath = curSourceImagePath;
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string CSprite::GetImageSourcePathFromProductPath(const AZStd::string& productPathname)
{
    AZStd::string sourcePathname(productPathname);
    if (IsImageProductPath(sourcePathname))
    {
        AzFramework::StringFunc::Path::StripExtension(sourcePathname);
    }
    return sourcePathname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::LoadImage(const AZStd::string& nameTex, AZ::Data::Instance<AZ::RPI::Image>& image)
{
    AZStd::string sourceRelativePath(nameTex);
    AZStd::string cacheRelativePath = AZStd::string::format("%s.%s", sourceRelativePath.c_str(), streamingImageExtension);
    bool textureExists = CheckIfFileExists(sourceRelativePath, cacheRelativePath);

    if (!textureExists)
    {
        // LyShine allows passing in a .dds extension even when the actual source file
        // is differnt like a .tif. For the product file, we need the correct source extension
        // prepended to the .streamingimage extension. So if the file doesn't exist and the
        // extension passed in is .dds then try replacing it with .tif
        // LYSHINE_ATOM_TODO - to remove this conversion we will have to update the existing
        // .dds references in Lua scripts, prefabs etc.
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(sourceRelativePath.c_str(), extension, false);
        if (extension == "dds")
        {
            textureExists = FixUpSourceImagePathFromUserDefinedPath(nameTex, sourceRelativePath);
        }
    }

    if (!textureExists)
    {
        AZ_Error("CSprite", false, "Attempted to load '%s', but it does not exist.", nameTex.c_str());
        return false;
    }

    // The file may not be in the AssetCatalog at this point if it is still processing or doesn't exist on disk.
    // Use GenerateAssetIdTEMP instead of GetAssetIdByPath so that it will return a valid AssetId anyways
    AZ::Data::AssetId streamingImageAssetId;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        streamingImageAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP,
        sourceRelativePath.c_str());
    streamingImageAssetId.m_subId = AZ::RPI::StreamingImageAsset::GetImageAssetSubId();

    auto streamingImageAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::RPI::StreamingImageAsset>(streamingImageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
    image = AZ::RPI::StreamingImage::FindOrCreate(streamingImageAsset);
    if (!image)
    {
        AZ_Error("CSprite", false, "Failed to find or create an image instance from image asset '%s', ID %s",
            streamingImageAsset.GetHint().c_str(), streamingImageAsset.GetId().ToString<AZStd::string>().c_str());
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::ReleaseImage(AZ::Data::Instance<AZ::RPI::Image>& image)
{
    image.reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::CellIndexWithinRange(int cellIndex) const
{
    return cellIndex >= 0 && cellIndex < m_spriteSheetCells.size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::LoadFromXmlFile()
{
    XmlNodeRef root = GetISystem()->LoadXmlFromFile(m_pathname.c_str());

    if (!root)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
            m_pathname.c_str(),
            "No sprite file found for sprite: %s, default sprite values will be used", m_pathname.c_str());
        return false;
    }

    std::unique_ptr<IXmlSerializer> pSerializer(GetISystem()->GetXmlUtils()->CreateXmlSerializer());
    ISerialize* pReader = pSerializer->GetReader(root);

    TSerialize ser = TSerialize(pReader);

    uint32 versionNumber = spriteFileVersionNumber;
    ser.Value(spriteVersionNumberTag, versionNumber);
    const bool validVersionNumber = versionNumber >= 1 && versionNumber <= spriteFileVersionNumber;
    if (!validVersionNumber)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
            m_pathname.c_str(),
            "Unsupported version number found for sprite file: %s, default sprite values will be used",
            m_pathname.c_str());
        return false;
    }

    // The serializer doesn't seem to have good support for parsing a variable
    // number of tags of the same type, so we count up the children ourselves
    // before starting serialization.
    m_numSpriteSheetCellTags = GetNumSpriteSheetCellTags(root);
    Serialize(ser);

    NotifyChanged();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::NotifyChanged()
{
    EBUS_EVENT_ID(this, UiSpriteSettingsChangeNotificationBus, OnSpriteSettingsChanged);
}
