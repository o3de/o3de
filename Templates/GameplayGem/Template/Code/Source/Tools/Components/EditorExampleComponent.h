// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Components/${SanitizedCppName}ComponentController.h>

namespace ${SanitizedCppName}
{
    class Editor${SanitizedCppName}Component
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(Editor${SanitizedCppName}Component, Editor${SanitizedCppName}ComponentTypeId, AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        Editor${SanitizedCppName}Component() = default;
        ~Editor${SanitizedCppName}Component() override = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        ${SanitizedCppName}ComponentController m_controller;
    };
} // namespace ${SanitizedCppName}
