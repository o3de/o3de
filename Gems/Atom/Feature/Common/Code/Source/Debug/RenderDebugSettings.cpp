/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <Debug/RenderDebugSettings.h>
#include <Debug/RenderDebugFeatureProcessor.h>

namespace AZ {
    namespace Render {

        RenderDebugSettings::RenderDebugSettings(RenderDebugFeatureProcessor* featureProcessor)
            : m_featureProcessor(featureProcessor)
        {
        }

        void RenderDebugSettings::OnConfigChanged()
        {
        }

        void RenderDebugSettings::Simulate(float deltaTime)
        {
            m_deltaTime = deltaTime;
        }

    } // namespace Render
} // namespace AZ
