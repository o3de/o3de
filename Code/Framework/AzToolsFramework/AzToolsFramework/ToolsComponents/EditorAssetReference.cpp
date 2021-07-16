/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorAssetReference.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AzToolsFramework
{
    void AssetReferenceBase::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AssetReferenceBase>()
                ->Version(1)
                ->Field("CurrentAssetID", &AssetReferenceBase::m_currentID);
        }
    }
}
