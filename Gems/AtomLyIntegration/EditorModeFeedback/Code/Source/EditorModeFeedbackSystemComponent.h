/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Draw/DrawableMeshEntity.h>

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <EditorModeFeedback/EditorModeFeedbackInterface.h>

namespace AZ
{
    namespace RPI
    {
        class Material;
    }

    namespace Render
    {
        //! Component for the editor mode feedback system.
        class EditorModeFeedbackSystemComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorModeFeedbackInterface            
            , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorModeFeedbackSystemComponent, "{A88EE29D-4C72-4995-B3BD-41EEDE480487}");

            static void Reflect(AZ::ReflectContext* context);

            // EditorComponentBase overrides ...
            void Activate() override;
            void Deactivate() override;

            // EditorModeFeedbackInterface overrides ...
            bool IsEnabled() const override;

        private:
            // EditorEntityContextNotificationBus overrides ...
            void OnStartPlayInEditorBegin() override;
            void OnStopPlayInEditor() override;

            // Enable/disable editor mode feedback rendering for the level viewport (main scene)
            void SetEnableRender(bool enableRender);

            //! Settings registry override for enabling/disabling editor mode feedback.
            bool m_registeryEnabled = false;
        };
    } // namespace Render
} // namespace AZ
