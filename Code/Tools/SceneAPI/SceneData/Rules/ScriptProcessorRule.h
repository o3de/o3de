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
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IScriptProcessorRule.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class ScriptProcessorRule
                : public DataTypes::IScriptProcessorRule
            {
            public:
                AZ_RTTI(ScriptProcessorRule, "{E61EDCBC-867A-4A6A-B49D-C87E60D3EC33}", DataTypes::IScriptProcessorRule);
                AZ_CLASS_ALLOCATOR(ScriptProcessorRule, AZ::SystemAllocator, 0)

                ~ScriptProcessorRule() override = default;

                const AZStd::string& GetScriptFilename() const override;

                inline void SetScriptFilename(AZStd::string scriptFilename)
                {
                    m_scriptFilename = AZStd::move(scriptFilename);
                }

                static void Reflect(ReflectContext* context);

            protected:
                AZStd::string m_scriptFilename;
            };
        } // SceneData
    } // SceneAPI
} // AZ
