/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <QApplication>
#include <QKeyEvent>
#include <QWidget>

// MSVC: warning suppressed: constant used in conditional expression
// CLANG: warning suppressed: implicit conversion loses integer precision
AZ_PUSH_DISABLE_WARNING(4127, "-Wshorten-64-to-32")
#include <QtTest/qtest.h>
AZ_POP_DISABLE_WARNING

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>

namespace ScriptCanvas::Developer
{
    //////////////////////
    // SimulateKeyAction
    //////////////////////

    SimulateKeyAction::SimulateKeyAction(KeyAction keyAction, AZ::u32 keyValue)
        : m_keyValue(keyValue)
        , m_keyAction(keyAction)
    {
    }

    bool SimulateKeyAction::Tick()
    {
#if defined(AZ_COMPILER_MSVC)
        INPUT osInput = { 0 };
        osInput.type = INPUT_KEYBOARD;
        osInput.ki.wVk = static_cast<WORD>(m_keyValue);

        switch (m_keyAction)
        {
        case KeyAction::Press:
            break;
        case KeyAction::Release:
            osInput.ki.dwFlags = KEYEVENTF_KEYUP;
            break;
        }

        ::SendInput(1, &osInput, sizeof(INPUT));
#endif
        return true;
    }

    ///////////////////
    // TypeCharAction
    ///////////////////

    TypeCharAction::TypeCharAction(QChar testCharacter)
    {
#if defined(AZ_COMPILER_MSVC)
        // This is a. Going to use this to manage most of my elements
        AZ::u32 aOffset = 0x41;
        AZ::u32 zeroOffset = 0x30;

        ushort unicodeA = QChar('a').unicode();
        ushort unicodeZero = QChar('0').unicode();

        if (testCharacter.isLetterOrNumber())
        {
            ushort unicodeValue = testCharacter.unicode();

            AZ::u32 offsetStart = 0;
            ushort difference = 0;

            if (testCharacter.isLetter())
            {
                offsetStart = aOffset;
                difference = unicodeValue - unicodeA;
            }
            else if (testCharacter.isNumber())
            {
                offsetStart = zeroOffset;
                difference = unicodeValue - unicodeZero;
            }

            AZ::u32 finalKey = offsetStart + difference;
            AddAction(aznew TypeCharAction(finalKey));
        }
        else if (testCharacter == QChar(' '))
        {
            AddAction(aznew TypeCharAction(VK_SPACE));
        }
        else if (testCharacter == QChar('*'))
        {
            AddAction(aznew KeyPressAction(VK_SHIFT));
            AddAction(aznew TypeCharAction(unicodeZero + 8));
            AddAction(aznew KeyReleaseAction(VK_SHIFT));
        }
        else if (testCharacter == QChar('('))
        {
            AddAction(aznew KeyPressAction(VK_SHIFT));
            AddAction(aznew TypeCharAction(unicodeZero + 9));
            AddAction(aznew KeyReleaseAction(VK_SHIFT));
        }
        else if (testCharacter == QChar(')'))
        {
            AddAction(aznew KeyPressAction(VK_SHIFT));
            AddAction(aznew TypeCharAction(unicodeZero + 0));
            AddAction(aznew KeyReleaseAction(VK_SHIFT));
        }
        else if (testCharacter == QChar(':'))
        {
            AddAction(aznew KeyPressAction(VK_SHIFT));
            AddAction(aznew TypeCharAction(VK_OEM_1));
            AddAction(aznew KeyReleaseAction(VK_SHIFT));
        }
        else if (testCharacter == QChar('.'))
        {
            AddAction(aznew TypeCharAction(VK_OEM_PERIOD));
        }
#endif
    }

    TypeCharAction::TypeCharAction(AZ::u32 keyValue)
    {
        AddAction(aznew KeyPressAction(keyValue));
        AddAction(aznew KeyReleaseAction(keyValue));        
    }

    /////////////////////
    // TypeStringAction
    /////////////////////

    TypeStringAction::TypeStringAction(QString targetString)
    {
        for (int i = 0; i < targetString.size(); ++i)
        {
            QChar testCharacter = targetString.at(i).toLower();
            AddAction(aznew TypeCharAction(testCharacter));
        }
    }
}
