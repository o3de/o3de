
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX_precompiled.h>

#include <Editor/EditorJointConfiguration.h>
#include <Editor/EditorJointComponentMode.h>
#include <Editor/EditorSubComponentModeAngleCone.h>
#include <Editor/EditorSubComponentModeAnglePair.h>
#include <Editor/EditorSubComponentModeLinear.h>
#include <Editor/EditorSubComponentModeRotation.h>
#include <Editor/EditorSubComponentModeSnapPosition.h>
#include <Editor/EditorSubComponentModeSnapRotation.h>
#include <Editor/EditorSubComponentModeVec3.h>
#include <PhysX/EditorJointBus.h>

namespace PhysX
{
    namespace
    {
        //! Uri's for shortcut actions.
        const AZ::Crc32 GoToNextModeActionUri = AZ_CRC("com.amazon.action.physx.joint.nextmode", 0xe9cf4ed6);
        const AZ::Crc32 GoToPrevModeActionUri = AZ_CRC("com.amazon.action.physx.joint.prevmode", 0xe70f8daa);
    }

    const AZStd::string EditorJointComponentMode::s_parameterAngularPair = "Twist Limits";
    const AZStd::string EditorJointComponentMode::s_parameterDamping = "Damping";
    const AZStd::string EditorJointComponentMode::s_parameterMaxForce = "Maximum Force";
    const AZStd::string EditorJointComponentMode::s_parameterMaxTorque = "Maximum Torque";
    const AZStd::string EditorJointComponentMode::s_parameterPosition = "Position";
    const AZStd::string EditorJointComponentMode::s_parameterRotation = "Rotation";
    const AZStd::string EditorJointComponentMode::s_parameterSnapPosition = "Snap Position";
    const AZStd::string EditorJointComponentMode::s_parameterSnapRotation = "Snap Rotation";
    const AZStd::string EditorJointComponentMode::s_parameterStiffness = "Stiffness";
    const AZStd::string EditorJointComponentMode::s_parameterSwingLimit = "Swing Limits";
    const AZStd::string EditorJointComponentMode::s_parameterTolerance = "Tolerance";
    const AZStd::string EditorJointComponentMode::s_parameterTransform = "Transform";
    const AZStd::string EditorJointComponentMode::s_parameterComponentMode = "Component Mode";
    const AZStd::string EditorJointComponentMode::s_parameterLeadEntity = "Lead Entity";
    const AZStd::string EditorJointComponentMode::s_parameterSelectOnSnap = "Select on Snap";

    EditorSubComponentModeConfig::EditorSubComponentModeConfig(const AZStd::string& name
        , EditorSubComponentModeType type)
        : m_name(name), m_type(type)
    {
    }

    EditorSubComponentModeConfig::EditorSubComponentModeConfig(const AZStd::string& name
        , EditorSubComponentModeType type
        , float exponent
        , float max
        , float min)
        : m_name(name), m_type(type), m_exponent(exponent), m_max(max), m_min(min)
    {
    }

    EditorSubComponentModeConfig::EditorSubComponentModeConfig(const AZStd::string& name
        , EditorSubComponentModeType type
        , const AZ::Vector3& axis
        , float max
        , float min)
        : m_name(name), m_type(type), m_axis(axis), m_max(max), m_min(min)
    {
    }

    EditorSubComponentModeConfig::EditorSubComponentModeConfig(const AZStd::string& name
        , EditorSubComponentModeType type
        , float max
        , float min)
        : m_name(name), m_type(type), m_max(max), m_min(min)
    {
    }

    EditorJointComponentMode::EditorJointComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_entityComponentIdPair(entityComponentIdPair)
        , m_componentType(componentType)
    {
        EditorJointRequestBus::Event(
            m_entityComponentIdPair
            , &EditorJointRequests::SetBoolValue
            , EditorJointComponentMode::s_parameterComponentMode
            , true);
    }

    EditorJointComponentMode::~EditorJointComponentMode()
    {
        EditorJointRequestBus::Event(
            m_entityComponentIdPair
            , &EditorJointRequests::SetBoolValue
            , EditorJointComponentMode::s_parameterComponentMode
            , false);
    }

    bool EditorJointComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Wheel &&
            mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
        {
            NextMode();
            return true;
        }

        // Propagate mouse interaction to sub-component mode.
        if (m_currentSubComponentMode)
        {
            m_currentSubComponentMode->HandleMouseInteraction(mouseInteraction);
        }

