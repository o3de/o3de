/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace PhysX
{
    namespace Debug
    {
        enum class PvdTransportType
        {
            Network,
            File
        };

        enum class PvdAutoConnectMode
        {
            Disabled, //!< Auto connection is disabled (default).
            Editor, //!< Auto connection takes place on editor launch and remains open until closed.
            Game, //!< Auto connection for game mode.
            Server //!<  Auto connection from the server.
        };

        //! Contains configuration data for initializing and interacting with PhysX Visual Debugger (PVD).
        struct PvdConfiguration
        {
            AZ_TYPE_INFO(PvdConfiguration, "{B60BDDCE-AA95-497B-BA9B-2E7DBD4DD189}");
            static void Reflect(AZ::ReflectContext* context);

            //! Determine if auto connection is enabled for Editor mode.
            bool IsAutoConnectionEditorMode() const { return m_autoConnectMode == PvdAutoConnectMode::Editor; }
            //! Determine if auto connection is enabled for Game mode.
            bool IsAutoConnectionGameMode() const { return m_autoConnectMode == PvdAutoConnectMode::Game; }

            //! Determine if the current debug type preference is the network (for the editor context).
            bool IsNetworkDebug() const { return m_transportType == PvdTransportType::Network; }
            //! Determine if the current debug type preference is file output (for the editor context).
            bool IsFileDebug() const { return m_transportType == PvdTransportType::File; }

            bool m_reconnect = true; //!< Reconnect when switching between game and edit mode automatically (Editor mode only).
            PvdTransportType m_transportType = PvdTransportType::Network; //!< PhysX Visual Debugger transport preference.
            int m_port = 5425; //!< PhysX Visual Debugger port (default: 5425).
            unsigned int m_timeoutInMilliseconds = 10; //!< Timeout used when connecting to PhysX Visual Debugger.
            PvdAutoConnectMode m_autoConnectMode = PvdAutoConnectMode::Disabled; ///< PVD auto connect preference.
            AZStd::string m_fileName = "physxDebugInfo.pxd2"; //!< PhysX Visual Debugger output filename.
            AZStd::string m_host = "127.0.0.1"; //!< PhysX Visual Debugger hostname.

            bool operator==(const PvdConfiguration& other) const;
            bool operator!=(const PvdConfiguration& other) const;
        };

        struct ColliderProximityVisualization
        {
            AZ_TYPE_INFO(ColliderProximityVisualization, "{2A9BA0AE-C6A7-4F87-B7F0-D62444035478}");
            static void Reflect(AZ::ReflectContext* context);

            ColliderProximityVisualization() = default;
            ColliderProximityVisualization(bool enabled, const AZ::Vector3 cameraPosition, float radius)
                : m_enabled(enabled)
                , m_cameraPosition(cameraPosition)
                , m_radius(radius)
            {

            }
            bool m_enabled = false; //!< If camera proximity based collider visualization is currently active.
            AZ::Vector3 m_cameraPosition = AZ::Vector3::CreateZero(); //!< Camera position to perform proximity based collider visulization around.
            float m_radius = 1.0f; //!< The radius to visualize colliders around the camera position.

            bool operator==(const ColliderProximityVisualization& other) const;
            bool operator!=(const ColliderProximityVisualization& other) const;
        };

        //! Contains various options for debug display of PhysX features.
        struct DebugDisplayData
        {
            AZ_TYPE_INFO(DebugDisplayData, "{E9F1C386-3726-45B8-8DA5-BF7135B3ACD0}");
            static void Reflect(AZ::ReflectContext* context);

            //! Center of Mass Debug Draw Circle Size
            float m_centerOfMassDebugSize = 0.1f;

            //! Center of Mass Debug Draw Circle Color
            AZ::Color m_centerOfMassDebugColor = AZ::Color((AZ::u8)255, (AZ::u8)0, (AZ::u8)0, (AZ::u8)255);

            //! Enable Global Collision Debug Draw
            enum class GlobalCollisionDebugState
            {
                AlwaysOn,         //!< Collision draw debug all entities.
                AlwaysOff,        //!< Collision debug draw disabled.
                Manual            //!< Set up in the entity.
            };
            GlobalCollisionDebugState m_globalCollisionDebugDraw = GlobalCollisionDebugState::Manual;

            //! Color scheme for debug collision
            enum class GlobalCollisionDebugColorMode
            {
                MaterialColor,   //!< Use debug color specified in the material
                ErrorColor       //!< Show default color and flashing red for colliders with errors.
            };
            GlobalCollisionDebugColorMode m_globalCollisionDebugDrawColorMode = GlobalCollisionDebugColorMode::MaterialColor;


            //! Colors for joint lead
            enum class JointLeadColor
            {
                Aquamarine,
                AliceBlue,
                CadetBlue,
                Coral,
                Green,
                DarkGreen,
                ForestGreen,
                Honeydew
            };

            //! Colors for joint follower
            enum class JointFollowerColor
            {
                Yellow,
                Chocolate,
                HotPink,
                Lavender,
                Magenta,
                LightYellow,
                Maroon,
                Red
            };

            AZ::Color GetJointLeadColor() const;
            AZ::Color GetJointFollowerColor() const;

            bool m_showJointHierarchy = true; //!< Flag to switch on/off the display of joints' lead-follower connections in the viewport.
            JointLeadColor m_jointHierarchyLeadColor = JointLeadColor::Aquamarine; //!< Color of the lead half of a lead-follower joint connection line.
            JointFollowerColor m_jointHierarchyFollowerColor = JointFollowerColor::Magenta; //!< Color of the follower half of a lead-follower joint connection line.
            float m_jointHierarchyDistanceThreshold = 1.0f; //!< Minimum distance required to draw from follower to joint. Distances shorter than this threshold will result in the line drawn from the joint to the lead.

            ColliderProximityVisualization m_colliderProximityVisualization;

            bool operator==(const DebugDisplayData& other) const;
            bool operator!=(const DebugDisplayData& other) const;
        };

        struct DebugConfiguration
        {
            AZ_TYPE_INFO(DebugConfiguration, "{0338FF5C-8BF8-4AE5-857F-2F195581CC74}");
            static void Reflect(AZ::ReflectContext* context);

            static DebugConfiguration CreateDefault() { return DebugConfiguration(); }

            DebugDisplayData m_debugDisplayData;
            PvdConfiguration m_pvdConfigurationData;

            bool operator==(const DebugConfiguration& other) const;
            bool operator!=(const DebugConfiguration& other) const;
        };
    } // namespace Debug
}
