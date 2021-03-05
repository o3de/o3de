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

#include "UiInteractableState.h"

#include <LyShine/Bus/UiInteractableStatesBus.h>

// Forward declarations
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiStateActionManager
    : public UiInteractableStatesBus::Handler
{
public: // types

    using StateActions = AZStd::vector<UiInteractableStateAction*>;

public: // member functions

    UiStateActionManager();
    ~UiStateActionManager();

    // UiInteractableStatesInterface
    void SetStateColor(UiInteractableStatesInterface::State state, AZ::EntityId target, const AZ::Color& color) override;
    AZ::Color GetStateColor(State state, AZ::EntityId target) override;
    bool HasStateColor(UiInteractableStatesInterface::State state, AZ::EntityId target) override;

    void SetStateAlpha(UiInteractableStatesInterface::State state, AZ::EntityId target, float alpha) override;
    float GetStateAlpha(State state, AZ::EntityId target) override;
    bool HasStateAlpha(UiInteractableStatesInterface::State state, AZ::EntityId target) override;

    void SetStateSprite(UiInteractableStatesInterface::State state, AZ::EntityId target, ISprite* sprite) override;
    ISprite* GetStateSprite(UiInteractableStatesInterface::State state, AZ::EntityId target) override;
    void SetStateSpritePathname(State state, AZ::EntityId target, const AZStd::string& spritePath) override;
    AZStd::string GetStateSpritePathname(State state, AZ::EntityId target) override;
    bool HasStateSprite(UiInteractableStatesInterface::State state, AZ::EntityId target) override;

    void SetStateFont(State state, AZ::EntityId target, const AZStd::string& fontPathname, unsigned int fontEffectIndex) override;
    AZStd::string GetStateFontPathname(State state, AZ::EntityId target) override;
    unsigned int GetStateFontEffectIndex(State state, AZ::EntityId target) override;
    bool HasStateFont(State state, AZ::EntityId target) override;
    // ~UiInteractableStatesInterface

    //! Initialize the manager
    void Init(AZ::EntityId entityId);

    //! Connect to the bus
    void Activate();

    //! Disconnect from the bus
    void Deactivate();

    //! Use this to add the the state actions for a state. This should be done at initialization time.
    void AddState(StateActions* stateActions);

    //! Reset thes on all visual components being affected by the state actions on all states
    void ResetAllOverrides();

    //! Apply the StateActions for the given state. This will apply any specifieds to the visual components
    void ApplyStateActions(int state);

    //! Whenever a new StateAction is added in the editor we need to initialize the target entity to
    //! the owning entity.
    void InitInteractableEntityForStateActions(StateActions& stateActions);

    //! Remove all the states after deleting each StateAction
    void ClearStates();

public: // static member functions

protected: // types

protected: // member functions

    //! Do any initialization of state actions required after load
    void InitStateActions();

    //! Get a list of all entities that appear as target entities in all of the lists of StateActions
    AZStd::vector<AZ::EntityId> GetTargetEntitiesInAllStates();

    //! Get the state actions for a given state
    StateActions* GetStateActions(int state);

    //! Get the derived class of UiInteractableStateAction for a given state/target and derived class type (if it exists)
    //! e.g.  StateColor* stateColor = GetStateAction<UiInteractableStateColor>(UiInteractableStatesInterface::StateColor, target);
    //! This will return null if no such state action exists.
    template <typename T>
    T* GetStateAction(int state, AZ::EntityId target);

protected: // data members

    AZ::EntityId m_entityId;
    AZStd::vector<StateActions*> m_states;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// INLINE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline T* UiStateActionManager::GetStateAction(int state, AZ::EntityId target)
{
    if (auto stateActions = GetStateActions(state))
    {
        for (auto stateAction : *stateActions)
        {
            T* stateT = azdynamic_cast<T*>(stateAction);
            if (stateT && stateT->GetTargetEntity() == target)
            {
                return stateT;
            }
        }
    }

    return nullptr;
}
