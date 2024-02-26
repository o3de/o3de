/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <MiniAudio/SoundAsset.h>

namespace MiniAudio
{
    void SoundAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SoundAsset, AZ::Data::AssetData>()->Version(1)->Field("Data", &SoundAsset::m_data);

            serializeContext->RegisterGenericType<AZ::Data::Asset<SoundAsset>>();

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SoundAsset>("MiniSound SoundAsset", "")->ClassElement(AZ::Edit::ClassElements::EditorData, "");
            }
        }
    }
} // namespace MiniAudio
