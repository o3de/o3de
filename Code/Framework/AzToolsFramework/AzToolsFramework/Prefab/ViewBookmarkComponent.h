/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {

        class ViewBookmarkComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            static constexpr const char* const ViewBookmarkComponentTypeId = "{6959832F-9382-4C7D-83AC-380DA9F138DE}";

            AZ_COMPONENT(ViewBookmarkComponent, ViewBookmarkComponentTypeId, EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            
            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override{};
            void Deactivate() override{};
            //////////////////////////////////////////////////////////////////////////
        protected:

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
            {
                services.push_back(AZ_CRC("EditoViewbookmarkingService"));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
            {
                services.push_back(AZ_CRC("EditoViewbookmarkingService"));
            }

        private:
            // A user editable comment for this entity
            AZStd::string m_comment;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
