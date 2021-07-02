/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>

#include <Editor/EditorSubComponentModeBase.h>

namespace PhysX
{
    enum class EditorSubComponentModeType : AZ::u8
    {
        Linear, /// sub-component mode to modify a single linear value, e.g. a float value.
        AnglePair, /// sub-component mode to modify a pair of float values representing angles.
        AngleCone, /// sub-component mode to modify a constraint's swing limits and local transformation.
        Rotation, /// sub-component mode to modify local transformation.
        SnapPosition, /// sub-component mode to modify local position using a point-and-snap feature in the viewport.
        SnapRotation, /// sub-component mode to modify local rotation using a point-and-snap feature in the viewport.
        Vec3 /// sub-component mode to modify a Vector3 value.
    };

    /// Contains configuration of a sub-component mode. Shared by different types of sub-component mode.
    /// Alternative implementation of this struct using AZStd::Variant is pending the development of the rest of the joint types.
    struct EditorSubComponentModeConfig
    {
        EditorSubComponentModeConfig() = default;

        EditorSubComponentModeConfig(const AZStd::string& name
            , EditorSubComponentModeType type);

        EditorSubComponentModeConfig(const AZStd::string& name
            , EditorSubComponentModeType type
            , float exponent
            , float max
            , float min);

        EditorSubComponentModeConfig(const AZStd::string& name
            , EditorSubComponentModeType type
            , const AZ::Vector3& axis
            , float max
            , float min);

        EditorSubComponentModeConfig(const AZStd::string& name
            , EditorSubComponentModeType type
            , float max
            , float min);

        AZStd::string m_name;
        EditorSubComponentModeType m_type;
        AZ::Vector3 m_axis = AZ::Vector3::CreateAxisX();
        float m_exponent = 1.0f;
        float m_max = FLT_MAX;
        float m_min = -FLT_MAX;
        bool m_selectLeadOnSnap = true; ///< A user may use the snap-to-position component mode to snap the position of a joint to an entity. This flag indicates if the snapped-to entity would be selected as a joint's lead when that happens.
    };

    /// Generic component mode that supports multiple sub-component modes.
    class EditorJointComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
    {
    public:
        static const AZStd::string s_parameterAngularPair;
        static const AZStd::string s_parameterDamping;
        static const AZStd::string s_parameterMaxForce;
        static const AZStd::string s_parameterMaxTorque;
        static const AZStd::string s_parameterPosition;
        static const AZStd::string s_parameterRotation;
        static const AZStd::string s_parameterSnapPosition;
        static const AZStd::string s_parameterSnapRotation;
        static const AZStd::string s_parameterStiffness;
        static const AZStd::string s_parameterSwingLimit;
        static const AZStd::string s_parameterTolerance;
        static const AZStd::string s_parameterTransform;
        static const AZStd::string s_parameterComponentMode;
        static const AZStd::string s_parameterLeadEntity;
        static const AZStd::string s_parameterSelectOnSnap;

        EditorJointComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType);
        ~EditorJointComponentMode();

        // EditorBaseComponentMode
        void Refresh() override;

        /// Returns map of sub-component mode configurations required by this component mode.
        virtual AZStd::map<AZStd::string, EditorSubComponentModeConfig> Configure() = 0;

    protected:
        /// AzToolsFramework::ViewportInteraction::MouseViewportRequests
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        /// EditorBaseComponentMode
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;

        /// Changes to the next sub-component mode found in m_configMap.
        void NextMode();

        /// Changes to the previous sub-component mode found in m_configMap.
        void PreviousMode();

        /// Changes to the next or previous sub-component mode found in m_configMap.
        void ChangeMode(bool forwardChange);

        /// Replaces m_currentSubComponentMode with a new one instantiated using the configuration identified by the input subComponentModeName.
        void SetCurrentSubComponentMode(const AZStd::string& subComponentModeName);

        AZStd::shared_ptr<EditorSubComponentModeBase> m_currentSubComponentMode = nullptr; ///< The active sub-component mode in this component mode.
        AZ::Uuid m_componentType = AZ::Uuid::CreateNull();
        AZStd::map<AZStd::string, EditorSubComponentModeConfig> m_configMap; ///< Contains sub-component mode configurations supported by this component mode.
        AZ::EntityComponentIdPair m_entityComponentIdPair;

    private:
        bool IsSubComponentModeUsed(const AZStd::string& subComponentModeName);
        
        void SetSubComponentModeAngleCone(const EditorSubComponentModeConfig& config);
        void SetSubComponentModeAnglePair(const EditorSubComponentModeConfig& config);
        void SetSubComponentModeLinear(const EditorSubComponentModeConfig& config);
        void SetSubComponentModeVec3(const EditorSubComponentModeConfig& config);
        void SetSubComponentModeRotation(const EditorSubComponentModeConfig& config);
        void SetSubComponentModeSnapPosition(const EditorSubComponentModeConfig& config);
        void SetSubComponentModeSnapRotation(const EditorSubComponentModeConfig& config);
    };

    /// Ball joint specific component mode. Configure() is overriden to set up the required sub-component modes.
    class EditorBallJointComponentMode
        : public EditorJointComponentMode
    {
    public:
        EditorBallJointComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType);
        ~EditorBallJointComponentMode();

        // EditorJointComponentMode
        AZStd::map<AZStd::string, EditorSubComponentModeConfig> Configure() override;
    };

    /// Fixed joint specific component mode. Configure() is overriden to set up the required sub-component modes.
    class EditorFixedJointComponentMode
        : public EditorJointComponentMode
    {
    public:
        EditorFixedJointComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType);
        ~EditorFixedJointComponentMode();

        // EditorJointComponentMode
        AZStd::map<AZStd::string, EditorSubComponentModeConfig> Configure() override;
    };

    /// Hinge joint specific component mode. Configure() is overriden to set up the required sub-component modes.
    class EditorHingeJointComponentMode
        : public EditorJointComponentMode
    {
    public:
        EditorHingeJointComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType);
        ~EditorHingeJointComponentMode();

        // EditorJointComponentMode
        AZStd::map<AZStd::string, EditorSubComponentModeConfig> Configure() override;
    };

    using ConfigMap = AZStd::map<AZStd::string, EditorSubComponentModeConfig>;
    using ConfigMapIter = ConfigMap::iterator;
    using ConfigMapReverseIter = ConfigMap::reverse_iterator;
} // namespace PhysX
