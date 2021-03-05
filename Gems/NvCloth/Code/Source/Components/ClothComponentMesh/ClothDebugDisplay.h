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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace NvCloth
{
    class ClothComponentMesh;

    //! Manages the debug display of a ClothComponentMesh.
    class ClothDebugDisplay
        : protected AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_TYPE_INFO(ClothDebugDisplay, "{306A2A30-8BB1-4D0F-9776-324CA1D90ABE}");

        ClothDebugDisplay(ClothComponentMesh* clothComponentMesh);
        ~ClothDebugDisplay();

        //! Returns true when any debug cloth information must be displayed.
        bool IsDebugDrawEnabled() const;

    protected:
        // AzFramework::EntityDebugDisplayEventBus::Handler overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        void DisplayParticles(AzFramework::DebugDisplayRequests& debugDisplay);
        void DisplayWireCloth(AzFramework::DebugDisplayRequests& debugDisplay);
        void DisplayNormals(AzFramework::DebugDisplayRequests& debugDisplay, bool showTangents);
        void DisplayColliders(AzFramework::DebugDisplayRequests& debugDisplay);
        void DisplayMotionConstraints(AzFramework::DebugDisplayRequests& debugDisplay);
        void DisplaySeparationConstraints(AzFramework::DebugDisplayRequests& debugDisplay);
        void DrawSphere(AzFramework::DebugDisplayRequests& debugDisplay, float radius, const AZ::Vector3& position, const AZ::Color& color);
        void DrawCapsule(AzFramework::DebugDisplayRequests& debugDisplay, float radius, float height, const AZ::Transform& transform, const AZ::Color& color);

        ClothComponentMesh* m_clothComponentMesh = nullptr;
    };
} // namespace NvCloth
