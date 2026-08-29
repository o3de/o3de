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

#include <Clients/${SanitizedCppName}SystemComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}EditorSystemComponent
        : public ${SanitizedCppName}SystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseClass = ${SanitizedCppName}SystemComponent;
    public:
        AZ_COMPONENT_DECL(${SanitizedCppName}EditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        ${SanitizedCppName}EditorSystemComponent() = default;
        ~${SanitizedCppName}EditorSystemComponent() override = default;

    private:
        void Activate() override;
        void Deactivate() override;
    };
} // namespace ${SanitizedCppName}
