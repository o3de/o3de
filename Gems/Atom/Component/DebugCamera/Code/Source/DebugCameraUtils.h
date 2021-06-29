/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    namespace Debug
    {
        void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

        float NormalizeAngle(float angle);
    } // namespace Debug
} // namespace AZ
