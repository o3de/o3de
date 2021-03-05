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

#include "ResourceCompilerScene_precompiled.h"
#include <platform.h>
#include <IResCompiler.h>
#include <IRCLog.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneConfig.h>
#include <SceneConverter.h>

extern "C"
{
    DLL_EXPORT void __stdcall RegisterConvertors(IResourceCompiler* pRC)
    {
        PREVENT_MODULE_AND_ENVIRONMENT_SYMBOL_STRIPPING

        SetRCLog(pRC->GetIRCLog());

        AZStd::shared_ptr<AZ::RC::SceneConfig> config = AZStd::make_shared<AZ::RC::SceneConfig>();
        pRC->RegisterConvertor("SceneConverter", new AZ::RC::SceneConverter(config));
    }
}
