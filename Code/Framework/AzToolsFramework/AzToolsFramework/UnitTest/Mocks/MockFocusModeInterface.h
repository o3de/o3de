/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/FocusMode/FocusModeInterface.h>

#include <gmock/gmock.h>

namespace UnitTest
{
    class MockFocusModeInterface : public AZ::Interface<AzToolsFramework::FocusModeInterface>::Registrar
    {
    public:
        virtual ~MockFocusModeInterface() = default;

        // AzToolsFramework::FocusModeInterface overrides ...
        MOCK_METHOD1(SetFocusRoot, void(AZ::EntityId entityId));
        MOCK_METHOD1(ClearFocusRoot, void(AzFramework::EntityContextId entityContextId));
        MOCK_METHOD1(GetFocusRoot, AZ::EntityId(AzFramework::EntityContextId entityContextId));
        MOCK_METHOD1(GetFocusedEntities, const AzToolsFramework::EntityIdList&(AzFramework::EntityContextId entityContextId));
        MOCK_CONST_METHOD1(IsInFocusSubTree, bool(AZ::EntityId entityId));
        MOCK_CONST_METHOD1(IsFocusRoot, bool(AZ::EntityId entityId));
    };
} // namespace UnitTest
