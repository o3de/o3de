/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace RPI
    {
        class Scene;
    }

    namespace Render
    {
        void ImGuiDrawCullingDebug(bool& draw, AZ::RPI::Scene* scene);
    }
}

#include "ImGuiCullingDebug.inl"
