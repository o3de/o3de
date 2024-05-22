/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>

#include <AzCore/Script/ScriptTimePoint.h>

namespace DebugDraw
{
    class DebugDrawLineElement
    {
    public:
        AZ_CLASS_ALLOCATOR(DebugDrawLineElement, AZ::SystemAllocator);
        AZ_TYPE_INFO(DebugDrawLineElement, "{A26E844A-36C6-4832-B779-237019324FAA}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId            m_startEntityId;
        AZ::EntityId            m_endEntityId;
        float                   m_duration = 0.0f;
        AZ::ScriptTimePoint     m_activateTime;
        AZ::Color               m_color = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        AZ::Vector3             m_startWorldLocation = AZ::Vector3::CreateZero();
        AZ::Vector3             m_endWorldLocation = AZ::Vector3::CreateZero();
        AZ::ComponentId         m_owningEditorComponent = AZ::InvalidComponentId;
    };

    class DebugDrawRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DebugDrawRequests() = default;

        /**
         * Draws an axis-aligned bounding-box (Aabb) in the world centered at worldLocation
         *
         * @param worldLocation     World location for the Aabb to be centered at
         * @param aabb              Aabb to render
         * @param color             Color of Aabb
         * @param duration          How long to display the Aabb for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawAabb([[maybe_unused]] const AZ::Aabb& aabb, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws an axis-aligned bounding-box (Aabb) in the world centered at targetEntity's location
         *
         * @param targetEntity      Entity for the world location of the Aabb to be centered at
         * @param Aabb              Aabb to render
         * @param color             Color of Aabb
         * @param duration          How long to display the Aabb for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawAabbOnEntity([[maybe_unused]] const AZ::EntityId& targetEntity, [[maybe_unused]] const AZ::Aabb& aabb, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws a line in the world for a specified duration
         *
         * @param startLocation     World location for the line to start at
         * @param endLocation       World location for the line to end at
         * @param color             Color of line
         * @param duration          How long to display the line for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawLineLocationToLocation([[maybe_unused]] const AZ::Vector3& startLocation, [[maybe_unused]] const AZ::Vector3& endLocation, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draw a batch of lines in the world

         * @param lineBatch          A collection of lines
         */
        virtual void DrawLineBatchLocationToLocation([[maybe_unused]] const AZStd::vector<DebugDraw::DebugDrawLineElement>& lineBatch) {}

