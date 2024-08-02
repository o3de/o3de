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

namespace AzFramework
{
    namespace RenderGeometry
    {
        //! System component for calculating render geometry intersections against game entities.
        //! Contains an implementation of AzFramework::IntersectorInterface.
        class GameIntersectorComponent
            : public AZ::Component
        {
        public:

            AZ_COMPONENT(GameIntersectorComponent, "{F32B0E3E-23D6-48EA-BC33-A6DE2F4495C6}");

            //////////////////////////////////////////////////////////////////////////
            // Component overrides
            void Activate() override;
            void Deactivate() override;

            static void Reflect(AZ::ReflectContext* context);

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC_CE("GameEntityContextService"));
            }

        private:
            AZStd::unique_ptr<AzFramework::RenderGeometry::IntersectorInterface> m_intersector;
        };
    } // namespace RenderGeometry

} // namespace AzFramework
