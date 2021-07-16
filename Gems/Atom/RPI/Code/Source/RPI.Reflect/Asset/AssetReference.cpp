/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Reflect/Asset/AssetReference.h>

namespace AZ
{
    namespace RPI
    {
        void AssetReference::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AssetReference>()
                    ->Version(0)
                    ->Field("FilePath", &AssetReference::m_filePath)
                    ->Field("AssetId", &AssetReference::m_assetId)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
