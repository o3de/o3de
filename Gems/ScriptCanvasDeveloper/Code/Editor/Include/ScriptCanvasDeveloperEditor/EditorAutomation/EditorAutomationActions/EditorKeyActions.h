/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformIncl.h>

#include <QPoint>
#include <QKeySequence>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationAction.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationAction that will simulate a key action(press or release) for the specified key
    */
    class SimulateKeyAction
        : public EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SimulateKeyAction, AZ::SystemAllocator);
        AZ_RTTI(SimulateKeyAction, "{07259AB5-E5B1-4A4E-8966-610CA3E5F5E3}", EditorAutomationAction);

        enum class KeyAction
        {
            Press,
            Release
        };
        
        SimulateKeyAction(KeyAction keyAction, AZ::u32 keyValue = 0);
        ~SimulateKeyAction() override = default;
        
        bool Tick() override;
        
    private:
    
        [[maybe_unused]] AZ::u32 m_keyValue;
        [[maybe_unused]] KeyAction m_keyAction;
    };

    /**
        EditorAutomationAction that will simulate a press action for the specified key
    */
    class KeyPressAction
        : public SimulateKeyAction
    {
    public:
        AZ_CLASS_ALLOCATOR(KeyPressAction, AZ::SystemAllocator);
        AZ_RTTI(KeyPressAction, "{20F93C69-993C-4658-974B-06F1EE9AFBAC}", SimulateKeyAction);

        explicit KeyPressAction(AZ::u32 keyValue = 0)
            : SimulateKeyAction(SimulateKeyAction::KeyAction::Press, keyValue)
        {
        }
    };

    /**
        EditorAutomationAction that will simulate a release action for the specified key
    */
    class KeyReleaseAction
        : public SimulateKeyAction
    {
    public:
        AZ_CLASS_ALLOCATOR(KeyReleaseAction, AZ::SystemAllocator);
        AZ_RTTI(KeyReleaseAction, "{B4000C12-03F9-4AC1-B1BA-88D8B7FC4CB1}", SimulateKeyAction);

        explicit KeyReleaseAction(AZ::u32 keyValue = 0)
            : SimulateKeyAction(SimulateKeyAction::KeyAction::Release, keyValue)
        {
        }
    };

    /**
        EditorAutomationAction that will simulate a press and a release action for the specified key(currently limited support for QChar to a-z,0-9, *, (, and ) )
    */
    class TypeCharAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(TypeCharAction, AZ::SystemAllocator);
        AZ_RTTI(TypeCharAction, "{67FBD994-18BB-4434-AD8E-70F446A9B185}", CompoundAction);

        explicit TypeCharAction(QChar qChar);
        explicit TypeCharAction(AZ::u32 key);
    };

    /**
        EditorAutomationAction that will simulate a user typing in the specified string.
    */
    class TypeStringAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(TypeStringAction, AZ::SystemAllocator);
        AZ_RTTI(TypeStringAction, "{51D701B1-6386-420C-A1BD-D72272BBBBD5}", CompoundAction);

        explicit TypeStringAction(QString targetString);
    };
}
