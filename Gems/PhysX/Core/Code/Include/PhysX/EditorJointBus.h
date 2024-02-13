/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <Editor/EditorJointCommon.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>

namespace PhysX
{
    //! Messages serviced by Editor Joint Components.
    class EditorJointRequests
        : public AZ::EntityComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Get bool parameter value identified by name.
        //! @return Value of bool parameter.
        virtual bool GetBoolValue(const AZStd::string& parameterName) = 0;

        //! Get entityID parameter value identified by name.
        //! @return Value of entityID parameter.
        virtual AZ::EntityId GetEntityIdValue(const AZStd::string& parameterName) = 0;

        //! Get linear parameter value identified by name.
        //! @return Value of linear value parameter.
        virtual float GetLinearValue(const AZStd::string& parameterName) = 0;

        //! Get linear parameter value pair identified by name.
        //! @return Linear parameter value pair.
        virtual AngleLimitsFloatPair GetLinearValuePair(const AZStd::string& parameterName) = 0;

        //! Get vector3 value identified by name.
        //! @return Vector3 parameter value.
        virtual AZ::Vector3 GetVector3Value(const AZStd::string& parameterName) = 0;

        //! Get transform value identified by name.
        //! @return Transform parameter value.
        virtual AZ::Transform GetTransformValue(const AZStd::string& parameterName) = 0;

        //! Get the Sub Component modes to enable.
        virtual AZStd::vector<JointsComponentModeCommon::SubModeParameterState> GetSubComponentModesState() = 0;

        //! Set bool parameter value identified by name.
        virtual void SetBoolValue(const AZStd::string& parameterName, bool value) = 0;

        //! Set entity ID parameter value identified by name.
        virtual void SetEntityIdValue(const AZStd::string& parameterName, AZ::EntityId value) = 0;

        //! Set linear parameter value identified by name.
        virtual void SetLinearValue(const AZStd::string& parameterName, float value) = 0;

        //! Set linear parameter value pair identified by name.
        virtual void SetLinearValuePair(const AZStd::string& parameterName, const AngleLimitsFloatPair& valuePair) = 0;

        //! Set vector3 parameter value identified by name.
        virtual void SetVector3Value(const AZStd::string& parameterName, const AZ::Vector3& value) = 0;
    };
    using EditorJointRequestBus = AZ::EBus<EditorJointRequests>;
} // namespace PhysX
