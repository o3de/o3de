/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabEditInterface;

        class PrefabUndoEdit : public UndoSystem::URSequencePoint
        {
        public:
            explicit PrefabUndoEdit(const AZStd::string& undoOperationName);

            bool Changed() const override
            {
                return m_changed;
            }
            void Capture(AZ::EntityId oldContainerEntityId, AZ::EntityId newContainerEntityId);

            void Undo();
            void Redo();

        protected:
            AZ::EntityId m_oldContainerEntityId;
            AZ::EntityId m_newContainerEntityId;

            PrefabEditInterface* m_prefabEditInterface;
            bool m_changed;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
