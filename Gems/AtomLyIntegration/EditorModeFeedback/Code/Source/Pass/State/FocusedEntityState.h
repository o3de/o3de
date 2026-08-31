/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateBase.h>

#include <AzCore/std/functional.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AZ::Render
{
    //! Class for the Focused Entity editor state effect.
    class FocusedEntityState
        : public EditorStateBase
    {
    public:
        explicit FocusedEntityState(AZStd::function<AzFramework::EntityContextId()> worldIdProvider);

        // EditorStateBase overrides ...
        bool IsEnabled() const override;
        AzToolsFramework::EntityIdList GetMaskedEntities() const override;

    private:
        AZStd::function<AzFramework::EntityContextId()> m_worldIdProvider;
    };
} // namespace AZ::Render
