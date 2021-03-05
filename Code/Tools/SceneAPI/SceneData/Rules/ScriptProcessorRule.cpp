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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            const AZStd::string& ScriptProcessorRule::GetScriptFilename() const
            {
                return m_scriptFilename;
            }

            void ScriptProcessorRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ScriptProcessorRule, DataTypes::IScriptProcessorRule>()->Version(1)
                        ->Field("scriptFilename", &ScriptProcessorRule::m_scriptFilename);

                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<ScriptProcessorRule>("ScriptProcessorRule", "Script rule settings to process a scene asset file")
                            ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->DataElement(0, &ScriptProcessorRule::m_scriptFilename, "scriptFilename",
                                "Relative path to scene processor Python script.");
                    }
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
