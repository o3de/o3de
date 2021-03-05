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

#include "ResourceCompilerXML_precompiled.h"
#include "IResCompiler.h"
#include "XMLConverter.h"
#include "IRCLog.h"
#include "XML/xml.h"
#include <CryLibrary.h>
#include <CryCommon/ISystem.h>
#include <ResourceCompiler/ResourceCompiler.h>

XmlStrCmpFunc g_pXmlStrCmp = &azstricmp;

extern "C" DLL_EXPORT void __stdcall RegisterConvertors(IResourceCompiler* pRC)
{
    PREVENT_MODULE_AND_ENVIRONMENT_SYMBOL_STRIPPING

    SetRCLog(pRC->GetIRCLog());

    ICryXML* pCryXML = LoadICryXML();
    if (pCryXML == 0)
    {
        RCLogError("Loading xml library failed - not registering xml converter.");
        return;
    }

    pCryXML->AddRef();

    pRC->RegisterConvertor("XMLConverter", new XMLConverter(pCryXML));

    pRC->RegisterKey("xmlFilterFile", "specify file with special commands to filter out unneeded XML elements and attributes");

    pCryXML->Release();
}

