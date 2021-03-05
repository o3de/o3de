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

#include <AzCore/Component/TransformBus.h>
#include <Rendering/WhiteBoxRenderMeshInterface.h>

namespace WhiteBox
{
    class LegacyRenderNode;

    //! A concrete implementation of RenderMeshInterface to support legacy rendering for the White Box Tool.
    class LegacyRenderMesh : public RenderMeshInterface
    {
    public:
        AZ_RTTI(LegacyRenderMesh, "{F7ACB0BF-2036-4682-92CF-FE4EF4AB771B}", RenderMeshInterface);

        LegacyRenderMesh();
        ~LegacyRenderMesh();

        // RenderMeshInterface ...
        void BuildMesh(
            const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal, AZ::EntityId entityId) override;
        void UpdateTransform(const AZ::Transform& worldFromLocal) override;
        void UpdateMaterial(const WhiteBoxMaterial& material) override;
        bool IsVisible() const override;
        void SetVisiblity(bool visibility) override;

    private:
        AZStd::unique_ptr<LegacyRenderNode> m_renderNode;
    };
} // namespace WhiteBox
