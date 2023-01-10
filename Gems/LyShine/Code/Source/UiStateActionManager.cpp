/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiStateActionManager.h"

#include <LyShine/Bus/UiVisualBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
UiStateActionManager::UiStateActionManager()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiStateActionManager::~UiStateActionManager()
{
    ClearStates();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::SetStateColor(int state, AZ::EntityId target, const AZ::Color& color)
{
    UiInteractableStateColor* stateColor = GetStateAction<UiInteractableStateColor>(state, target);
    if (stateColor)
    {
        stateColor->SetColor(color);
    }
    else
    {
        StateActions* stateActions = GetStateActions(state);
        if (stateActions)
        {
            stateActions->push_back(new UiInteractableStateColor(target, color));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiStateActionManager::GetStateColor(int state, AZ::EntityId target)
{
    AZ::Color color(0.0f, 0.0f, 0.0f, 1.0f);

    UiInteractableStateColor* stateColor = GetStateAction<UiInteractableStateColor>(state, target);
    if (stateColor)
    {
        color = stateColor->GetColor();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "GetStateColor: Couldn't find color action for state/target combination");
    }

    return color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiStateActionManager::HasStateColor(int state, AZ::EntityId target)
{
    UiInteractableStateColor* stateColor = GetStateAction<UiInteractableStateColor>(state, target);
    return stateColor ? true : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::SetStateAlpha(int state, AZ::EntityId target, float alpha)
{
    UiInteractableStateAlpha* stateAlpha = GetStateAction<UiInteractableStateAlpha>(state, target);
    if (stateAlpha)
    {
        stateAlpha->SetAlpha(alpha);
    }
    else
    {
        StateActions* stateActions = GetStateActions(state);
        if (stateActions)
        {
            stateActions->push_back(new UiInteractableStateAlpha(target, alpha));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiStateActionManager::GetStateAlpha(int state, AZ::EntityId target)
{
    float alpha = 1.0f;

    UiInteractableStateAlpha* stateAlpha = GetStateAction<UiInteractableStateAlpha>(state, target);
    if (stateAlpha)
    {
        alpha = stateAlpha->GetAlpha();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "GetStateAlpha: Couldn't find alpha action for state/target combination");
    }

    return alpha;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiStateActionManager::HasStateAlpha(int state, AZ::EntityId target)
{
    UiInteractableStateAlpha* stateAlpha = GetStateAction<UiInteractableStateAlpha>(state, target);
    return stateAlpha ? true : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::SetStateSprite(int state, AZ::EntityId target, ISprite* sprite)
{
    UiInteractableStateSprite* stateSprite = GetStateAction<UiInteractableStateSprite>(state, target);
    if (stateSprite)
    {
        stateSprite->SetSprite(sprite);
    }
    else
    {
        StateActions* stateActions = GetStateActions(state);
        if (stateActions)
        {
            stateActions->push_back(new UiInteractableStateSprite(target, sprite));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* UiStateActionManager::GetStateSprite(int state, AZ::EntityId target)
{
    ISprite* sprite = nullptr;

    UiInteractableStateSprite* stateSprite = GetStateAction<UiInteractableStateSprite>(state, target);
    if (stateSprite)
    {
        sprite = stateSprite->GetSprite();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "GetStateSprite: Couldn't find sprite action for state/target combination");
    }

    return sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::SetStateSpritePathname(int state, AZ::EntityId target, const AZStd::string& spritePath)
{
    UiInteractableStateSprite* stateSprite = GetStateAction<UiInteractableStateSprite>(state, target);
    if (stateSprite)
    {
        stateSprite->SetSpritePathname(spritePath);
    }
    else
    {
        StateActions* stateActions = GetStateActions(state);
        if (stateActions)
        {
            stateActions->push_back(new UiInteractableStateSprite(target, spritePath));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiStateActionManager::GetStateSpritePathname(int state, AZ::EntityId target)
{
    AZStd::string spritePath;

    UiInteractableStateSprite* stateSprite = GetStateAction<UiInteractableStateSprite>(state, target);
    if (stateSprite)
    {
        spritePath = stateSprite->GetSpritePathname();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "GetStateSpritePathname: Couldn't find sprite action for state/target combination");
    }

    return spritePath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiStateActionManager::HasStateSprite(int state, AZ::EntityId target)
{
    UiInteractableStateSprite* stateSprite = GetStateAction<UiInteractableStateSprite>(state, target);
    return stateSprite ? true : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::SetStateFont(
    int state,
    AZ::EntityId target,
    const AZStd::string& fontPathname,
    unsigned int fontEffectIndex)
{
    UiInteractableStateFont* stateFont = GetStateAction<UiInteractableStateFont>(state, target);
    if (stateFont)
    {
        stateFont->SetFontPathname(fontPathname);
        stateFont->SetFontEffectIndex(fontEffectIndex);
    }
    else
    {
        StateActions* stateActions = GetStateActions(state);
        if (stateActions)
        {
            stateActions->push_back(new UiInteractableStateFont(target, fontPathname, fontEffectIndex));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiStateActionManager::GetStateFontPathname(int state, AZ::EntityId target)
{
    AZStd::string fontPath;

    UiInteractableStateFont* stateFont = GetStateAction<UiInteractableStateFont>(state, target);
    if (stateFont)
    {
        fontPath = stateFont->GetFontPathname();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "GetStateFontPathname: Couldn't find font action for state/target combination");
    }

    return fontPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int UiStateActionManager::GetStateFontEffectIndex(int state, AZ::EntityId target)
{
    unsigned int fontEffectIndex = 0;

    UiInteractableStateFont* stateFont = GetStateAction<UiInteractableStateFont>(state, target);
    if (stateFont)
    {
        fontEffectIndex = stateFont->GetFontEffectIndex();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "GetStateFontEffectIndex: Couldn't find font action for state/target combination");
    }

    return fontEffectIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiStateActionManager::HasStateFont(int state, AZ::EntityId target)
{
    UiInteractableStateFont* stateFont = GetStateAction<UiInteractableStateFont>(state, target);
    return stateFont ? true : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::Init(AZ::EntityId entityId)
{
    m_entityId = entityId;
    InitStateActions();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::Activate()
{
    UiInteractableStatesBus::Handler::BusConnect(m_entityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::Deactivate()
{
    UiInteractableStatesBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::AddState(StateActions* stateActions)
{
    m_states.push_back(stateActions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::ResetAllOverrides()
{
    AZStd::vector<AZ::EntityId> entityIdList = GetTargetEntitiesInAllStates();
    for (auto targetEntityId : entityIdList)
    {
        UiVisualBus::Event(targetEntityId, &UiVisualBus::Events::ResetOverrides);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::ApplyStateActions(int state)
{
    // Get the actions for the state (if any) and apply them
    StateActions* stateActions = m_states[state];
    if (stateActions)
    {
        for (UiInteractableStateAction* stateAction : (*stateActions))
        {
            stateAction->ApplyState();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::InitInteractableEntityForStateActions(StateActions& stateActions)
{
    for (auto stateAction : stateActions)
    {
        stateAction->SetInteractableEntity(m_entityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::ClearStates()
{
    for (StateActions* stateActions : m_states)
    {
        if (stateActions)
        {
            for (UiInteractableStateAction* stateAction : * stateActions)
            {
                delete stateAction;
            }
        }
    }

    m_states.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiStateActionManager::InitStateActions()
{
    for (StateActions* stateActions : m_states)
    {
        if (stateActions)
        {
            for (UiInteractableStateAction* stateAction : * stateActions)
            {
                stateAction->Init(m_entityId);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::vector<AZ::EntityId> UiStateActionManager::GetTargetEntitiesInAllStates()
{
    AZStd::vector<AZ::EntityId> result;

    for (StateActions* stateActions : m_states)
    {
        if (stateActions)
        {
            for (auto stateAction : (*stateActions))
            {
                AZ::EntityId targetEntity = stateAction->GetTargetEntity();

                // if this state action has a target entity and it is not already in the list then add it
                if (targetEntity.IsValid() && AZStd::find(result.begin(), result.end(), targetEntity) == result.end())
                {
                    result.push_back(targetEntity);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiStateActionManager::StateActions* UiStateActionManager::GetStateActions(int state)
{
    if (state >= 0 && state < m_states.size())
    {
        return m_states[state];
    }

    return nullptr;
}
