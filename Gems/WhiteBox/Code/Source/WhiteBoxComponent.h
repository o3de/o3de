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

#include "Rendering/WhiteBoxRenderData.h"
#include "WhiteBox/WhiteBoxComponentBus.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

namespace WhiteBox
{
    class RenderMeshInterface;

    //! Runtime representation of White Box.
    class WhiteBoxComponent
        : public AZ::Component
        , public WhiteBoxComponentRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(WhiteBoxComponent, "{6CFD4D82-FA68-4C18-BE67-43FC2B755B64}", AZ::Component)
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxComponent();
        WhiteBoxComponent(const WhiteBoxComponent&) = delete;
        WhiteBoxComponent& operator=(const WhiteBoxComponent&) = delete;
        WhiteBoxComponent(WhiteBoxComponent&&) = default;
        WhiteBoxComponent& operator=(WhiteBoxComponent&&) = default;
        ~WhiteBoxComponent();

        // WhiteBoxComponentRequestBus ...
        bool WhiteBoxIsVisible() const override;

        void GenerateWhiteBoxMesh(const WhiteBoxRenderData& whiteBoxRenderData);

    private:
        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        WhiteBoxRenderData m_whiteBoxRenderData; //!< Intermediate format to store White Box render data.
        AZStd::unique_ptr<RenderMeshInterface> m_renderMesh; //!< The render mesh to use for White Box rendering.
    };
} // namespace WhiteBox
