/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that the UI Editor Animation Window needs to implement
//! (e.g. UiAnimViewDialog)

class CUiAnimViewDialog;
class CUiAnimViewSequence;
class CUiAnimationContext;
struct IUiAnimationSystem;

class UiEditorAnimationInterface
    : public AZ::EBusTraits
{
public: // member functions

    virtual ~UiEditorAnimationInterface(){}

    //! Get the animation context for the UI animation window
    virtual CUiAnimationContext* GetAnimationContext() = 0;

    //! Get the active UI animation system, this is the animation system for the active canvas
    virtual IUiAnimationSystem* GetAnimationSystem() = 0;

    //! Get the active UI animation sequence in the UI Animation Window
    virtual CUiAnimViewSequence* GetCurrentSequence() = 0;

    //! Called when the active canvas in the UI Editor window changes so that the UI Editor
    //! animation window can update to show the correct sequences. Active canvas could change
    //! from a valid entity Id to an invalid entity Id and vice versa
    virtual void ActiveCanvasChanged() = 0;

public: // static member functions

    static const char* GetUniqueName() { return "UiEditorAnimationInterface"; }
};

typedef AZ::EBus<UiEditorAnimationInterface> UiEditorAnimationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that the UI Editor Animation Window needs to implement
//! (e.g. UiAnimViewDialog)

class UiEditorAnimationStateInterface
    : public AZ::EBusTraits
{
public: // types

    struct UiEditorAnimationEditState
    {
        AZStd::string m_sequenceName;
        float m_time;
        float m_timelineScale;
        int m_timelineScrollOffset;
    };

public: // member functions

    virtual ~UiEditorAnimationStateInterface(){}

    //! Get the current animation edit state
    virtual UiEditorAnimationEditState GetCurrentEditState() = 0;

    //! Restore the current animation edit state
    virtual void RestoreCurrentEditState(const UiEditorAnimationEditState& animEditState) = 0;

public: // static member functions

    static const char* GetUniqueName() { return "UiEditorAnimationStateInterface"; }
};

typedef AZ::EBus<UiEditorAnimationStateInterface> UiEditorAnimationStateBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Listener class that any UI Editor Animation class can implement to get notifications

class UiEditorAnimListenerInterface
    : public AZ::EBusTraits
{
public: // member functions

    virtual ~UiEditorAnimListenerInterface(){}

    //! Called when the active canvas in the UI Editor window changes.
    //! When this is called the UiEditorAnimationBus may be used to get the new active canvas.
    //! Active canvas could change from a valid entity Id to an invalid entity Id and vice versa
    virtual void OnActiveCanvasChanged() = 0;

    //! Called when UI elements have been deleted from or re-added to the canvas
    //! This requires the sequences to be updated
    virtual void OnUiElementsDeletedOrReAdded() {};

public: // static member functions

    static const char* GetUniqueName() { return "UiEditorAnimListenerInterface"; }
};

typedef AZ::EBus<UiEditorAnimListenerInterface> UiEditorAnimListenerBus;
