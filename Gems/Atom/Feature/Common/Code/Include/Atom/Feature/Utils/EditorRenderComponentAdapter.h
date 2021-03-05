/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>

namespace AZ
{
    namespace Render
    {
        //! Base editor component adapter adding automatic editor visibility support
        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        class EditorRenderComponentAdapter
            : public AzToolsFramework::Components::EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>
            , public AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration>;
            AZ_RTTI((EditorRenderComponentAdapter, "{AAF38BE4-EA2F-408B-9C44-63C7FBAC6B33}", TController, TRuntimeComponent, TConfiguration), BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorRenderComponentAdapter() = default;
            EditorRenderComponentAdapter(const TConfiguration& config);

            // AzToolsFramework::Components::EditorComponentAdapter overrides
            void Activate() override;
            void Deactivate() override;

            bool IsVisible() const;

        protected:
            // AzToolsFramework::Components::EditorComponentAdapter overrides
            bool ShouldActivateController() const override;

            // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides
            void OnEntityVisibilityChanged(bool visibility) override;

            // Convert pre-existing EditorCompnentAdapter based serialized data to EditorRenderComponentAdapter
            template<int TVersion>
            static bool ConvertToEditorRenderComponentAdapter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        };
    } // namespace Render
} // namespace AZ

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.inl>
