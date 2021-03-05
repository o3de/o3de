/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    protected:
        bool HasDelta() const;
        virtual void TickInternal([[maybe_unused]] float x, [[maybe_unused]] float y) {}
        virtual float GetSensitivityX();
        virtual float GetSensitivityY();

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

    private:
        // AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    };
} // namespace MaterialEditor
