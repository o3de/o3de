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

// Description : Allows creation of text mode displays the for dedicated server


#ifndef CRYINCLUDE_CRYCOMMON_ITEXTMODECONSOLE_H
#define CRYINCLUDE_CRYCOMMON_ITEXTMODECONSOLE_H
#pragma once


struct ITextModeConsole
{
    // <interfuscator:shuffle>
    virtual ~ITextModeConsole() {}
    virtual Vec2_tpl<int> BeginDraw() = 0;
    virtual void PutText(int x, int y, const char* msg) = 0;
    virtual void EndDraw() = 0;
    virtual void OnShutdown() = 0;

    virtual void SetTitle([[maybe_unused]] const char* title) {}
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ITEXTMODECONSOLE_H
