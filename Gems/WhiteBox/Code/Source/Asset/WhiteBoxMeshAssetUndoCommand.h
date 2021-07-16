/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxMeshAsset.h"

#include <AzToolsFramework/Undo/UndoSystem.h>

namespace WhiteBox
{
    //! Records undo/redo states when modifying an asset.
    class WhiteBoxMeshAssetUndoCommand : public AzToolsFramework::UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ_RTTI(
            WhiteBoxMeshAssetUndoCommand, "{C99CD86C-035A-4FC9-AADC-4C746C38F119}",
            AzToolsFramework::UndoSystem::URSequencePoint)

        explicit WhiteBoxMeshAssetUndoCommand();
        WhiteBoxMeshAssetUndoCommand(const WhiteBoxMeshAssetUndoCommand& other) = delete;
        WhiteBoxMeshAssetUndoCommand& operator=(const WhiteBoxMeshAssetUndoCommand& other) = delete;
        ~WhiteBoxMeshAssetUndoCommand() override = default;

        void SetAsset(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> asset);
        void SetUndoState(const Api::WhiteBoxMeshStream& undoState);
        void SetRedoState(const Api::WhiteBoxMeshStream& redoState);

        // AzToolsFramework::UndoSystem::URSequencePoint ...
        void Undo() override;
        void Redo() override;
        bool Changed() const override;

    protected:
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> m_asset;
        Api::WhiteBoxMeshStream m_undoState;
        Api::WhiteBoxMeshStream m_redoState;
    };
} // namespace WhiteBox
