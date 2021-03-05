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
        void SetUndoState(const AZStd::vector<AZ::u8>& undoState);
        void SetRedoState(const AZStd::vector<AZ::u8>& redoState);

        // AzToolsFramework::UndoSystem::URSequencePoint ...
        void Undo() override;
        void Redo() override;
        bool Changed() const override;

    protected:
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> m_asset;
        AZStd::vector<AZ::u8> m_undoState;
        AZStd::vector<AZ::u8> m_redoState;
    };
} // namespace WhiteBox
