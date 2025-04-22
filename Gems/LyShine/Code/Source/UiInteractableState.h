/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiInteractableStatesBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/IDraw2d.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <AzCore/Math/Color.h>
#include <LmbrCentral/Rendering/TextureAsset.h>
#include <LyShine/UiAssetTypes.h>

#include <IFont.h>

// Forward declarations
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Base class for all interactable state actions
// Interactable state actions are properties that are set while in that interactable state
// (e.g. color override) or things that happen when entering that state (e.g. playing an animation)
class UiInteractableStateAction
{
public: // member functions
    AZ_CLASS_ALLOCATOR(UiInteractableStateAction, AZ::SystemAllocator);
    AZ_RTTI(UiInteractableStateAction, "{D86C82E1-E027-453F-A43B-BD801CF88391}");

    virtual ~UiInteractableStateAction() {}

    static void Reflect(AZ::ReflectContext* context);

    //! Called from the Init of the UiInteractableComponent
    virtual void Init(AZ::EntityId);

    //! Apply state or do action
    virtual void ApplyState() = 0;

    virtual void SetInteractableEntity(AZ::EntityId interactableEntityId);
    virtual AZ::EntityId GetTargetEntity() { return AZ::EntityId(); }

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateTargetEntityList();

protected: // data

    //! The interactable entity that this state belongs to.
    AZ::EntityId m_interactableEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiInteractableStateColor
    : public UiInteractableStateAction
{
public: // member functions
    AZ_CLASS_ALLOCATOR(UiInteractableStateColor, AZ::SystemAllocator);
    AZ_RTTI(UiInteractableStateColor, "{D7978A94-592F-4E1A-86EF-E34A819A55FB}", UiInteractableStateAction);

    UiInteractableStateColor();
    UiInteractableStateColor(AZ::EntityId target, AZ::Color color);

    // UiInteractableStateAction
    void Init(AZ::EntityId) override;
    void ApplyState() override;
    void SetInteractableEntity(AZ::EntityId interactableEntityId) override;
    AZ::EntityId GetTargetEntity() override { return m_targetEntity; }
    // ~UiInteractableStateAction

    AZ::Color GetColor() { return m_color; }
    void SetColor(AZ::Color color) { m_color = color; }

    EntityComboBoxVec PopulateTargetEntityList();

public: // static member functions
    static void Reflect(AZ::ReflectContext* context);

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

protected: // data
    AZ::EntityId m_targetEntity;
    AZ::Color m_color;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiInteractableStateAlpha
    : public UiInteractableStateAction
{
public: // member functions
    AZ_CLASS_ALLOCATOR(UiInteractableStateAlpha, AZ::SystemAllocator);
    AZ_RTTI(UiInteractableStateAlpha, "{ABCD5D45-CC47-4C17-8D21-9471032618F6}", UiInteractableStateAction);

    UiInteractableStateAlpha();
    UiInteractableStateAlpha(AZ::EntityId target, float alpha);

    // UiInteractableStateAction
    void Init(AZ::EntityId) override;
    void ApplyState() override;
    void SetInteractableEntity(AZ::EntityId interactableEntityId) override;
    AZ::EntityId GetTargetEntity() override { return m_targetEntity; }
    // ~UiInteractableStateAction

    float GetAlpha() { return m_alpha; }
    void SetAlpha(float alpha) { m_alpha = alpha; }

    EntityComboBoxVec PopulateTargetEntityList();

public: // static member functions
    static void Reflect(AZ::ReflectContext* context);

protected: // data
    AZ::EntityId m_targetEntity;
    float m_alpha;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiInteractableStateSprite
    : public UiInteractableStateAction
{
public: // member functions
    AZ_CLASS_ALLOCATOR(UiInteractableStateSprite, AZ::SystemAllocator);
    AZ_RTTI(UiInteractableStateSprite, "{89294558-CF45-4AA8-9EAA-A1D81BAB92A7}", UiInteractableStateAction);

    UiInteractableStateSprite();
    UiInteractableStateSprite(AZ::EntityId target, ISprite* sprite);
    UiInteractableStateSprite(AZ::EntityId target, const AZStd::string& spritePath);
    ~UiInteractableStateSprite() override;

    // UiInteractableStateAction
    void Init(AZ::EntityId) override;
    void ApplyState() override;
    void SetInteractableEntity(AZ::EntityId interactableEntityId) override;
    AZ::EntityId GetTargetEntity() override { return m_targetEntity; }
    // ~UiInteractableStateAction

    ISprite* GetSprite() { return m_sprite; }
    void SetSprite(ISprite* sprite);

    AZStd::string GetSpritePathname();
    void SetSpritePathname(const AZStd::string& spritePath);

    EntityComboBoxVec PopulateTargetEntityList();
    void OnSpritePathnameChange();

public: // static member functions
    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    bool IsSpriteSheet();
    void OnTargetElementChange();
    void LoadSpriteFromTargetElement();

    using AZu32ComboBoxVec = AZStd::vector<AZStd::pair<AZ::u32, AZStd::string> >;

    //! Returns a string representation of the indices used to index sprite-sheet types.
    AZu32ComboBoxVec PopulateIndexStringList() const;

protected: // data
    AZ::EntityId m_targetEntity;
    AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_spritePathname;
    ISprite* m_sprite = nullptr;
    AZ::u32 m_spriteSheetCellIndex = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiInteractableStateFont
    : public UiInteractableStateAction
    , public FontNotificationBus::Handler
{
public: // types
    using FontEffectComboBoxVec = AZStd::vector < AZStd::pair<unsigned int, AZStd::string> >;

public: // member functions
    AZ_CLASS_ALLOCATOR(UiInteractableStateFont, AZ::SystemAllocator);
    AZ_RTTI(UiInteractableStateFont, "{0E39A3BC-CEF5-4385-9D06-BFEE189E77E1}", UiInteractableStateAction);

    UiInteractableStateFont();
    UiInteractableStateFont(AZ::EntityId target, const AZStd::string& pathname, unsigned int fontEffectIndex);
    ~UiInteractableStateFont() override;

    // UiInteractableStateAction
    void Init(AZ::EntityId) override;
    void ApplyState() override;
    void SetInteractableEntity(AZ::EntityId interactableEntityId) override;
    AZ::EntityId GetTargetEntity() override { return m_targetEntity; }
    // ~UiInteractableStateAction

    // FontNotifications
    void OnFontsReloaded() override;
    // ~FontNotifications

    const AZStd::string&  GetFontPathname() { return m_fontFilename.GetAssetPath(); }
    void SetFontPathname(const AZStd::string& pathname);

    const unsigned int  GetFontEffectIndex() { return m_fontEffectIndex; }
    void SetFontEffectIndex(unsigned int index) { m_fontEffectIndex = index; }

    EntityComboBoxVec PopulateTargetEntityList();

    //! Populate the list for the font effect combo box in the properties pane
    FontEffectComboBoxVec PopulateFontEffectList();

    void OnFontPathnameChange();

public: // static member functions
    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    void InitCommon(const AZStd::string& fontPathname);

protected: // data
    AZ::EntityId m_targetEntity;
    AzFramework::SimpleAssetReference<LyShine::FontAsset> m_fontFilename;
    FontFamilyPtr m_fontFamily;
    unsigned int m_fontEffectIndex;
};

