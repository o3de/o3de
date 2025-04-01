/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "MeshBuilderVertexAttributeLayers.h"
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ::MeshBuilder
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderVertexAttributeLayer, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerFloat, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerUInt32, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerVector2, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerVector3, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerVector4, AZ::SystemAllocator)
} // namespace AZ::MeshBuilder
