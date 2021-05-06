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
