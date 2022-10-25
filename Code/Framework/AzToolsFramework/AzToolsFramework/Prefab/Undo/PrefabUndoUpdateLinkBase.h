/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabDomUtils.h>
#include <Prefab/Undo/PrefabUndoBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class PrefabSystemComponentInterface;

        class PrefabUndoUpdateLinkBase
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoUpdateLinkBase, "{9D2488FA-E0C4-408A-9494-4E0624E95820}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoUpdateLinkBase, AZ::SystemAllocator, 0);

            explicit PrefabUndoUpdateLinkBase(const AZStd::string& undoOperationName);

            void Undo() override;
            void Redo() override;

        protected:
            void SetLink(LinkId linkId);
            void GenerateUndoUpdateLinkPatches(const PrefabDom& linkedInstancePatch);
            void UpdateLink(const PrefabDom& linkDom);

            LinkReference m_link = AZStd::nullopt;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
