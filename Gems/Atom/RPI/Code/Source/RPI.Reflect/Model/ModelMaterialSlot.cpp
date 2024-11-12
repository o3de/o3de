/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/ModelMaterialSlot.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ModelMaterialSlot::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelMaterialSlot>()
                    ->Version(0)
                    ->Field("StableId", &ModelMaterialSlot::m_stableId)
                    ->Field("DisplayName", &ModelMaterialSlot::m_displayName)
                    ->Field("DefaultMaterialAsset", &ModelMaterialSlot::m_defaultMaterialAsset)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
