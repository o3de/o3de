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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Assert dialog box for android


#ifndef CRYINCLUDE_CRYCOMMON_CRYASSERT_ANDROID_H
#define CRYINCLUDE_CRYCOMMON_CRYASSERT_ANDROID_H
#pragma once

#if defined(USE_CRY_ASSERT) && defined(ANDROID)

#include <AzCore/NativeUI/NativeUIRequests.h>

static char gs_szMessage[MAX_PATH];

void CryAssertTrace(const char* szFormat, ...)
{
    if (gEnv == 0)
    {
        return;
    }

    if (!gEnv->bIgnoreAllAsserts || gEnv->bTesting)
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
        return true;
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

    gEnv->pSystem->OnAssert(szCondition, gs_szMessage, szFile, line);

    if (!gEnv->bNoAssertDialog && !gEnv->bIgnoreAllAsserts)
    {
        AZ::NativeUI::AssertAction result;
        EBUS_EVENT_RESULT(result, AZ::NativeUI::NativeUIRequestBus, DisplayAssertDialog, gs_szMessage);

        switch (result)
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
        
        return true;
    }
    else
    {
        return false;
    }
}

#endif
#endif // CRYINCLUDE_CRYCOMMON_CRYASSERT_ANDROID_H
