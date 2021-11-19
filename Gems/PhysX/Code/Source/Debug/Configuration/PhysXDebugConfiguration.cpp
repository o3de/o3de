/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX/Debug/PhysXDebugConfiguration.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace PhysX
{
    namespace Debug
    {
        /*static*/ void PvdConfiguration::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<PvdConfiguration>()
                    ->Version(1)
                    ->Field("Reconnect", &PvdConfiguration::m_reconnect)
                    ->Field("TransportType", &PvdConfiguration::m_transportType)
                    ->Field("Port", &PvdConfiguration::m_port)
                    ->Field("TimeoutInMilliseconds", &PvdConfiguration::m_timeoutInMilliseconds)
                    ->Field("AutoConnectMode", &PvdConfiguration::m_autoConnectMode)
                    ->Field("FileName", &PvdConfiguration::m_fileName)
                    ->Field("Host", &PvdConfiguration::m_host)
                    ;

                if (AZ::EditContext* editContext = serialize->GetEditContext())
                {
                    editContext->Class<PvdConfiguration>("PhysX PVD Settings",
                        "Connection configuration settings for the PhysX Visual Debugger (PVD). Requires PhysX Debug Gem.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &PvdConfiguration::m_transportType,
                            "PVD Transport Type", "Output PhysX Visual Debugger data to a TCP/IP network socket or to a file.")
                        ->EnumAttribute(Debug::PvdTransportType::Network, "Network")
                        ->EnumAttribute(Debug::PvdTransportType::File, "File")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &PvdConfiguration::m_host,
                            "PVD Host", "Host IP address of the PhysX Visual Debugger server.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PvdConfiguration::IsNetworkDebug)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PvdConfiguration::m_port,
                            "PVD Port", "Port of the PhysX Visual Debugger server.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PvdConfiguration::IsNetworkDebug)
                        ->Attribute(AZ::Edit::Attributes::Min, AZStd::numeric_limits<uint16_t>::min())
                        ->Attribute(AZ::Edit::Attributes::Max, AZStd::numeric_limits<uint16_t>::max())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PvdConfiguration::m_timeoutInMilliseconds,
                            "PVD Timeout", "Timeout (in milliseconds) when connecting to the PhysX Visual Debugger server.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PvdConfiguration::IsNetworkDebug)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PvdConfiguration::m_fileName,
                            "PVD FileName", "Output filename for PhysX Visual Debugger data.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PvdConfiguration::IsFileDebug)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &PvdConfiguration::m_autoConnectMode,
                            "PVD Auto Connect", "Automatically connect to the PhysX Visual Debugger.")
                        ->EnumAttribute(Debug::PvdAutoConnectMode::Disabled, "Disabled")
                        ->EnumAttribute(Debug::PvdAutoConnectMode::Editor, "Editor")
                        ->EnumAttribute(Debug::PvdAutoConnectMode::Game, "Game")

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PvdConfiguration::m_reconnect, "PVD Reconnect",
                            "Reconnect (disconnect and connect) to the PhysX Visual Debugger server when switching between game and edit mode.")
                        ;
                }
            }
        }

        bool PvdConfiguration::operator==(const PvdConfiguration& other) const
        {
            return m_reconnect == other.m_reconnect &&
                m_transportType == other.m_transportType &&
                m_port == other.m_port &&
                m_timeoutInMilliseconds == other.m_timeoutInMilliseconds &&
                m_autoConnectMode == other.m_autoConnectMode &&
                m_fileName == other.m_fileName &&
                m_host == other.m_host
                ;
        }

        bool PvdConfiguration::operator!=(const PvdConfiguration& other) const
        {
            return !(*this == other);
        }

        /*static*/ void ColliderProximityVisualization::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<ColliderProximityVisualization>()
                    ->Version(1)
                    ->Field("Enabled", &ColliderProximityVisualization::m_enabled)
                    ->Field("CameraPosition", &ColliderProximityVisualization::m_cameraPosition)
                    ->Field("Radius", &ColliderProximityVisualization::m_radius)
                    ;
            }
        }
        
        bool ColliderProximityVisualization::operator==(const ColliderProximityVisualization& other) const
        {
            return m_enabled == other.m_enabled &&
                AZ::IsClose(m_radius, other.m_radius) &&
                m_cameraPosition == other.m_cameraPosition
                ;
        }
        bool ColliderProximityVisualization::operator!=(const ColliderProximityVisualization& other) const
        {
            return !(*this == other);
        }

        /*static*/ void DebugDisplayData::Reflect(AZ::ReflectContext* context)
        {
            ColliderProximityVisualization::Reflect(context);

            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<DebugDisplayData>()
                    ->Version(1)
                    ->Field("COMDebugSize", &DebugDisplayData::m_centerOfMassDebugSize)
                    ->Field("COMDebugColor", &DebugDisplayData::m_centerOfMassDebugColor)
                    ->Field("GlobalColliderDebugDraw", &DebugDisplayData::m_globalCollisionDebugDraw)
                    ->Field("GlobalColliderDebugDrawColorMode", &DebugDisplayData::m_globalCollisionDebugDrawColorMode)
                    ->Field("ShowJointHierarchy", &DebugDisplayData::m_showJointHierarchy)
                    ->Field("JointHierarchyLeadColor", &DebugDisplayData::m_jointHierarchyLeadColor)
                    ->Field("JointHierarchyFollowerColor", &DebugDisplayData::m_jointHierarchyFollowerColor)
                    ->Field("JointHierarchyDistanceThreshold", &DebugDisplayData::m_jointHierarchyDistanceThreshold)
                    ->Field("ColliderProximityVisualization", &DebugDisplayData::m_colliderProximityVisualization)
                    ;

                if (AZ::EditContext* editContext = serialize->GetEditContext())
                {
                    editContext->Class<DebugDisplayData>("Editor Configuration", "Editor settings for PhysX.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DebugDisplayData::m_centerOfMassDebugSize,
                            "Debug Draw Center of Mass Size", "The size of the debug draw circle representing the center of mass.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DebugDisplayData::m_centerOfMassDebugColor,
                            "Debug Draw Center of Mass Color", "The color of the debug draw circle representing the center of mass.")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DebugDisplayData::m_globalCollisionDebugDraw, "Global Collision Debug",
                            "Set up global collision debug draw."
                            "<ul style=\"margin-left:15px; margin-top:-10px; -qt-list-indent:0;\">"
                            "<li><b>Enable all colliders</b><br>Displays all PhysX collider shapes, including colliders previously set as hidden.\n</li>"
                            "<li><b>Disable all colliders</b><br>Hides all PhysX collider shapes, including colliders previously set as visible.\n</li>"
                            "<li><b>Set manually</b><br>You can update PhysX colliders on each entity. The default state is on.</li>"
                            "</ul>")
                        ->EnumAttribute(DebugDisplayData::GlobalCollisionDebugState::AlwaysOn, "Enable all colliders")
                        ->EnumAttribute(DebugDisplayData::GlobalCollisionDebugState::AlwaysOff, "Disable all colliders")
                        ->EnumAttribute(DebugDisplayData::GlobalCollisionDebugState::Manual, "Set manually")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DebugDisplayData::m_globalCollisionDebugDrawColorMode, "Global Collision Debug Color Mode",
                            "Set up debug color mode."
                            "<ul style=\"margin-left:15px; margin-top:-10px; -qt-list-indent:0;\">"
                            "<li><b>Material Color Mode</b><br>Uses material's debug color specified in material library.\n</li>"
                            "<li><b>Error Mode</b><br>Shows glowing red error colors for cases like meshes with too many triangles.\n</li>"
                            "</ul>")
                        ->EnumAttribute(DebugDisplayData::GlobalCollisionDebugColorMode::MaterialColor, "Material Color Mode")
                        ->EnumAttribute(DebugDisplayData::GlobalCollisionDebugColorMode::ErrorColor, "Error Mode")

                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &DebugDisplayData::m_showJointHierarchy,
                            "Display Joints Hierarchy",
                            "Flag to switch on / off the display of joint lead-follower connections in the viewport.")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DebugDisplayData::m_jointHierarchyLeadColor,
                            "Joints Hierarchy Lead Color",
                            "Color of the lead half of a lead-follower joint connection line.")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::Aquamarine, "Aquamarine")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::AliceBlue, "AliceBlue")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::CadetBlue, "CadetBlue")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::Coral, "Coral")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::Green, "Green")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::DarkGreen, "DarkGreen")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::ForestGreen, "ForestGreen")
                        ->EnumAttribute(DebugDisplayData::JointLeadColor::Honeydew, "Honeydew")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DebugDisplayData::m_jointHierarchyFollowerColor,
                            "Joints Hierarchy Follower Color",
                            "Color of the follower half of a lead-follower joint connection line.")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::Chocolate, "Chocolate")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::HotPink, "HotPink")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::Lavender, "Lavender")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::Magenta, "Magenta")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::LightYellow, "LightYellow")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::Maroon, "Maroon")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::Red, "Red")
                        ->EnumAttribute(DebugDisplayData::JointFollowerColor::Yellow, "Yellow")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DebugDisplayData::m_jointHierarchyDistanceThreshold,
                            "Joints Hierarchy Distance Threshold",
                            "Minimum distance required to draw from follower to joint. Distances shorter than this threshold will result in the line drawn from the joint to the lead.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.000001f)
                        ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ;
                }
            }
        }

        AZ::Color DebugDisplayData::GetJointLeadColor() const
        {
            switch (m_jointHierarchyLeadColor)
            {
            case JointLeadColor::Aquamarine:
                return AZ::Colors::Aquamarine;
            case JointLeadColor::AliceBlue:
                return AZ::Colors::AliceBlue;
            case JointLeadColor::CadetBlue:
                return AZ::Colors::CadetBlue;
            case JointLeadColor::Coral:
                return AZ::Colors::Coral;
            case JointLeadColor::Green:
                return AZ::Colors::Green;
            case JointLeadColor::DarkGreen:
                return AZ::Colors::DarkGreen;
            case JointLeadColor::ForestGreen:
                return AZ::Colors::ForestGreen;
            case JointLeadColor::Honeydew:
                return AZ::Colors::Honeydew;
            default:
                return AZ::Colors::Aquamarine;
            }
        }

        AZ::Color DebugDisplayData::GetJointFollowerColor() const
        {
            switch (m_jointHierarchyFollowerColor)
            {
            case JointFollowerColor::Chocolate:
                return AZ::Colors::Chocolate;
            case JointFollowerColor::HotPink:
                return AZ::Colors::HotPink;
            case JointFollowerColor::Lavender:
                return AZ::Colors::Lavender;
            case JointFollowerColor::Magenta:
                return AZ::Colors::Magenta;
            case JointFollowerColor::LightYellow:
                return AZ::Colors::LightYellow;
            case JointFollowerColor::Maroon:
                return AZ::Colors::Maroon;
            case JointFollowerColor::Red:
                return AZ::Colors::Red;
            case JointFollowerColor::Yellow:
                return AZ::Colors::Yellow;
            default:
                return AZ::Colors::Magenta;
            }
        }
        
        bool DebugDisplayData::operator==(const DebugDisplayData& other) const
        {
            return m_showJointHierarchy == other.m_showJointHierarchy &&
                m_globalCollisionDebugDraw == other.m_globalCollisionDebugDraw &&
                m_globalCollisionDebugDrawColorMode == other.m_globalCollisionDebugDrawColorMode &&
                m_centerOfMassDebugColor == other.m_centerOfMassDebugColor &&
                m_jointHierarchyLeadColor == other.m_jointHierarchyLeadColor &&
                AZ::IsClose(m_centerOfMassDebugSize, other.m_centerOfMassDebugSize) &&
                AZ::IsClose(m_jointHierarchyDistanceThreshold, other.m_jointHierarchyDistanceThreshold) &&
                m_colliderProximityVisualization == other.m_colliderProximityVisualization
                ;
        }

        bool DebugDisplayData::operator!=(const DebugDisplayData& other) const
        {
            return !(*this == other);
        }

        /*static*/ void DebugConfiguration::Reflect(AZ::ReflectContext* context)
        {
            DebugDisplayData::Reflect(context);
            PvdConfiguration::Reflect(context);

            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<DebugConfiguration>()
                    ->Version(1)
                    ->Field("DebugDisplayData", &DebugConfiguration::m_debugDisplayData)
                    ->Field("PvdConfigurationData", &DebugConfiguration::m_pvdConfigurationData)
                    ;
            }
        }

        bool DebugConfiguration::operator==(const DebugConfiguration& other) const
        {
            return m_debugDisplayData == other.m_debugDisplayData &&
                m_pvdConfigurationData == other.m_pvdConfigurationData
                ;
        }
        bool DebugConfiguration::operator!=(const DebugConfiguration& other) const
        {
            return !(*this == other);
        }
    } // namespace Debug
} //namespace PhysX