        return false;
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorJointComponentMode::PopulateActionsImpl()
    {
        AzToolsFramework::ActionOverride goToNextMode;
        goToNextMode.SetUri(GoToNextModeActionUri);
        goToNextMode.SetKeySequence(QKeySequence(Qt::Key_Tab));
        goToNextMode.SetTitle("Next Mode");
        goToNextMode.SetTip("Go to next mode");
        goToNextMode.SetEntityComponentIdPair(GetEntityComponentIdPair());
        goToNextMode.SetCallback([this]()
        {
            NextMode();
        });

        AzToolsFramework::ActionOverride goToPrevMode;
        goToPrevMode.SetUri(GoToPrevModeActionUri);
        goToPrevMode.SetKeySequence(QKeySequence(Qt::SHIFT + Qt::Key_Tab));
        goToPrevMode.SetTitle("Previous Mode");
        goToPrevMode.SetTip("Go to previous mode");
        goToPrevMode.SetEntityComponentIdPair(GetEntityComponentIdPair());
        goToPrevMode.SetCallback([this]()
        {
            PreviousMode();
        });

        return {goToNextMode, goToPrevMode};
    }

    void EditorJointComponentMode::NextMode()
    {
        const bool isForwardChange = true;
        ChangeMode(isForwardChange);
    }

    void EditorJointComponentMode::PreviousMode()
    {
        const bool isForwardChange = false;
        ChangeMode(isForwardChange);
    }

    void EditorJointComponentMode::ChangeMode(bool forwardChange)
    {
        if (m_configMap.empty())
        {
            return;
        }

        AZStd::string previousModeName;
        if (m_currentSubComponentMode)
        {
            previousModeName = m_currentSubComponentMode->m_name;
            m_currentSubComponentMode.reset();
        }

        ConfigMapIter configIter = m_configMap.begin();
        if (!previousModeName.empty())
        {
            configIter = m_configMap.find(previousModeName);
        }

        AZ::u32 iterCount = 0;
        do
        {
            if (forwardChange)
            {
                ++configIter;
                if (configIter == m_configMap.end())
                {
                    configIter = m_configMap.begin();
                }
            }
            else
            {
                if (configIter == m_configMap.begin())
                {
                    configIter = m_configMap.end();
                }
                --configIter;
            }
            ++iterCount;
        } while (!IsSubComponentModeUsed(configIter->second.m_name) && iterCount != m_configMap.size());

        if (iterCount == m_configMap.size()) // All sub component modes are not in use.
        {
            return;
        }

        SetCurrentSubComponentMode(configIter->second.m_name);
    }

    void EditorJointComponentMode::SetCurrentSubComponentMode(const AZStd::string& subComponentModeName)
    {
        ConfigMapIter configIter = m_configMap.find(subComponentModeName);
        if (configIter == m_configMap.end())
        {
            AZ_Warning("EditorJointComponentMode"
                , false
                , "Attempt to set sub component mode which does not exist: %s"
                , subComponentModeName.c_str());
            return;
        }
        EditorSubComponentModeConfig config = configIter->second;

        EditorJointRequestBus::EventResult(
            config.m_selectLeadOnSnap, m_entityComponentIdPair
            , &EditorJointRequests::GetBoolValue
            , s_parameterSelectOnSnap);

        m_currentSubComponentMode.reset();

        switch (config.m_type)
        {
        case EditorSubComponentModeType::Linear:
            SetSubComponentModeLinear(config);
            break;
        case EditorSubComponentModeType::AnglePair:
            SetSubComponentModeAnglePair(config);
            break;
        case EditorSubComponentModeType::AngleCone:
            SetSubComponentModeAngleCone(config);
            break;
        case EditorSubComponentModeType::Vec3:
            SetSubComponentModeVec3(config);
            break;
        case EditorSubComponentModeType::Rotation:
            SetSubComponentModeRotation(config);
            break;
        case EditorSubComponentModeType::SnapPosition:
            SetSubComponentModeSnapPosition(config);
            break;
        case EditorSubComponentModeType::SnapRotation:
            SetSubComponentModeSnapRotation(config);
            break;
        default:
            AZ_Error("EditorJointComponentMode::SetCurrentSubComponentMode"
                , false
                , "Unsupported sub-component mode type.");
        }
    }

    bool EditorJointComponentMode::IsSubComponentModeUsed(const AZStd::string& subComponentModeName)
    {
        bool isUsed = false;
        EditorJointRequestBus::EventResult(
            isUsed, m_entityComponentIdPair
            , &EditorJointRequests::IsParameterUsed
            , subComponentModeName);
        return isUsed;
    }

