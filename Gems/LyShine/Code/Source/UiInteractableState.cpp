/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiInteractableState.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <LyShine/ISprite.h>
#include <LyShine/UiSerializeHelpers.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiIndexableImageBus.h>
#include <LyShine/ILyShine.h>

#include "EditorPropertyTypes.h"
#include "Sprite.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// UiInteractableStateAction class
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateAction::SetInteractableEntity(AZ::EntityId interactableEntityId)
{
    m_interactableEntity = interactableEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateAction::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<UiInteractableStateAction>();
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateAction::Init(AZ::EntityId interactableEntityId)
{
    m_interactableEntity = interactableEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateAction::EntityComboBoxVec UiInteractableStateAction::PopulateTargetEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.emplace_back(m_interactableEntity, "<This element>");

    // Get a list of all child elements
    LyShine::EntityArray matchingElements;
    UiElementBus::Event(
        m_interactableEntity,
        &UiElementBus::Events::FindDescendantElements,
        []([[maybe_unused]] const AZ::Entity* entity)
        {
            return true;
        },
        matchingElements);

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : matchingElements)
    {
        result.push_back(AZStd::make_pair(childEntity->GetId(), childEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// UiInteractableStateColor class
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateColor::UiInteractableStateColor()
    : m_color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateColor::UiInteractableStateColor(AZ::EntityId target, AZ::Color color)
    : m_targetEntity(target)
    , m_color(color)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateColor::Init(AZ::EntityId interactableEntityId)
{
    UiInteractableStateAction::Init(interactableEntityId);

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = interactableEntityId;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateColor::ApplyState()
{
    UiVisualBus::Event(m_targetEntity, &UiVisualBus::Events::SetOverrideColor, m_color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateColor::SetInteractableEntity(AZ::EntityId interactableEntityId)
{
    m_interactableEntity = interactableEntityId;

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = m_interactableEntity;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateAction::EntityComboBoxVec UiInteractableStateColor::PopulateTargetEntityList()
{
    return UiInteractableStateAction::PopulateTargetEntityList();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateColor::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiInteractableStateColor, UiInteractableStateAction>()
            ->Version(3, &VersionConverter)
            ->Field("TargetEntity", &UiInteractableStateColor::m_targetEntity)
            ->Field("Color", &UiInteractableStateColor::m_color);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiInteractableStateColor>("Color", "Overrides the color tint on the target element.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("ComboBox", &UiInteractableStateColor::m_targetEntity, "Target", "The target element.")
                ->Attribute("EnumValues", &UiInteractableStateColor::PopulateTargetEntityList);
            editInfo->DataElement("Color", &UiInteractableStateColor::m_color, "Color", "The color tint.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableStateColor::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to current:
    // - Need to convert AZ::Vector3 to AZ::Color
    if (classElement.GetVersion() <= 1)
    {
        if (!LyShine::ConvertSubElementFromVector3ToAzColor(context, classElement, "Color"))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// UiInteractableStateAlpha class
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateAlpha::UiInteractableStateAlpha()
    : m_alpha(1.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateAlpha::UiInteractableStateAlpha(AZ::EntityId target, float alpha)
    : m_targetEntity(target)
    , m_alpha(alpha)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateAlpha::Init(AZ::EntityId interactableEntityId)
{
    UiInteractableStateAction::Init(interactableEntityId);

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = interactableEntityId;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateAlpha::ApplyState()
{
    UiVisualBus::Event(m_targetEntity, &UiVisualBus::Events::SetOverrideAlpha, m_alpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateAlpha::SetInteractableEntity(AZ::EntityId interactableEntityId)
{
    m_interactableEntity = interactableEntityId;

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = m_interactableEntity;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateAlpha::EntityComboBoxVec UiInteractableStateAlpha::PopulateTargetEntityList()
{
    return UiInteractableStateAction::PopulateTargetEntityList();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateAlpha::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiInteractableStateAlpha, UiInteractableStateAction>()
            ->Version(2)
            ->Field("TargetEntity", &UiInteractableStateAlpha::m_targetEntity)
            ->Field("Alpha", &UiInteractableStateAlpha::m_alpha);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiInteractableStateAlpha>("Alpha", "Overrides the alpha on the target element.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("ComboBox", &UiInteractableStateAlpha::m_targetEntity, "Target", "The target element.")
                ->Attribute("EnumValues", &UiInteractableStateAlpha::PopulateTargetEntityList);
            editInfo->DataElement("Slider", &UiInteractableStateAlpha::m_alpha, "Alpha", "The opacity.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// UiInteractableStateSprite class
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateSprite::UiInteractableStateSprite()
    : m_sprite(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateSprite::UiInteractableStateSprite(AZ::EntityId target, ISprite* sprite)
    : m_targetEntity(target)
    , m_sprite(sprite)
{
    m_sprite->AddRef();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateSprite::UiInteractableStateSprite(AZ::EntityId target, const AZStd::string& spritePath)
    : m_targetEntity(target)
{
    m_spritePathname.SetAssetPath(spritePath.c_str());

    if (!m_spritePathname.GetAssetPath().empty())
    {
        m_sprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePathname.GetAssetPath().c_str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateSprite::~UiInteractableStateSprite()
{
    SAFE_RELEASE(m_sprite);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::Init(AZ::EntityId interactableEntityId)
{
    UiInteractableStateAction::Init(interactableEntityId);

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = interactableEntityId;
    }

    // If this is called from RC.exe for example these pointers will not be set. In that case
    // we only need to be able to load, init and save the component. It will never be
    // activated.
    if (!AZ::Interface<ILyShine>::Get())
    {
        return;
    }

    // for the case of serializing from disk, if we have sprite pathnames but the sprites
    // are not loaded then load them
    if (!m_sprite && !m_spritePathname.GetAssetPath().empty())
    {
        m_sprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePathname.GetAssetPath().c_str());
    }

    if (!m_sprite)
    {
        LoadSpriteFromTargetElement();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::ApplyState()
{
    UiVisualBus::Event(m_targetEntity, &UiVisualBus::Events::SetOverrideSprite, m_sprite, m_spriteSheetCellIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::SetInteractableEntity(AZ::EntityId interactableEntityId)
{
    m_interactableEntity = interactableEntityId;

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = m_interactableEntity;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::SetSprite(ISprite* sprite)
{
    CSprite::ReplaceSprite(&m_sprite, sprite);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiInteractableStateSprite::GetSpritePathname()
{
    return m_spritePathname.GetAssetPath();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::SetSpritePathname(const AZStd::string& spritePath)
{
    m_spritePathname.SetAssetPath(spritePath.c_str());

    OnSpritePathnameChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateSprite::EntityComboBoxVec UiInteractableStateSprite::PopulateTargetEntityList()
{
    return UiInteractableStateAction::PopulateTargetEntityList();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::OnSpritePathnameChange()
{
    ISprite* newSprite = nullptr;

    if (!m_spritePathname.GetAssetPath().empty())
    {
        // Load the new texture.
        newSprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePathname.GetAssetPath().c_str());
    }

    SAFE_RELEASE(m_sprite);

    m_sprite = newSprite;

    // Default to selecting first cell in sprite-sheet
    m_spriteSheetCellIndex = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiInteractableStateSprite, UiInteractableStateAction>()
            ->Version(4)
            ->Field("TargetEntity", &UiInteractableStateSprite::m_targetEntity)
            ->Field("Sprite", &UiInteractableStateSprite::m_spritePathname)
            ->Field("Index", &UiInteractableStateSprite::m_spriteSheetCellIndex);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiInteractableStateSprite>("Sprite", "Overrides the sprite on the target element.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("ComboBox", &UiInteractableStateSprite::m_targetEntity, "Target", "The target element.")
                ->Attribute("EnumValues", &UiInteractableStateSprite::PopulateTargetEntityList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiInteractableStateSprite::OnTargetElementChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
            editInfo->DataElement("Sprite", &UiInteractableStateSprite::m_spritePathname, "Sprite", "The sprite.")
                ->Attribute("ChangeNotify", &UiInteractableStateSprite::OnSpritePathnameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiInteractableStateSprite::m_spriteSheetCellIndex, "Index", "Sprite-sheet index. Defines which cell in a sprite-sheet is displayed.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiInteractableStateSprite::IsSpriteSheet)
                ->Attribute("EnumValues", &UiInteractableStateSprite::PopulateIndexStringList);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiInteractableStateSprite::IsSpriteSheet()
{
    if (!m_sprite)
    {
        return false;
    }

    // We could query the target element's UiImageBus to see if the
    // sprite-type is actually sprite-sheet, but instead we simply check if
    // the provided sprite has more than one sprite-sheet cell configured.
    return m_sprite->GetSpriteSheetCells().size() > 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::OnTargetElementChange()
{
    if (!m_sprite && m_targetEntity.IsValid())
    {
        LoadSpriteFromTargetElement();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateSprite::LoadSpriteFromTargetElement()
{
    AZStd::string spritePathname;
    UiImageBus::EventResult(spritePathname, m_targetEntity, &UiImageBus::Events::GetSpritePathname);
    m_spritePathname.SetAssetPath(spritePathname.c_str());

    OnSpritePathnameChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateSprite::AZu32ComboBoxVec UiInteractableStateSprite::PopulateIndexStringList() const
{
    int indexCount = 0;
    UiIndexableImageBus::EventResult(indexCount, m_targetEntity, &UiIndexableImageBus::Events::GetImageIndexCount);

    if (indexCount > 0)
    {
        return LyShine::GetEnumSpriteIndexList(m_targetEntity, 0, indexCount - 1);
    }

    return LyShine::AZu32ComboBoxVec();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// UiInteractableStateFont class
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateFont::UiInteractableStateFont()
    : m_fontFamily(nullptr)
    , m_fontEffectIndex(0)
{
    InitCommon("default-ui");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateFont::UiInteractableStateFont(AZ::EntityId target, const AZStd::string& pathname, unsigned int fontEffectIndex)
    : m_targetEntity(target)
    , m_fontFamily(nullptr)
    , m_fontEffectIndex(fontEffectIndex)
{
    InitCommon(pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::InitCommon(const AZStd::string& fontPathname)
{
    SetFontPathname(fontPathname);

    FontNotificationBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateFont::~UiInteractableStateFont()
{
    FontNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::Init(AZ::EntityId interactableEntityId)
{
    UiInteractableStateAction::Init(interactableEntityId);

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = interactableEntityId;
    }

    // This will load the font if needed
    SetFontPathname(m_fontFilename.GetAssetPath().c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::ApplyState()
{
    UiVisualBus::Event(m_targetEntity, &UiVisualBus::Events::SetOverrideFont, m_fontFamily);
    UiVisualBus::Event(m_targetEntity, &UiVisualBus::Events::SetOverrideFontEffect, m_fontEffectIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::SetInteractableEntity(AZ::EntityId interactableEntityId)
{
    m_interactableEntity = interactableEntityId;

    if (!m_targetEntity.IsValid())
    {
        m_targetEntity = m_interactableEntity;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::OnFontsReloaded()
{
    // All old font pointers have been deleted and the old font family pointers have been removed from the CryFont list.
    // New fonts and font family objects have been created and added to the CryFont list.
    // However, the old font family objects are still around because we have a shared pointer to them.
    // Clear the font family shared pointers since they should no longer be used (their fonts have been deleted).
    // When the last one is cleared, the font family's custom deleter will be called and the object will be deleted.
    // This is OK because the custom deleter doesn't do anything if the font family is not in the CryFont's list (which it isn't)
    m_fontFamily = nullptr;

    SetFontPathname(m_fontFilename.GetAssetPath());

    // It's possible that the font failed to load. If it did, try to load and use the default font but leave the
    // assigned font path the same
    if (!m_fontFamily)
    {
        AZStd::string assignedFontFilepath = m_fontFilename.GetAssetPath();
        SetFontPathname("");
        m_fontFilename.SetAssetPath(assignedFontFilepath.c_str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::SetFontPathname(const AZStd::string& pathname)
{
    // Just to be safe we make sure is normalized
    AZStd::string fontPath = pathname;
    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::NormalizePath, fontPath);
    m_fontFilename.SetAssetPath(fontPath.c_str());

    // We should minimize what is done in constructors and Init since components may be constructed
    // in RC or other tools. Currrently this method is called from the constructor and Init.
    if (gEnv && gEnv->pCryFont &&
        (!m_fontFamily || gEnv->pCryFont->GetFontFamily(fontPath.c_str()) != m_fontFamily))
    {
        AZStd::string fileName = fontPath;
        if (fileName.empty())
        {
            fileName = "default-ui";
        }

        FontFamilyPtr fontFamily = gEnv->pCryFont->GetFontFamily(fileName.c_str());
        if (!fontFamily)
        {
            fontFamily = gEnv->pCryFont->LoadFontFamily(fileName.c_str());
            if (!fontFamily)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Error loading a font from %s.", fileName.c_str());
            }
        }

        if (fontFamily)
        {
            m_fontFamily = fontFamily;
            // we know that the input path is a root relative and normalized pathname
            m_fontFilename.SetAssetPath(fileName.c_str());

            // the font has changed so check that the font effect is valid
            unsigned int numEffects = m_fontFamily ? m_fontFamily->normal->GetNumEffects() : 0;
            if (m_fontEffectIndex >= numEffects)
            {
                m_fontEffectIndex = 0;
                AZ_Warning("UiInteractableState", false, "Font effect index is out of range for changed font, resetting index to 0");
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateFont::EntityComboBoxVec UiInteractableStateFont::PopulateTargetEntityList()
{
    return UiInteractableStateAction::PopulateTargetEntityList();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStateFont::FontEffectComboBoxVec UiInteractableStateFont::PopulateFontEffectList()
{
    FontEffectComboBoxVec result;
    AZStd::vector<AZ::EntityId> entityIdList;

    // there is always a valid font since we default to "default-ui"
    // so just get the effects from the font and add their names to the result list
    // NOTE: Curently, in order for this to work, when the font is changed we need to do
    // "RefreshEntireTree" to get the combo box list refreshed.
    unsigned int numEffects = m_fontFamily ? m_fontFamily->normal->GetNumEffects() : 0;
    for (unsigned int i = 0; i < numEffects; ++i)
    {
        const char* name = m_fontFamily->normal->GetEffectName(i);
        result.push_back(AZStd::make_pair(i, name));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::OnFontPathnameChange()
{
    AZStd::string fontPath = m_fontFilename.GetAssetPath();
    SetFontPathname(fontPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiInteractableStateFont::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiInteractableStateFont, UiInteractableStateAction>()
            ->Version(2)
            ->Field("TargetEntity", &UiInteractableStateFont::m_targetEntity)
            ->Field("FontFileName", &UiInteractableStateFont::m_fontFilename)
            ->Field("EffectIndex", &UiInteractableStateFont::m_fontEffectIndex);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiInteractableStateFont>("Font", "Overrides the font on the target element.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement("ComboBox", &UiInteractableStateFont::m_targetEntity, "Target", "The target element.")
                ->Attribute("EnumValues", &UiInteractableStateFont::PopulateTargetEntityList);
            editInfo->DataElement("SimpleAssetRef", &UiInteractableStateFont::m_fontFilename, "Font path", "The font asset pathname.")
                ->Attribute("ChangeNotify", &UiInteractableStateFont::OnFontPathnameChange)
                ->Attribute("ChangeNotify", AZ_CRC_CE("RefreshEntireTree"));
            editInfo->DataElement("ComboBox", &UiInteractableStateFont::m_fontEffectIndex, "Font effect", "The font effect (from font file).")
                ->Attribute("EnumValues", &UiInteractableStateFont::PopulateFontEffectList);
        }
    }
}
