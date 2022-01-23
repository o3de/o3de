/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include "ShaderCollection.h"

namespace AZ::RPI
{
    //! A material pipeline describes a set of backing shaders, one per material render pass, needed to render
    //! material types. For example, material pipeline may describe shaders associated with the following passes:
    //! - Depth pass
    //! - Motion vector pass
    //! - Shadow pass
    //! - Forward pass
    //! The material type specifies shader function implementations for functions invoked by the shaders in the
    //! pipeline. For example, the depth pass shader in the material pipeline requires an implementation of
    //! VertexLocalToClip. Currently, the interface requirements are implicit, and failure to meet the interface
    //! requirements of a pipeline will simply cause the material type to fail to compile. A future change may
    //! provide an explicit `MaterialPipelineInterfaceAsset` to catch interface failures earlier.
    class MaterialPipelineAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(MaterialPipelineAsset, "{BC3C3993-09CB-4E65-95D2-D7EE512A1394}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(MaterialPipelineAsset, SystemAllocator, 0);

        static void Reflect(ReflectContext* context);

        const ShaderCollection& GetShaderCollection() const;

    private:
        //! The version of this MaterialPipelineAsset.
        uint32_t m_version = 1;

        Name m_name;

        ShaderCollection m_shaderCollection;
    };
} // namespace AZ::RPI
