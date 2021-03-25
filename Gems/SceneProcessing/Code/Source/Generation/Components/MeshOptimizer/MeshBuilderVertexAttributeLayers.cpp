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

// include the required headers
#include "MeshBuilderVertexAttributeLayers.h"
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ::MeshBuilder
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderVertexAttributeLayer, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerFloat, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerUInt32, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerVector2, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerVector3, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(MeshBuilderVertexAttributeLayerVector4, AZ::SystemAllocator, 0)
} // namespace AZ::MeshBuilder
