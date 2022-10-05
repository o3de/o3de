/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabUndoBase
            : public UndoSystem::URSequencePoint
        {
        public:
            explicit PrefabUndoBase(const AZStd::string& undoOperationName);

            bool Changed() const override { return m_changed; }

        protected:
            TemplateId m_templateId;

            PrefabDom m_redoPatch;
            PrefabDom m_undoPatch;

            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;

            bool m_changed;
        };
    }
}
