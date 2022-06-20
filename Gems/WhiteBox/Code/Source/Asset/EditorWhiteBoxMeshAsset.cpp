/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Asset/WhiteBoxMeshAssetHandler.h"
#include "Asset/WhiteBoxMeshAssetUndoCommand.h"
#include "EditorWhiteBoxMeshAsset.h"
#include "Util/WhiteBoxEditorUtil.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorWhiteBoxMeshAsset, AZ::SystemAllocator, 0)

    static constexpr const char* const AssetModifiedUndoRedoDesc = "White Box Mesh asset was updated";

    static bool MeshAssetValid(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> meshAsset)
    {
        return meshAsset.GetId().IsValid();
    }

    static bool MeshAssetLoaded(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> meshAsset)
    {
        return MeshAssetValid(meshAsset) && meshAsset.IsReady();
    }

    static AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> CreateOrFindMeshAsset(
        const AZStd::string& assetPath, AZ::Data::AssetLoadBehavior loadBehavior)
    {
        AZ::Data::AssetId generatedAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            generatedAssetId, &AZ::Data::AssetCatalogRequests::GenerateAssetIdTEMP, assetPath.c_str());

        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> meshAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(
            generatedAssetId, azrtti_typeid<Pipeline::WhiteBoxMeshAsset>(), loadBehavior);

        return meshAsset;
    }

    static AZStd::optional<AZStd::string> AbsolutePathForSourceAsset(
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>& asset)
    {
        AZStd::string relativeAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            relativeAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, asset.GetId());

        AZStd::string absoluteAssetPath;
        bool foundAbsolutePath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            foundAbsolutePath,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath,
            relativeAssetPath, absoluteAssetPath);

        if (foundAbsolutePath)
        {
            return absoluteAssetPath;
        }

        return AZStd::nullopt;
    }

    static bool SaveAsset(
        const AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>& meshAsset, const AZStd::string& absoluteFilePath)
    {
        bool success = false;
        const auto assetType = AZ::AzTypeInfo<Pipeline::WhiteBoxMeshAsset>::Uuid();
        if (auto assetHandler = AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            if (AZ::IO::FileIOStream fileStream(absoluteFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
                fileStream.IsOpen())
            {
                success = assetHandler->SaveAssetData(meshAsset, &fileStream);
                AZ_Printf(
                    "EditorWhiteBoxMeshAsset", "Save %s. Location: %s", success ? "succeeded" : "failed",
                    absoluteFilePath.c_str());
            }
        }

        return success;
    }

    void EditorWhiteBoxMeshAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWhiteBoxMeshAsset>()->Version(1)->Field(
                "MeshAsset", &EditorWhiteBoxMeshAsset::m_meshAsset);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorWhiteBoxMeshAsset>("Editor White Box Mesh Asset", "White Box Mesh Asset")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxMeshAsset::m_meshAsset, "Mesh Asset",
                        "Mesh Asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxMeshAsset::AssetChanged)
                    ->Attribute(AZ::Edit::Attributes::ClearNotify, &EditorWhiteBoxMeshAsset::AssetCleared);
            }
        }
    }

    EditorWhiteBoxMeshAsset::~EditorWhiteBoxMeshAsset()
    {
        if (InUse())
        {
            Reset();
        }
    }

    bool EditorWhiteBoxMeshAsset::InUse() const
    {
        return MeshAssetValid(m_meshAsset);
    }

    bool EditorWhiteBoxMeshAsset::Loaded() const
    {
        // it is possible that we've switched to use an asset but it isn't
        // ready yet, in this case return that the asset isn't yet loaded
        return MeshAssetLoaded(m_meshAsset);
    }

    void EditorWhiteBoxMeshAsset::Serialize()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch(AssetModifiedUndoRedoDesc);

        // create undo command to record changes to the asset
        auto* command = aznew WhiteBoxMeshAssetUndoCommand();
        command->SetAsset(m_meshAsset);
        command->SetUndoState(m_meshAsset->GetWhiteBoxData());

        m_meshAsset->Serialize();

        command->SetRedoState(m_meshAsset->GetWhiteBoxData());
        command->SetParent(undoBatch.GetUndoBatch());
    }

    void EditorWhiteBoxMeshAsset::Load()
    {
        if (!InUse())
        {
            return;
        }

        Disconnect();

        if (m_meshAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error ||
            m_meshAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded)
        {
            m_meshAsset.QueueLoad();
        }

        RegisterForEditorEvents();
        AZ::Data::AssetBus::Handler::BusConnect(m_meshAsset.GetId());
        WhiteBoxMeshAssetNotificationBus::Handler::BusConnect(m_meshAsset.GetId());
    }

    void EditorWhiteBoxMeshAsset::Associate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;
    }

    void EditorWhiteBoxMeshAsset::Disconnect()
    {
        UnregisterForEditorEvents();

        // disconnect from any previously connected asset id
        AZ::Data::AssetBus::Handler::BusDisconnect();
        WhiteBoxMeshAssetNotificationBus::Handler::BusDisconnect();

        // ensure we're disconnected from the tick bus
        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorWhiteBoxMeshAsset::Release()
    {
        Disconnect();
        m_meshAsset.Release();
    }

    void EditorWhiteBoxMeshAsset::Reset()
    {
        Disconnect();
        m_meshAsset = AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>{};
    }

    WhiteBoxMesh* EditorWhiteBoxMeshAsset::GetWhiteBoxMesh()
    {
        return Loaded() ? m_meshAsset->GetMesh() : nullptr;
    }

    AZ::Data::AssetId EditorWhiteBoxMeshAsset::GetWhiteBoxMeshAssetId() const
    {
        return m_meshAsset.GetId();
    }

    AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> EditorWhiteBoxMeshAsset::GetWhiteBoxMeshAsset()
    {
        return m_meshAsset;
    }

    void EditorWhiteBoxMeshAsset::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            m_meshAsset = asset;

            // defer rebuilding the mesh by a frame by connecting to the tick bus - this prevents issues
            // with reentrancy when rebuilding the white box mesh
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void EditorWhiteBoxMeshAsset::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // after rebuilding the white box mesh, immediately disconnect from the tick bus (we only use it for deferred rebuilding)
        EditorWhiteBoxComponentRequestBus::Event(m_entityComponentIdPair, &EditorWhiteBoxComponentRequestBus::Events::RebuildWhiteBox);
        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorWhiteBoxMeshAsset::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void EditorWhiteBoxMeshAsset::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            AZ_Warning("EditorWhiteBoxMeshAsset", false, "OnAssetError: %s", asset.GetHint().c_str());
        }
    }

    void EditorWhiteBoxMeshAsset::OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            AZ_Warning("EditorWhiteBoxMeshAsset", false, "OnAssetReloadError: %s", asset.GetHint().c_str());
        }
    }

    void EditorWhiteBoxMeshAsset::OnWhiteBoxMeshAssetModified(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            EditorWhiteBoxComponentRequestBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxComponentRequestBus::Events::RebuildWhiteBox);
        }
    }

    void EditorWhiteBoxMeshAsset::RegisterForEditorEvents()
    {
        if (auto* editor = GetIEditor())
        {
            editor->RegisterNotifyListener(this);
        }
    }

    void EditorWhiteBoxMeshAsset::UnregisterForEditorEvents()
    {
        if (auto* editor = GetIEditor())
        {
            editor->UnregisterNotifyListener(this);
        }
    }

    void EditorWhiteBoxMeshAsset::OnEditorNotifyEvent(const EEditorNotifyEvent editorEvent)
    {
        switch (editorEvent)
        {
        case eNotify_OnEndSceneSave:
            if (InUse())
            {
                Save();
            }
            break;
        default:
            break;
        }
    }

    void EditorWhiteBoxMeshAsset::TakeOwnershipOfWhiteBoxMesh(
        const AZStd::string& relativeAssetPath, Api::WhiteBoxMeshPtr whiteBoxMesh)
    {
        m_meshAsset = CreateOrFindMeshAsset(relativeAssetPath, m_meshAsset.GetAutoLoadBehavior());
        m_meshAsset->SetMesh(AZStd::move(whiteBoxMesh));

        // make sure the new asset has an up to date serialized state (for use in undo/redo)
        m_meshAsset->Serialize();

        AZ::Data::AssetBus::Handler::BusDisconnect();
        WhiteBoxMeshAssetNotificationBus::Handler::BusDisconnect();

        AZ::Data::AssetBus::Handler::BusConnect(m_meshAsset.GetId());
        WhiteBoxMeshAssetNotificationBus::Handler::BusConnect(m_meshAsset.GetId());
    }

    void EditorWhiteBoxMeshAsset::Save()
    {
        if (const auto absolutePath = AbsolutePathForSourceAsset(m_meshAsset))
        {
            Save(absolutePath.value());
        }
    }

    void EditorWhiteBoxMeshAsset::Save(const AZStd::string& absolutePath)
    {
        // save the asset to disk in the project folder
        if (WhiteBox::SaveAsset(m_meshAsset, absolutePath))
        {
            RequestEditSourceControl(absolutePath.c_str());
        }
    }

    void EditorWhiteBoxMeshAsset::AssetChanged()
    {
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequestBus::Events::DeserializeWhiteBox);
    }

    void EditorWhiteBoxMeshAsset::AssetCleared()
    {
        // when hitting 'clear' on the asset widget, the asset data is written locally to the component
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequestBus::Events::WriteAssetToComponent);
    }
} // namespace WhiteBox
