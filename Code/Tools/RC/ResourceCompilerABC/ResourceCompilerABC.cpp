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

#include "ResourceCompilerABC_precompiled.h"
#include "IResCompiler.h"
#include "AlembicConvertor.h"
#include "IRCLog.h"
#include "CryLibrary.h"
#include "../../CryXML/ICryXML.h"
#include <AzCore/Utils/Utils.h>
#include <ResourceCompiler/ResourceCompiler.h>

extern "C" DLL_EXPORT void __stdcall RegisterConvertors(IResourceCompiler* pRC)
{
    PREVENT_MODULE_AND_ENVIRONMENT_SYMBOL_STRIPPING

    SetRCLog(pRC->GetIRCLog());

    ICryXML* const pCryXML = LoadICryXML();

    if (pCryXML == 0)
    {
        RCLogError("Loading xml library failed - not registering alembic converter.");
    }
    else
    {
        pRC->RegisterConvertor("AlembicCompiler", new AlembicConvertor(pCryXML, pRC->GetPakSystem()));

        pRC->RegisterKey("upAxis", "[ABC] Up axis of alembic file\n"
            "Z = Use Z as up axis: No conversion\n"
            "Y = Use Y as up axis: Convert Y up to Z up (default)");

        pRC->RegisterKey("meshPrediction", "[ABC] Use mesh prediction for index frames\n"
            "0 = No mesh prediction (default)\n"
            "1 = Use mesh prediction");

        pRC->RegisterKey("useBFrames", "[ABC] Use bi-directional predicted frames\n"
            "0 = Don't use b-frames (default)\n"
            "1 = Use b-frames");

        pRC->RegisterKey("indexFrameDistance", "[ABC] Index frame distance when using b-frames (default is 15)");

        pRC->RegisterKey("blockCompressionFormat", "[ABC] Method used to compress data\n"
            "store = No compression\n"
            "deflate = Use deflate (zlib) compression (default)");

        pRC->RegisterKey("playbackFromMemory", "[ABC] Set flag that resulting cache will be played back from memory\n"
            "0 = Do not play back from memory (default)\n"
            "1 = Cache plays from memory after loading");

        pRC->RegisterKey("positionPrecision", "[ABC] Set the position precision in mm. Higher values usually result in better compression (default is 1)");

        pRC->RegisterKey("uvMax", "[ABC] Set the upper value of the UV range. Values above this value will be wrapped.\n"
            "0 = use detected per-mesh uvMax values. (default is 0)");

        pRC->RegisterKey("skipFilesWithoutBuildConfig", "[ABC] Skip files without build configuration (.CBC)");
    }
}
