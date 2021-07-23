/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorCommonFeaturesSystemComponent.h>
#include <SkinnedMesh/SkinnedMeshDebugDisplay.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <IEditor.h>

namespace AZ
{
    namespace Render
    {
        static IEditor* GetLegacyEditor()
        {
            IEditor* editor = nullptr;
            AzToolsFramework::EditorRequestBus::BroadcastResult(editor, &AzToolsFramework::EditorRequestBus::Events::GetEditor);
            return editor;
        }

        EditorCommonFeaturesSystemComponent::EditorCommonFeaturesSystemComponent() = default;

        EditorCommonFeaturesSystemComponent::~EditorCommonFeaturesSystemComponent() = default;

        //! Main system component for the Atom Common Feature Gem's editor/tools module.
        void EditorCommonFeaturesSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorCommonFeaturesSystemComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Atom Level Default Asset Path", &EditorCommonFeaturesSystemComponent::m_atomLevelDefaultAssetPath);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorCommonFeaturesSystemComponent>("AtomEditorCommonFeaturesSystemComponent",
                        "Configures editor- and tool-specific functionality for common render features.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(nullptr, &EditorCommonFeaturesSystemComponent::m_atomLevelDefaultAssetPath, "Atom Level Default Asset Path",
                            "path to the slice the instantiate for a new Atom level")
                        ;
                }
            }
        }

        void EditorCommonFeaturesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EditorCommonFeaturesService", 0x94945c0c));
        }

        void EditorCommonFeaturesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EditorCommonFeaturesService", 0x94945c0c));
        }

        void EditorCommonFeaturesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        void EditorCommonFeaturesSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void EditorCommonFeaturesSystemComponent::Init()
        {
        }

        void EditorCommonFeaturesSystemComponent::Activate()
        {
            m_renderer = AZStd::make_unique<AZ::LyIntegration::Thumbnails::CommonThumbnailRenderer>();
            m_previewerFactory = AZStd::make_unique <LyIntegration::CommonPreviewerFactory>();
            m_skinnedMeshDebugDisplay = AZStd::make_unique<SkinnedMeshDebugDisplay>();

            AzToolsFramework::EditorLevelNotificationBus::Handler::BusConnect();
            AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusConnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        }

        void EditorCommonFeaturesSystemComponent::Deactivate()
        {
            AzToolsFramework::EditorLevelNotificationBus::Handler::BusDisconnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
            AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusDisconnect();

            m_skinnedMeshDebugDisplay.reset();
            m_previewerFactory.reset();
            m_renderer.reset();
        }

        void EditorCommonFeaturesSystemComponent::OnNewLevelCreated()
        {
            bool isPrefabSystemEnabled = false;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(
                isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

            if (!isPrefabSystemEnabled)
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    m_levelDefaultSliceAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, m_atomLevelDefaultAssetPath.c_str(),
                    azrtti_typeid<AZ::SliceAsset>(), false);

                if (m_levelDefaultSliceAssetId.IsValid())
                {
                    AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(
                        m_levelDefaultSliceAssetId, AZ::Data::AssetLoadBehavior::Default);

                    asset.BlockUntilLoadComplete();

                    if (asset)
                    {
                        AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
                        bool activeCameraFound = false;
                        Camera::EditorCameraRequestBus::BroadcastResult(
                            activeCameraFound, &Camera::EditorCameraRequestBus::Events::GetActiveCameraPosition, cameraPosition);

                        if (activeCameraFound)
                        {
                            AZ::Transform worldTransform = AZ::Transform::CreateTranslation(cameraPosition);

                            AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusConnect();

                            if (IEditor* editor = GetLegacyEditor(); !editor->IsUndoSuspended())
                            {
                                editor->SuspendUndo();
                            }

                            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                                &AzToolsFramework::SliceEditorEntityOwnershipServiceRequests::InstantiateEditorSlice, asset,
                                worldTransform);
                        }
                    }
                }
            }
        }

        void EditorCommonFeaturesSystemComponent::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& /*ticket*/)
        {
            if (m_levelDefaultSliceAssetId == sliceAssetId)
            {
                const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

                AZStd::vector<AZ::EntityId> entityIds;
                entityIds.reserve(entities.size());
                for (const Entity* entity : entities)
                {
                    entityIds.push_back(entity->GetId());
                }

                //Detach instantiated env probe entities from engine slice
                AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                    &AzToolsFramework::SliceEditorEntityOwnershipServiceRequests::DetachSliceEntities, entityIds);
                sliceAddress.SetInstance(nullptr);
                sliceAddress.SetReference(nullptr);

                AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();

                IEditor* editor = GetLegacyEditor();

                //save after level default slice fully instantiated
                editor->SaveDocument();

                if (editor->IsUndoSuspended())
                {
                    editor->ResumeUndo();
                }
            }
        }

        void EditorCommonFeaturesSystemComponent::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& /*ticket*/)
        {
            if (m_levelDefaultSliceAssetId == sliceAssetId)
            {
                GetLegacyEditor()->ResumeUndo();
                AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
                AZ_Warning("EditorCommonFeaturesSystemComponent", false, "Failed to instantiate default Atom environment slice.");
            }
        }

        const AzToolsFramework::AssetBrowser::PreviewerFactory* EditorCommonFeaturesSystemComponent::GetPreviewerFactory(
            const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
        {
            return m_previewerFactory->IsEntrySupported(entry) ? m_previewerFactory.get() : nullptr;
        }

        void EditorCommonFeaturesSystemComponent::OnApplicationAboutToStop()
        {
            m_renderer.reset();
        }
    } // namespace Render
} // namespace AZ
