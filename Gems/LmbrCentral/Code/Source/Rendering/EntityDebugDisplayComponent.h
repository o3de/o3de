/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace LmbrCentral
{
    /**
     * Base type to be used to do custom component debug drawing.
     */
    class EntityDebugDisplayComponent
        : public AZ::Component
        , private AZ::TransformNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityDebugDisplayComponent, AZ::SystemAllocator);
        AZ_RTTI(EntityDebugDisplayComponent, "{091EA609-13E9-4553-83BA-36878CBAB950}", AZ::Component);

        EntityDebugDisplayComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        
        /**
         * Interface to draw using DebugDisplayRequests API.
         */
        virtual void Draw(AzFramework::DebugDisplayRequests& debugDisplay) = 0;

        const AZ::Transform& GetCurrentTransform() const { return m_currentEntityTransform; }
    
    private:
        AZ_DISABLE_COPY_MOVE(EntityDebugDisplayComponent)

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        AZ::Transform m_currentEntityTransform; ///< Stores the transform of the entity.
    };
}
