/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral_precompiled.h>
#include "CopyDependencyBuilderComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace CopyDependencyBuilder
{
    void CopyDependencyBuilderComponent::Activate()
    {
        m_fontBuilderWorker.RegisterBuilderWorker();
        m_cfgBuilderWorker.RegisterBuilderWorker();
        m_schemaBuilderWorker.RegisterBuilderWorker();
        m_xmlBuilderWorker.RegisterBuilderWorker();
        m_emfxWorkspaceBuilderWorker.RegisterBuilderWorker();
    }

    void CopyDependencyBuilderComponent::Deactivate()
    {
        m_emfxWorkspaceBuilderWorker.UnregisterBuilderWorker();
        m_xmlBuilderWorker.UnregisterBuilderWorker();
        m_schemaBuilderWorker.UnregisterBuilderWorker();
        m_cfgBuilderWorker.UnregisterBuilderWorker();
        m_fontBuilderWorker.UnregisterBuilderWorker();
    }

    void CopyDependencyBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CopyDependencyBuilderComponent, AZ::Component>()
                ->Version(3)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }
}
