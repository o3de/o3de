/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>

namespace AZ::Render
{
    // forward declaration
    class ModelDataInstanceInterface;

    //! This feature processor manages Deferred DrawPackages for a Scene
    class DeferredMaterialFeatureProcessorInterface : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AZ::Render::DeferredMaterialFeatureProcessorInterface, "{27B1C9E5-99D9-4DEC-AE66-5F0131B20BE3}", RPI::FeatureProcessor);

        using ModelId = AZ::Uuid;
        //! Create a deferred draw-item for the referenced material types, if they don't exist yet
        virtual void AddModel(const ModelId& uuid, ModelDataInstanceInterface* meshHandle, const Data::Instance<RPI::Model>& model) = 0;

        //! Removes a mesh and potentially the draw-item for the material type.
        virtual void RemoveModel(const ModelId& uuid) = 0;

        //! A buffer with the DrawPacketId for the given deferred DrawListTag.
        //! This buffer contains one entry for every mesh in the scene, with the id of the deferred fullscreen-DrawItem that is
        //! responsible for that mesh. The buffer is kept in sync with the MeshInfoBuffer, and can be indexed using the meshInfoIndex.
        virtual AZ::Data::Instance<AZ::RPI::Buffer> GetDrawPacketIdBuffer(const RHI::DrawListTag& drawListTag) = 0;

        virtual bool IsEnabled() = 0;
    };
} // namespace AZ::Render
