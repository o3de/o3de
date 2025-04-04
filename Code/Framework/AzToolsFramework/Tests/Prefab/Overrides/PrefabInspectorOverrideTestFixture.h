/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <Prefab/Overrides/PrefabOverrideTestFixture.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;

    class PrefabInspectorOverrideTestFixture : public PrefabOverrideTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        void GenerateComponentAdapterDoms(AZ::EntityId);
        void ValidateComponentEditorDomContents(
            const AzToolsFramework::ComponentEditor::VisitComponentAdapterContentsCallback& callback);

        AZStd::unique_ptr<AzToolsFramework::EntityPropertyEditor> m_testEntityPropertyEditor;
        bool m_ed_enableDPEInspector = false;
        bool m_ed_enableInspectorOverrideManagement = false;
    };
} // namespace UnitTest
