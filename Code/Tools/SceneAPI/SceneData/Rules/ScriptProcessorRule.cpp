/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