    void EditorJointComponentMode::SetSubComponentModeAngleCone(const EditorSubComponentModeConfig& config)
    {
        m_currentSubComponentMode = AZStd::make_shared<EditorSubComponentModeAngleCone>(m_entityComponentIdPair
            , m_componentType
            , config.m_name
            , config.m_max
            , config.m_min);
    }

    void EditorJointComponentMode::SetSubComponentModeAnglePair(const EditorSubComponentModeConfig& config)
    {
        m_currentSubComponentMode = AZStd::make_shared<EditorSubComponentModeAnglePair>(m_entityComponentIdPair
            , m_componentType
            , config.m_name
            , config.m_axis
            , config.m_max
            , config.m_min
            , config.m_min
            , -config.m_max);
    }

    void EditorJointComponentMode::SetSubComponentModeLinear(const EditorSubComponentModeConfig& config)
    {
        m_currentSubComponentMode = AZStd::make_shared<EditorSubComponentModeLinear>(m_entityComponentIdPair
            , m_componentType
            , config.m_name
            , config.m_exponent
            , config.m_max
            , config.m_min);
    }

    void EditorJointComponentMode::SetSubComponentModeVec3(const EditorSubComponentModeConfig& config)
    {
        m_currentSubComponentMode = AZStd::make_shared<EditorSubComponentModeVec3>(m_entityComponentIdPair
            , m_componentType
            , config.m_name);
    }

    void EditorJointComponentMode::SetSubComponentModeRotation(const EditorSubComponentModeConfig& config)
    {
        m_currentSubComponentMode = AZStd::make_shared<EditorSubComponentModeRotation>(m_entityComponentIdPair
            , m_componentType
            , config.m_name);
    }

    void EditorJointComponentMode::SetSubComponentModeSnapPosition(const EditorSubComponentModeConfig& config)
    {
        m_currentSubComponentMode = AZStd::make_shared<EditorSubComponentModeSnapPosition>(m_entityComponentIdPair
            , m_componentType
            , config.m_name
            , config.m_selectLeadOnSnap);
    }

    void EditorJointComponentMode::SetSubComponentModeSnapRotation(const EditorSubComponentModeConfig& config)
    {
        m_currentSubComponentMode = AZStd::make_shared<EditorSubComponentModeSnapRotation>(m_entityComponentIdPair
            , m_componentType
            , config.m_name);
    }

