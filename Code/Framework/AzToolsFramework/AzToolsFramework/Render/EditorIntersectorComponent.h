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

#include <AzCore/Component/Component.h>
#include <AzFramework/Render/IntersectorInterface.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! System component for calculating render geometry intersections against entities.
        //! Contains an implementation of AzFramework::IntersectorInterface.
        class EditorIntersectorComponent
            : public AZ::Component
        {
        public:

            AZ_COMPONENT(EditorIntersectorComponent, "{8496804C-2CC8-4ABF-B9B1-60845E37FB7A}");

            //////////////////////////////////////////////////////////////////////////
            // Component overrides
            void Activate() override;
            void Deactivate() override;

            static void Reflect(AZ::ReflectContext* context);

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC("EditorEntityContextService", 0x28d93a43));
            }

        private:
            AZStd::unique_ptr<AzFramework::RenderGeometry::IntersectorInterface> m_intersector;
        };
    } // namespace Components
} // namespace AzToolsFramework
