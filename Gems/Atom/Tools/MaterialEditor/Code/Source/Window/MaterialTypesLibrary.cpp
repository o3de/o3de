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

#include <Source/Window/MaterialTypesLibrary.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace MaterialEditor
{
    void MaterialTypeEntry::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<AZStd::vector<MaterialTypeEntry>>();

            serializeContext->Class<MaterialTypeEntry>()
                ->Version(1)
                ->Field("displayName", &MaterialTypeEntry::m_displayName)
                ->Field("filePath", &MaterialTypeEntry::m_filePath)
                ;
        }
    }

    void MaterialTypesLibrary::Reflect(AZ::ReflectContext* context)
    {
        MaterialTypeEntry::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialTypesLibrary>()
                ->Version(1)
                ->Field("entries", &MaterialTypesLibrary::m_entries)
                ;
        }
    }
}
