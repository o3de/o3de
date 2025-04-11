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
#include <Atom/RPI.Public/Configuration.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/fixed_vector.h>

ATOM_RPI_PUBLIC_API AZ_DECLARE_BUDGET(AzRender);
ATOM_RPI_PUBLIC_API AZ_DECLARE_BUDGET(RPI);

namespace AZ
{
    class Matrix4x4;

    namespace RHI
    {
        class ShaderResourceGroup;
    }

    namespace RPI
    {
        class View;
        using ViewPtr = AZStd::shared_ptr<View>;
        using ConstViewPtr = AZStd::shared_ptr<const View>;

        class ViewGroup;
        using ViewGroupPtr = AZStd::shared_ptr<ViewGroup>;
        using ConstViewGroupPtr = AZStd::shared_ptr<const ViewGroup>;

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

        //! A name tag used in a RenderPipeline to associate a View to a Pass
        //! For example, a RasterPass can have a PipelineViewTag name "MainCamera". And user can attach a RPI::View generated from a user camera to the render pipeline
        //! via RenderPipelie::SetPersistentView(const PipelineViewTag&, ViewPtr) function so the camera is used as "MainCamera" for this render pipeline.
        using PipelineViewTag = AZ::Name;

        //! A collection of unique render pipeline view tags
        using PipelineViewTags = AZStd::unordered_set<PipelineViewTag>;

        class FeatureProcessor;

        using FeatureProcessorId = AZ::Name;
    }   // namespace RPI
}   // namespace AZ


