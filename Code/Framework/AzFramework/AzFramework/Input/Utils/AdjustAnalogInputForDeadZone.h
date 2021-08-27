/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Utility function to adjust and normalize an analog input value so that values within a given
    //! dead-zone are set to zero, and values outside ramp up smoothly to a given max absolute value.
    //! \param[in] value The analog value to adjust and normalize
    //! \param[in] deadZone The dead zone within which the value will be set to zero
    //! \param[in] maximumAbsoluteValue The maximum absolute value that the value will be clamped to
    //! \return The analog value adjusted and normalized using the supplied dead-zone and max values
    inline float AdjustForDeadZoneAndNormalizeAnalogInput(float value,
                                                          float deadZone,
                                                          float maximumAbsoluteValue)
    {
        const float absValue = fabsf(value);
        if (absValue > maximumAbsoluteValue)
        {
            // Clamp values that exceed the maximum absolute value
            value = AZ::GetClamp(value, -1.0f, 1.0f);
        }
        else if (absValue > deadZone)
        {
            // Adjust values outside the dead zone so they ramp smoothly from zero to one
            const float valueAdjustedForDeadZone = (value == absValue) ? (value - deadZone) : (value + deadZone);
            const float maxAbsValAdjustedForDeadZone = maximumAbsoluteValue - deadZone;
            value = valueAdjustedForDeadZone / maxAbsValAdjustedForDeadZone;
        }
        else
        {
            // Set values within the dead zone to zero
            value = 0.0f;
        }
        return value;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Utility function to adjust and normalize thumb-stick values so that values within the given
    //! radial dead-zone are set to zero, and values outside ramp up smoothly to a given max radius.
    //! \param[in] valueX The thumb-stick x-axis value to adjust and normalize
    //! \param[in] valueY The thumb-stick y-axis value to adjust and normalize
    //! \param[in] radialDeadZone The radial dead zone within which the values will be set to zero
    //! \param[in] maximumRadiusValue The maximum radius value that will be used to clamp the values
    //! \return The thumb-stick values adjusted and normalized using the supplied dead-zone and max
    inline AZ::Vector2 AdjustForDeadZoneAndNormalizeThumbStickInput(float valueX,
                                                                    float valueY,
                                                                    float radialDeadZone,
                                                                    float maximumRadiusValue)
    {
        AZ::Vector2 values(valueX, valueY);
        const float length = values.GetLength();
        if (length > maximumRadiusValue)
        {
            // Normalize values that exceed the maximum radius value
            values /= length;
        }
        else if (length > radialDeadZone)
        {
            // Adjust values outside the dead zone so they ramp smoothly from zero to one
            const float lengthAdjustedForDeadZone = length - radialDeadZone;
            const float maxRadiusAdjustedForDeadZone = maximumRadiusValue - radialDeadZone;
            values *= (lengthAdjustedForDeadZone / maxRadiusAdjustedForDeadZone) / length;
        }
        else
        {
            // Set values within the dead zone to zero
            values.Set(0.0f);
        }
        return values;
    }
} // namespace AzFramework
