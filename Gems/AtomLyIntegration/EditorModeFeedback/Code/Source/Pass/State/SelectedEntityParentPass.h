/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateParentPassBase.h>

namespace AZ::Render
{
    class SelectedEntityParentPass
        : public EditorStateParentPassBase
        //, private EditorModeSelectedEntityStateNotificationBus::Handler
        //, private EditorModeSelectedEntityStateRequestBus::Handler
    {
    public:
        SelectedEntityParentPass();

        // EditorModeSelectedEntityStateRequestBus overrides ...
        //void SetLineWidth(float intensity);
        //void SetLineColor(const AZ::Color& tint);

        // EditorModeStateParentPass overrides ...
        void InitPassData(RPI::ParentPass* parentPass) override;

        // EditorStateParentPassBase overrides ...
        AzToolsFramework::EntityIdList GetMaskedEntities() const override;
    private:

    };
} // namespace AZ::Render
