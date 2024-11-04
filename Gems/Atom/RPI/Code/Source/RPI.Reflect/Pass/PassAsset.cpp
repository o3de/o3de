/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