        /**
         * Draws a line in the world from an entity to a location for a specified duration
         *
         * @param startEntity   Entity for the world location of the line to start at
         * @param endLocation   World location for the line to end at
         * @param color         Color of line
         * @param duration      How long to display the line for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawLineEntityToLocation([[maybe_unused]] const AZ::EntityId& startEntity, [[maybe_unused]] const AZ::Vector3& endLocation, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws a line in the world from an entity to a location for a specified duration
         *
         * @param startEntity   Entity for the world location of the line to start at
         * @param endEntity     Entity for the world location of the line to end at
         * @param color         Color of line
         * @param duration      How long to display the line for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawLineEntityToEntity([[maybe_unused]] const AZ::EntityId& startEntity, [[maybe_unused]] const AZ::EntityId& endEntity, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws an oriented bounding-box (Obb) in the world
         *
         * @param obb               Obb to render
         * @param color             Color of Obb
         * @param duration          How long to display the Obb for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawObb([[maybe_unused]] const AZ::Obb& obb, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws an oriented bounding-box (Obb) in the world centered at targetEntity's location and in entity space (rotates/scales with entity)
         *
         * @param targetEntity      Entity for the Obb to be transformed by (located at entity location, rotates/scales with entity)
         * @param Obb               Obb to render
         * @param color             Color of Obb
         * @param duration          How long to display the Obb for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawObbOnEntity([[maybe_unused]] const AZ::EntityId& targetEntity, [[maybe_unused]] const AZ::Obb& obb, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] bool enableRayTracing, [[maybe_unused]] float duration) {}

        /**
         * Draws text in the world centered at worldLocation
         *
         * @param worldLocation     World location for the text to be centered at
         * @param text              Text to be displayed
         * @param color             Color of text
         * @param duration          How long to display the text for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawTextAtLocation([[maybe_unused]] const AZ::Vector3& worldLocation, [[maybe_unused]] const AZStd::string& text, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws text in the world at targetEntity's location
         *
         * @param targetEntity      Entity for the world location of the text to be centered at
         * @param text              Text to be displayed
         * @param color             Color of text
         * @param duration          How long to display the text for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawTextOnEntity([[maybe_unused]] const AZ::EntityId& targetEntity, [[maybe_unused]] const AZStd::string& text, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws text on the screen
         *
         * @param text              Text to be displayed. prefix with "-category:Name " for automatic grouping of screen text
         *                          Ex: "-category:MyRenderingInfo FPS:60" will draw "FPS:60" in a MyRenderingInfo category box
         * @param color             Color of text
         * @param duration          How long to display the text for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawTextOnScreen([[maybe_unused]] const AZStd::string& text, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws text on the screen with scaled default render font.
         *
         * @param text              Text to be displayed.
         * @param fontScale         Scale factor to default render font.
         * @param color             Color of text.
         * @param duration          How long to display the text for (in seconds); 0 value will draw for one frame; negative values draw forever.
         */
        virtual void DrawScaledTextOnScreen([[maybe_unused]] const AZStd::string& text
            , [[maybe_unused]] float fontScale, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws text on the screen with scaled default render font at given 2D coordinates.
         *
         * @param x                 X coordinate.
         * @param y                 Y coordinate.
         * @param text              Text to be displayed.
         * @param fontScale         Scale factor to default render font.
         * @param color             Color of text.
         * @param duration          How long to display the text for (in seconds); 0 value will draw for one frame; negative values draw forever.
         * @param bCenter           If true (default), centers drawn text relative to x coordinate, otherwise text is left-aligned.
         */
        virtual void DrawScaledTextOnScreenPos([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] const AZStd::string& text
            , [[maybe_unused]] float fontScale, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration, [[maybe_unused]] bool bCenter = true) {}

        /**
         * Draws a ray in the world for a specified duration
         *
         * @param worldLocation     World location for the ray to start at
         * @param worldDirection    World direction for the ray to draw towards
         * @param color             Color of ray
         * @param duration          How long to display the ray for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawRayLocationToDirection([[maybe_unused]] const AZ::Vector3& worldLocation, [[maybe_unused]] const AZ::Vector3& worldDirection, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws a ray in the world starting at an entity's location for a specified duration
         *
         * @param startEntity       Entity for the world location of the ray to start at
         * @param worldDirection    World direction for the ray to draw towards
         * @param color             Color of ray
         * @param duration          How long to display the ray for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawRayEntityToDirection([[maybe_unused]] const AZ::EntityId& startEntity, [[maybe_unused]] const AZ::Vector3& worldDirection, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws a ray in the world starting at an entity's location and ending at another's for a specified duration
         *
         * @param startEntity       Entity for the world location of the ray to start at
         * @param endEntity         Entity for the world location of the ray to end at
         * @param color             Color of ray
         * @param duration          How long to display the ray for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawRayEntityToEntity([[maybe_unused]] const AZ::EntityId& startEntity, [[maybe_unused]] const AZ::EntityId& endEntity, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws a sphere in the world centered at worldLocation
         *
         * @param worldLocation     World location for the sphere to be centered at
         * @param radius            Radius of the sphere
         * @param color             Color of sphere
         * @param duration          How long to display the sphere for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawSphereAtLocation([[maybe_unused]] const AZ::Vector3& worldLocation, [[maybe_unused]] float radius, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] float duration) {}

        /**
         * Draws a sphere in the world centered at targetEntity's location
         *
         * @param targetEntity      Entity for the world location of the sphere to be centered at
         * @param radius            Radius of the sphere
         * @param color             Color of sphere
         * @param duration          How long to display the sphere for; 0 value will draw for one frame; negative values draw forever
         */
        virtual void DrawSphereOnEntity([[maybe_unused]] const AZ::EntityId& targetEntity, [[maybe_unused]] float radius, [[maybe_unused]] const AZ::Color& color, [[maybe_unused]] bool enableRayTracing, [[maybe_unused]] float duration) {}
    };
    using DebugDrawRequestBus = AZ::EBus<DebugDrawRequests>;

    class DebugDrawInternalRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DebugDrawInternalRequests() = default;

        /**
         * Registers a DebugDraw component with the DebugDraw system component
         *
         * @param component         DebugDraw component that needs registered
         */
        virtual void RegisterDebugDrawComponent(AZ::Component* component) = 0;

        /**
         * Unregisters a DebugDraw component with the DebugDraw system component
         *
         * @param component         DebugDraw component that needs unregistered
         */
        virtual void UnregisterDebugDrawComponent(AZ::Component* component) = 0;
    };
    using DebugDrawInternalRequestBus = AZ::EBus<DebugDrawInternalRequests>;
} // namespace DebugDraw
