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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_DEBUGLOG_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_DEBUGLOG_H
#pragma once

inline void DebugLog (const char* szFormat, ...)
{
    
    FILE* f = nullptr;
    azfopen(&f, "Rc.Debug.log", "wa");
    if (!f)
    {
        return;
    }
    va_list args;
    va_start (args, szFormat);
    vfprintf (f, szFormat, args);
    fprintf (f, "\n");
    vprintf (szFormat, args);
    printf ("\n");
    va_end(args);
    fclose (f);
}

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_DEBUGLOG_H
