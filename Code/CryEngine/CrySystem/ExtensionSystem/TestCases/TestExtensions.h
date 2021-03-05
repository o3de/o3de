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

// Description : Part of CryEngine's extension framework.


#ifndef CRYINCLUDE_CRYSYSTEM_EXTENSIONSYSTEM_TESTCASES_TESTEXTENSIONS_H
#define CRYINCLUDE_CRYSYSTEM_EXTENSIONSYSTEM_TESTCASES_TESTEXTENSIONS_H
#pragma once


//#define EXTENSION_SYSTEM_INCLUDE_TESTCASES

#ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES

#include <CryExtension/ICryUnknown.h>

struct ICryFactoryRegistryImpl;

void TestExtensions(ICryFactoryRegistryImpl* pReg);

struct IFoobar
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IFoobar, 0x539e9c672cad4a03, 0x9ecd8069c99a846b)

    virtual void Foo() = 0;
};

DECLARE_SMART_POINTERS(IFoobar);

struct IRaboof
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IRaboof, 0x135ca25e634b4d13, 0x9e4467968a708822)

    virtual void Rab() = 0;
};

DECLARE_SMART_POINTERS(IRaboof);

struct IA
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IA, 0xd93aaceb35ec427e, 0xb64bf8dec4997e67)

    virtual void A() = 0;
};

DECLARE_SMART_POINTERS(IA);

struct IB
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IB, 0xe0d830c826424e11, 0x9eacfa19eaf31ffb)

    virtual void B() = 0;
};

DECLARE_SMART_POINTERS(IB);

struct IC
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IC, 0x577509a20fc5477c, 0x893757c9ca88b27b)

    virtual void C() = 0;
};

DECLARE_SMART_POINTERS(IC);

struct ICustomC
    : public IC
{
    CRYINTERFACE_DECLARE(ICustomC, 0x2ac769da4c7443bf, 0x80911033e21dfbcf)

    virtual void C1() = 0;
};

DECLARE_SMART_POINTERS(ICustomC);

//////////////////////////////////////////////////////////////////////////
// use of extension system without any of the helper macros/templates

struct IDontLikeMacros
    : public ICryUnknown
{
    template <class T>
    friend const CryInterfaceID& InterfaceCastSemantics::cryiidof();
    template <class T>
    friend void AZStd::Internal::sp_ms_deleter<T>::destroy();
    template <class T>
    friend AZStd::shared_ptr<T> AZStd::make_shared<T>();
protected:
    virtual ~IDontLikeMacros() {}

private:
    // It's very important that this static function is implemented for each interface!
    // Otherwise the consistency of cryinterface_cast<T>() is compromised because
    // cryiidof<T>() = cryiidof<baseof<T>>() {baseof<T> = ICryUnknown in most cases}
    static const CryInterfaceID& IID()
    {
        static const CryInterfaceID iid = {0x0f43b7e3f1364af0ull, 0xb4a16a975bea3ec4ull};
        return iid;
    }

public:
    virtual void CallMe() = 0;
};

DECLARE_SMART_POINTERS(IDontLikeMacros);


#endif // #ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES

#endif // CRYINCLUDE_CRYSYSTEM_EXTENSIONSYSTEM_TESTCASES_TESTEXTENSIONS_H
