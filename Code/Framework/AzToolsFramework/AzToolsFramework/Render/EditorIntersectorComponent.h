/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                dependent.push_back(AZ_CRC_CE("EditorEntityContextService"));
            }

        private:
            AZStd::unique_ptr<AzFramework::RenderGeometry::IntersectorInterface> m_intersector;
        };
    } // namespace Components
} // namespace AzToolsFramework
