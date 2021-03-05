/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
