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

#include <StdAfx.h>

#include <Asset/BlastSliceAsset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Editor/EditorSystemComponent.h>
#include <Editor/EditorWindow.h>
#include <Editor/MaterialIdWidget.h>

namespace Blast
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        BlastSliceAsset::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void EditorSystemComponent::Activate()
    {
        m_editorBlastSliceAssetHandler = AZStd::make_unique<EditorBlastSliceAssetHandler>();
        m_editorBlastSliceAssetHandler->Register();

        auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
        if (assetCatalog)
        {
            assetCatalog->EnableCatalogForAsset(azrtti_typeid<BlastSliceAsset>());
            assetCatalog->AddExtension("blast_slice");
        }

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            aznew Blast::Editor::MaterialIdWidget());
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        m_editorBlastSliceAssetHandler.reset();
    }

    // This will be called when the IEditor instance is ready
    void EditorSystemComponent::NotifyRegisterViews()
    {
        Blast::Editor::EditorWindow::RegisterViewClass();
    }
} // namespace Blast
