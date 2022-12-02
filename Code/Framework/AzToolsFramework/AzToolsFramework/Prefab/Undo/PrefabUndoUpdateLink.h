/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class PrefabSystemComponentInterface;

        class PrefabUndoUpdateLink
            : public PrefabUndoBase
        {
        public:
            AZ_RTTI(PrefabUndoUpdateLink, "{9D2488FA-E0C4-408A-9494-4E0624E95820}", PrefabUndoBase);
            AZ_CLASS_ALLOCATOR(PrefabUndoUpdateLink, AZ::SystemAllocator, 0);

            explicit PrefabUndoUpdateLink(const AZStd::string& undoOperationName);

            void Undo() override;
            void Redo() override;
            void Redo(InstanceOptionalConstReference instanceToExclude) override;

            void Capture(const PrefabDom& linkedInstancePatch, LinkId linkId);

        protected:
            void SetLink(LinkId linkId);
            void UpdateLink(const PrefabDom& linkDom,
                InstanceOptionalConstReference instanceToExclude = AZStd::nullopt);

            LinkReference m_link = AZStd::nullopt;
        };
    }
}
