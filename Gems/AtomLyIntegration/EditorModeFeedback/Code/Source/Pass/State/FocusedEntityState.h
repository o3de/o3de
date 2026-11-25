/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateBase.h>


namespace AZ::Render
{
    //! Class for the Focused Entity editor state effect.
    class FocusedEntityState
        : public EditorStateBase
    {
    public:
        FocusedEntityState();

        bool IsEnabled(const RPI::RenderPipeline* pipeline) const override;
        AzToolsFramework::EntityIdList GetMaskedEntities(const RPI::RenderPipeline* pipeline) const override;
        void UpdatePassDataForPipelines() override;

    private:
        AZStd::unordered_map<const RPI::RenderPipeline*, AZ::EntityId> m_focusRoots;
    };
} // namespace AZ::Render
