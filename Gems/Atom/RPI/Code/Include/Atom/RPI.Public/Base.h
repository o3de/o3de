/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/**
 * This header file is for declaring types used for RPI System classes to avoid recursive includes
 */
 
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/fixed_vector.h>

AZ_DECLARE_BUDGET(AzRender);
AZ_DECLARE_BUDGET(RPI);

namespace AZ
{
    class Matrix4x4;

    namespace RHI
    {
        class ShaderResourceGroup;
    }

    namespace RPI
    {
        using ShaderResourceGroupList = AZStd::fixed_vector<const RHI::ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;

        class View;
        using ViewPtr = AZStd::shared_ptr<View>;
        using ConstViewPtr = AZStd::shared_ptr<const View>;

        class QueryPool;
        using QueryPoolPtr = AZStd::unique_ptr<QueryPool>;

        class Scene;
        using SceneId = AZ::Uuid;
        using ScenePtr = AZStd::shared_ptr<Scene>;

        class RenderPipeline;
        using RenderPipelineId = AZ::Name;
        using RenderPipelinePtr = AZStd::shared_ptr<RenderPipeline>;

        class ViewportContext;
        using ViewportContextPtr = AZStd::shared_ptr<ViewportContext>;
        using ConstViewportContextPtr = AZStd::shared_ptr<const ViewportContext>;

        using MatrixChangedEvent = Event<const AZ::Matrix4x4&>;

        //! The name used to identify a View within in a Scene.
        //! Note that the same View could have different tags in different RenderPipelines.
        using PipelineViewTag = AZ::Name;

        // [GFX TODO][ATOM-2620] Move this to live with Name code and make it sort alphabetically. Current uses can be switched to unsorted containers.
        struct AZNameSortAscending
        {
            bool operator()(const AZ::Name& lhs, const AZ::Name& rhs) const
            {
                return AZStd::hash<Name>()(lhs) < AZStd::hash<Name>()(rhs);
            }
        };

        class FeatureProcessor;

        using FeatureProcessorId = AZ::Name;
    }   // namespace RPI
}   // namespace AZ


