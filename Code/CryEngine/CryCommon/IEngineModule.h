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

// Description : Defines the extension interface for the CryEngine modules.


#ifndef CRYINCLUDE_CRYCOMMON_IENGINEMODULE_H
#define CRYINCLUDE_CRYCOMMON_IENGINEMODULE_H
#pragma once

#include <CryExtension/ICryUnknown.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Console/ConsoleFunctor.h>

struct SSystemInitParams;

// Base Interface for all engine module extensions
struct IEngineModule
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IEngineModule, 0xf899cf661df04f61, 0xa341a8a7ffdf9de4);

    // <interfuscator:shuffle>
    // Retrieve name of the extension module.
    virtual const char* GetName() const = 0;

    // Retrieve category for the extension module (CryEngine for standard modules).
    virtual const char* GetCategory() const = 0;

    // This is called to initialize the new module.
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) = 0;
    // </interfuscator:shuffle>

    // This is called to register any AZ console vars declared within this engine module
    virtual void RegisterConsoleVars()
    {
        AZ::ConsoleFunctorBase*& deferredHead = AZ::ConsoleFunctorBase::GetDeferredHead();
        AZ::Interface<AZ::IConsole>::Get()->LinkDeferredFunctors(deferredHead);
    }
};

#endif // CRYINCLUDE_CRYCOMMON_IENGINEMODULE_H
