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

#include <EditorCommonFeaturesSystemComponent.h>
#include <SkinnedMesh/SkinnedMeshDebugDisplay.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzToolsFramework/API/EditorCameraBus.h>

#include <IEditor.h>

namespace AZ
{
    namespace Render
    {
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
            m_skinnedMeshDebugDisplay = AZStd::make_unique<SkinnedMeshDebugDisplay>();

            AzToolsFramework::EditorLevelNotificationBus::Handler::BusConnect();
        }

        void EditorCommonFeaturesSystemComponent::Deactivate()
        {
            AzToolsFramework::EditorLevelNotificationBus::Handler::BusDisconnect();
            m_skinnedMeshDebugDisplay.reset();
        }

        void EditorCommonFeaturesSystemComponent::OnNewLevelCreated()
        {
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(m_levelDefaultSliceAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, m_atomLevelDefaultAssetPath.c_str(), azrtti_typeid<AZ::SliceAsset>(), false);

            if (m_levelDefaultSliceAssetId.IsValid())
            {
                AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().FindAsset<AZ::SliceAsset>(
                    m_levelDefaultSliceAssetId,
                    AZ::Data::AssetLoadBehavior::Default);

                if (asset)
                {
                    AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
                    bool activeCameraFound = false;
                    Camera::EditorCameraRequestBus::BroadcastResult(activeCameraFound, &Camera::EditorCameraRequestBus::Events::GetActiveCameraPosition, cameraPosition);

                    if (activeCameraFound)
                    {
                        AZ::Transform worldTransform = AZ::Transform::CreateTranslation(cameraPosition);

                        AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusConnect();

                        if (!GetIEditor()->IsUndoSuspended())
                        {
                            GetIEditor()->SuspendUndo();
                        }

                        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequests::InstantiateEditorSlice, asset, worldTransform);
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

                //save after level default slice fully instantiated
                GetIEditor()->SaveDocument();

                if (GetIEditor()->IsUndoSuspended())
                {
                    GetIEditor()->ResumeUndo();
                }
            }
        }


        void EditorCommonFeaturesSystemComponent::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& /*ticket*/)
        {
            if (m_levelDefaultSliceAssetId == sliceAssetId)
            {
                GetIEditor()->ResumeUndo();
                AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
                AZ_Warning("EditorCommonFeaturesSystemComponent", false, "Failed to instantiate default Atom environment slice.");
            }
        }
    } // namespace Render
} // namespace AZ
