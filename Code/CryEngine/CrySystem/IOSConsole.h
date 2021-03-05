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

// Description : Console implementation for iOS, reports back to the main interface


#ifndef CRYINCLUDE_CRYSYSTEM_IOSCONSOLE_H
#define CRYINCLUDE_CRYSYSTEM_IOSCONSOLE_H
#pragma once


#include <IConsole.h>
#include <ITextModeConsole.h>
#include <INetwork.h>



class CIOSConsole
    : public ISystemUserCallback
    , public IOutputPrintSink
    , public ITextModeConsole
{
    CIOSConsole(const CIOSConsole&);
    CIOSConsole& operator = (const CIOSConsole&);

    bool m_isInitialized;
public:
    static CryCriticalSectionNonRecursive s_lock;
public:
    CIOSConsole();
    ~CIOSConsole();

    // Interface IOutputPrintSink /////////////////////////////////////////////
    DLL_EXPORT virtual void Print(const char* line);

    // Interface ISystemUserCallback //////////////////////////////////////////
    virtual bool OnError(const char* errorString);
    virtual bool OnSaveDocument() { return false; }
    virtual void OnProcessSwitch() { }
    virtual void OnInitProgress(const char* sProgressMsg);
    virtual void OnInit(ISystem*);
    virtual void OnShutdown();
    virtual void OnUpdate();
    virtual void GetMemoryUsage(ICrySizer* pSizer);
    void SetRequireDedicatedServer(bool) {}
    void SetHeader(const char*) {}
    // Interface ITextModeConsole /////////////////////////////////////////////
    virtual Vec2_tpl<int> BeginDraw();
    virtual void PutText(int x, int y, const char* msg);
    virtual void EndDraw();
};

#endif // CRYINCLUDE_CRYSYSTEM_IOSCONSOLE_H
