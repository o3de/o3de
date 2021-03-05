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

#include "ResourceCompilerLegacy_precompiled.h"
#include <LegacyConverter.h>
#include <LegacyCompiler.h>
#include <AzCore/std/iterator.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace RC
    {
        LegacyConverter::LegacyConverter()
        {
        }

        void LegacyConverter::Release()
        {
            delete this;
        }
        void LegacyConverter::Init([[maybe_unused]] const ConvertorInitContext& context)
        {
        }

        ICompiler* LegacyConverter::CreateCompiler()
        {
            return new LegacyCompiler();
        }

        const char* LegacyConverter::GetExt(int index) const
        {
            // List all the file types that should use this resource compiler module to retrieve product dependencies.
            // These file types require the old cry code to parse the source asset file.

            // Sample Code:
            if (index == 0)
            {
                return "font";
            }
            else if (index == 1)
            {
                return "fontfamily";
            }

            return nullptr;
        }
    }
}
