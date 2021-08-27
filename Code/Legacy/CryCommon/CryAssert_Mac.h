/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Assert dialog box for Mac OS X


#ifndef CRYINCLUDE_CRYCOMMON_CRYASSERT_MAC_H
#define CRYINCLUDE_CRYCOMMON_CRYASSERT_MAC_H
#pragma once

#if defined(USE_CRY_ASSERT) && defined(MAC)
#include <AzCore/NativeUI/NativeUIRequests.h>

static char gs_szMessage[MAX_PATH];

void CryAssertTrace(const char* szFormat, ...)
{
    if (gEnv == 0)
    {
        return;
    }

    if (!gEnv->bIgnoreAllAsserts)
    {
        if (szFormat == NULL)
        {
            gs_szMessage[0] = '\0';
        }
        else
        {
            va_list args;
            va_start(args, szFormat);
            vsnprintf(gs_szMessage, sizeof(gs_szMessage), szFormat, args);
            va_end(args);
        }
    }
}

bool CryAssert(const char* szCondition, const char* szFile, unsigned int line, bool* pIgnore)
{
    if (!gEnv)
    {
        return false;
    }

#if defined(CRY_ASSERT_DIALOG_ONLY_IN_DEBUG) && !defined(AZ_DEBUG_BUILD)
    // we are in a non-debug build, so we should turn this into a warning instead.
    if ((gEnv) && (gEnv->pLog))
    {
        if (!gEnv->bIgnoreAllAsserts)
        {
            gEnv->pLog->LogWarning("%s(%u): Assertion failed - \"%s\"", szFile, line, szCondition);
        }
    }
    
    if (pIgnore)
    {
        // avoid showing the same one repeatedly.
        *pIgnore = true;
    }
    return false;
#endif

    static const int max_len = 4096;
    static char gs_command_str[4096];

    gEnv->pSystem->OnAssert(szCondition, gs_szMessage, szFile, line);

    size_t file_len = strlen(szFile);

    if (!gEnv->bNoAssertDialog && !gEnv->bIgnoreAllAsserts)
    {
        AZ::NativeUI::AssertAction result;
        EBUS_EVENT_RESULT(result, AZ::NativeUI::NativeUIRequestBus, DisplayAssertDialog, gs_szMessage);
        
        switch(result)
        {
            case AZ::NativeUI::AssertAction::IGNORE_ASSERT:
                return false;
            case AZ::NativeUI::AssertAction::IGNORE_ALL_ASSERTS:
                gEnv->bNoAssertDialog = true;
                gEnv->bIgnoreAllAsserts = true;
                return false;
            case AZ::NativeUI::AssertAction::BREAK:
                return true;
            default:
                break;
        }
        
        // For asserts on the Mac always trigger a debug break. Annoying but at least it does not kill the thread like assert() does.
        __asm__("int $3");
    }


    return false;
}


#endif

#endif // CRYINCLUDE_CRYCOMMON_CRYASSERT_MAC_H
