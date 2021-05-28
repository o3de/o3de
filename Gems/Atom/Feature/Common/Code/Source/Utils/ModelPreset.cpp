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

#include <Atom/Feature/Utils/ModelPreset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void ModelPreset::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelPreset>()
                    ->Version(3)
                    ->Field("displayName", &ModelPreset::m_displayName)
                    ->Field("modelAsset", &ModelPreset::m_modelAsset)
                    ->Field("previewImageAsset", &ModelPreset::m_previewImageAsset)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ModelPreset>("ModelPreset")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const ModelPreset&>()
                    ->Property("displayName", BehaviorValueProperty(&ModelPreset::m_displayName))
                    ->Property("modelAsset", BehaviorValueProperty(&ModelPreset::m_modelAsset))
                    ->Property("previewImageAsset", BehaviorValueProperty(&ModelPreset::m_previewImageAsset))
                    ;
            }
        }
    }
}
