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

#undef RC_INVOKED

#include <Atom/Feature/Utils/EditorModelPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorModelPreset::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ModelPreset>(
                        "ModelPreset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ModelPreset::m_displayName, "Display Name", "Identifier used for display and selection")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ModelPreset::m_autoSelect, "Auto Select", "When true, the configuration is automatically selected when loaded")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ModelPreset::m_modelAsset, "Model Asset", "Model asset reference")
                        ;
                }
            }
        }
    }
}
