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

#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        const char* PassAsset::DisplayName = "Pass";
        const char* PassAsset::Group = "RenderingPipeline";
        const char* PassAsset::Extension = "pass";

        void PassAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PassAsset, Data::AssetData>()
                    ->Version(1)
                    ->Field("PassTemplate", &PassAsset::m_passTemplate)
                    ;
            }
        }

        const AZStd::unique_ptr<PassTemplate>& PassAsset::GetPassTemplate() const
        {
            return m_passTemplate;
        }

        void PassAsset::SetPassTemplateForTestingOnly(const PassTemplate& passTemplate)
        {
            m_passTemplate = passTemplate.CloneUnique();
        }
    } // namespace RPI
} // namespace AZ
