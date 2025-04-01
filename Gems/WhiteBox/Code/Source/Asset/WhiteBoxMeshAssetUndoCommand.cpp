/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Asset/WhiteBoxMeshAssetBus.h"
#include "WhiteBoxMeshAssetUndoCommand.h"

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxMeshAssetUndoCommand, AZ::SystemAllocator);

    WhiteBoxMeshAssetUndoCommand::WhiteBoxMeshAssetUndoCommand()
        : AzToolsFramework::UndoSystem::URSequencePoint("WhiteBoxMeshAssetUndoCommand")
    {
    }

    void WhiteBoxMeshAssetUndoCommand::SetAsset(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> asset)
    {
        m_asset = asset;
    }

    void WhiteBoxMeshAssetUndoCommand::SetUndoState(const Api::WhiteBoxMeshStream& undoState)
    {
        m_undoState = undoState;
    }

    void WhiteBoxMeshAssetUndoCommand::SetRedoState(const Api::WhiteBoxMeshStream& redoState)
    {
        m_redoState = redoState;
    }

    void WhiteBoxMeshAssetUndoCommand::Undo()
    {
        Api::ReadMesh(*m_asset->GetMesh(), m_undoState);
        m_asset->SetWhiteBoxData(m_undoState);
        WhiteBoxMeshAssetNotificationBus::Event(
            m_asset.GetId(), &WhiteBoxMeshAssetNotificationBus::Events::OnWhiteBoxMeshAssetModified, m_asset);
    }

    void WhiteBoxMeshAssetUndoCommand::Redo()
    {
        Api::ReadMesh(*m_asset->GetMesh(), m_redoState);
        m_asset->SetWhiteBoxData(m_redoState);
        WhiteBoxMeshAssetNotificationBus::Event(
            m_asset.GetId(), &WhiteBoxMeshAssetNotificationBus::Events::OnWhiteBoxMeshAssetModified, m_asset);
    }

    bool WhiteBoxMeshAssetUndoCommand::Changed() const
    {
        return m_undoState != m_redoState;
    }
} // namespace WhiteBox
