/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/BlastChunksAsset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Editor/EditorSystemComponent.h>
#include <Editor/EditorWindow.h>
#include <Editor/Material/LegacyBlastMaterialAssetConversion.h>

namespace Blast
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        BlastChunksAsset::Reflect(context);

        ReflectLegacyMaterialClasses(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void EditorSystemComponent::Activate()
    {
        m_editorBlastChunksAssetHandler = AZStd::make_unique<EditorBlastChunksAssetHandler>();
        m_editorBlastChunksAssetHandler->Register();

        auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
        if (assetCatalog)
        {
            assetCatalog->EnableCatalogForAsset(azrtti_typeid<BlastChunksAsset>());
            assetCatalog->AddExtension("blast_chunks");
        }

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        m_editorBlastChunksAssetHandler.reset();
    }

    // This will be called when the IEditor instance is ready
    void EditorSystemComponent::NotifyRegisterViews()
    {
        Blast::Editor::EditorWindow::RegisterViewClass();
    }
} // namespace Blast
