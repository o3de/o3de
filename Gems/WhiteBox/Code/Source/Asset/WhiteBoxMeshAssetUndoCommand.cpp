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

#include "WhiteBox_precompiled.h"

#include "Asset/WhiteBoxMeshAssetBus.h"
#include "WhiteBoxMeshAssetUndoCommand.h"

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxMeshAssetUndoCommand, AZ::SystemAllocator, 0);

    WhiteBoxMeshAssetUndoCommand::WhiteBoxMeshAssetUndoCommand()
        : AzToolsFramework::UndoSystem::URSequencePoint("WhiteBoxMeshAssetUndoCommand")
    {
    }

    void WhiteBoxMeshAssetUndoCommand::SetAsset(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> asset)
    {
        m_asset = asset;
    }

    void WhiteBoxMeshAssetUndoCommand::SetUndoState(const AZStd::vector<AZ::u8>& undoState)
    {
        m_undoState = undoState;
    }

    void WhiteBoxMeshAssetUndoCommand::SetRedoState(const AZStd::vector<AZ::u8>& redoState)
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