    EditorBallJointComponentMode::EditorBallJointComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType)
        : EditorJointComponentMode(entityComponentIdPair, componentType)
    {
        m_configMap = Configure();
        NextMode();
    }

    EditorBallJointComponentMode::~EditorBallJointComponentMode()
    {
        if (m_currentSubComponentMode)
        {
            m_currentSubComponentMode.reset();
        }
    }

    void EditorJointComponentMode::Refresh()
    {
        if (m_currentSubComponentMode)
        {
            m_currentSubComponentMode->Refresh();
        }
    }

    AZStd::map<AZStd::string, EditorSubComponentModeConfig> EditorBallJointComponentMode::Configure()
    {
        AZStd::map<AZStd::string, EditorSubComponentModeConfig> configMap;

        configMap.emplace(
            EditorJointComponentMode::s_parameterPosition
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterPosition
                , EditorSubComponentModeType::Vec3)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterRotation
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterRotation
                , EditorSubComponentModeType::Rotation)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterSnapPosition
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterSnapPosition
                , EditorSubComponentModeType::SnapPosition)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterSnapRotation
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterSnapRotation
                , EditorSubComponentModeType::SnapRotation)
        );

        const float exponentBreakage = 1.0f;

        configMap.emplace(
            EditorJointComponentMode::s_parameterMaxForce
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterMaxForce
                , EditorSubComponentModeType::Linear
                , exponentBreakage
                , PhysX::EditorJointConfig::s_breakageMax
                , PhysX::EditorJointConfig::s_breakageMin)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterMaxTorque
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterMaxTorque
                , EditorSubComponentModeType::Linear
                , exponentBreakage
                , PhysX::EditorJointConfig::s_breakageMax
                , PhysX::EditorJointConfig::s_breakageMin)
        );

        const float exponentSpring = 2.0f;

        configMap.emplace(
            EditorJointComponentMode::s_parameterDamping
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterDamping
                , EditorSubComponentModeType::Linear
                , exponentSpring
                , PhysX::EditorJointLimitConeConfig::s_springMax
                , PhysX::EditorJointLimitConeConfig::s_springMin)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterStiffness
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterStiffness
                , EditorSubComponentModeType::Linear
                , exponentSpring
                , PhysX::EditorJointLimitConeConfig::s_springMax
                , PhysX::EditorJointLimitConeConfig::s_springMin)
        );

        // Cone tip to base is always X-axis. 
        //The angle cone defines the limitations for rotation about the Y and Z axes.
        configMap.emplace(
            EditorJointComponentMode::s_parameterSwingLimit
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterSwingLimit
                , EditorSubComponentModeType::AngleCone
                , PhysX::EditorJointLimitConeConfig::s_angleMax
                , PhysX::EditorJointLimitConeConfig::s_angleMin)
        );

        return configMap;
    }

    EditorFixedJointComponentMode::EditorFixedJointComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType)
        : EditorJointComponentMode(entityComponentIdPair, componentType)
    {
        m_configMap = Configure();
        NextMode();
    }

    EditorFixedJointComponentMode::~EditorFixedJointComponentMode()
    {
        if (m_currentSubComponentMode)
        {
            m_currentSubComponentMode.reset();
        }
    }

    AZStd::map<AZStd::string, EditorSubComponentModeConfig> EditorFixedJointComponentMode::Configure()
    {
        AZStd::map<AZStd::string, EditorSubComponentModeConfig> configMap;

        configMap.emplace(
            EditorJointComponentMode::s_parameterPosition
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterPosition
                , EditorSubComponentModeType::Vec3)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterRotation
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterRotation
                , EditorSubComponentModeType::Rotation)
        );

        const float exponentBreakage = 1.0f;

        configMap.emplace(
            EditorJointComponentMode::s_parameterMaxForce
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterMaxForce
                , EditorSubComponentModeType::Linear
                , exponentBreakage
                , PhysX::EditorJointConfig::s_breakageMax
                , PhysX::EditorJointConfig::s_breakageMin)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterMaxTorque
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterMaxTorque
                , EditorSubComponentModeType::Linear
                , exponentBreakage
                , PhysX::EditorJointConfig::s_breakageMax
                , PhysX::EditorJointConfig::s_breakageMin)
        );

        return configMap;
    }

    EditorHingeJointComponentMode::EditorHingeJointComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType)
        : EditorJointComponentMode(entityComponentIdPair, componentType)
    {
        m_configMap = Configure();
        NextMode();
    }

    EditorHingeJointComponentMode::~EditorHingeJointComponentMode()
    {
        if (m_currentSubComponentMode)
        {
            m_currentSubComponentMode.reset();
        }
    }

    AZStd::map<AZStd::string, EditorSubComponentModeConfig> EditorHingeJointComponentMode::Configure()
    {
        AZStd::map<AZStd::string, EditorSubComponentModeConfig> configMap;

        configMap.emplace(
            EditorJointComponentMode::s_parameterPosition
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterPosition
                , EditorSubComponentModeType::Vec3)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterRotation
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterRotation
                , EditorSubComponentModeType::Rotation)
        );

        const float exponentBreakage = 1.0f;

        configMap.emplace(
            EditorJointComponentMode::s_parameterMaxForce
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterMaxForce
                , EditorSubComponentModeType::Linear
                , exponentBreakage
                , PhysX::EditorJointConfig::s_breakageMax
                , PhysX::EditorJointConfig::s_breakageMin)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterMaxTorque
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterMaxTorque
                , EditorSubComponentModeType::Linear
                , exponentBreakage
                , PhysX::EditorJointConfig::s_breakageMax
                , PhysX::EditorJointConfig::s_breakageMin)
        );

        const float exponentSpring = 2.0f;

        configMap.emplace(
            EditorJointComponentMode::s_parameterDamping
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterDamping
                , EditorSubComponentModeType::Linear
                , exponentSpring
                , PhysX::EditorJointLimitPairConfig::s_springMax
                , PhysX::EditorJointLimitPairConfig::s_springMin)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterStiffness
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterStiffness
                , EditorSubComponentModeType::Linear
                , exponentSpring
                , PhysX::EditorJointLimitPairConfig::s_springMax
                , PhysX::EditorJointLimitPairConfig::s_springMin)
        );

        AZ::Vector3 axis = AZ::Vector3::CreateAxisX(); // PhysX revolute joints uses the x-axis by default
        configMap.emplace(
            EditorJointComponentMode::s_parameterAngularPair
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterAngularPair
                , EditorSubComponentModeType::AnglePair
                , axis
                , PhysX::EditorJointLimitPairConfig::s_angleMax
                , PhysX::EditorJointLimitPairConfig::s_angleMin)
        );

        configMap.emplace(
            EditorJointComponentMode::s_parameterSnapPosition
            , EditorSubComponentModeConfig(EditorJointComponentMode::s_parameterSnapPosition
                , EditorSubComponentModeType::SnapPosition)
        );

        return configMap;
    }
} // namespace LmbrCentral
