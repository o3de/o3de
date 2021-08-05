/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/TickBus.h>

namespace MaterialEditor
{
    //! Performs a single type of action for MaterialEditorViewportInputController based on input
    //! See derived behaviors for specific details
    class Behavior
        : public AZ::TickBus::Handler
    {
    public:
        Behavior();
        virtual ~Behavior();

        virtual void Start();
        virtual void End();
        virtual void MoveX(float value);
        virtual void MoveY(float value);
        virtual void MoveZ(float value);

    protected:
        bool HasDelta() const;
        virtual void TickInternal(float x, float y, float z);
        virtual float GetSensitivityX();
        virtual float GetSensitivityY();
        virtual float GetSensitivityZ();

        //! Calculate rotation quaternion towards a forward vector along world up axis
        static AZ::Quaternion LookRotation(AZ::Vector3 forward);
        //! Lerp a float value to 0 then decrement it by that interval and return it
        static float TakeStep(float& value, float t);

        //! Time in seconds to approximately complete a transformation
        static constexpr float LerpTime = 0.05f;
        //! If delta transform less than this, snap instantly
        static constexpr float SnapInterval = 0.01f;
        //! delta x movement accumulated during current frame
        float m_x = 0;
        //! delta y movement accumulated during current frame
        float m_y = 0;
        //! delta scroll wheel accumulated during current frame
        float m_z = 0;
        //! Model radius
        float m_radius = 1.0f;

        AZ::EntityId m_cameraEntityId;
        AZ::Vector3 m_targetPosition = AZ::Vector3::CreateZero();
        float m_distanceToTarget = 0;

    private:
        // AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    };
} // namespace MaterialEditor
