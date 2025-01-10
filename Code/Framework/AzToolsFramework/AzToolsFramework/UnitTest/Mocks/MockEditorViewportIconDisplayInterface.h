/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/EditorViewportIconDisplayInterface.h>

#include <gmock/gmock.h>

namespace UnitTest
{
    class MockEditorViewportIconDisplayInterface : public AZ::Interface<AzToolsFramework::EditorViewportIconDisplayInterface>::Registrar
    {
    public:
        virtual ~MockEditorViewportIconDisplayInterface() = default;

        //! AzToolsFramework::EditorViewportIconDisplayInterface overrides ...
        MOCK_METHOD1(DrawIcon, void(const DrawParameters&));
        MOCK_METHOD1(AddIcon, void(const DrawParameters&));
        MOCK_METHOD1(GetOrLoadIconForPath, IconId(AZStd::string_view path));
        MOCK_METHOD1(GetIconLoadStatus, IconLoadStatus(IconId icon));
        MOCK_METHOD0(DrawIcons, void());
    };
} // namespace UnitTest
