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

#include "WhiteBoxMaterial.h"

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
}

namespace WhiteBox
{
    struct WhiteBoxVertex;
    struct WhiteBoxFace;
    using WhiteBoxFaces = AZStd::vector<WhiteBoxFace>;

    struct WhiteBoxRenderData
    {
        AZ_TYPE_INFO(WhiteBoxRenderData, "{7B46EB9E-0CDF-492C-B015-240D8AB74A37}");
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxRenderData() = default;
        ~WhiteBoxRenderData() = default;

        WhiteBoxFaces m_faces;
        WhiteBoxMaterial m_material;
    };

    //! Vertex layout for WhiteBox faces
    struct WhiteBoxVertex
    {
        AZ_TYPE_INFO(WhiteBoxVertex, "{617FFD68-3528-4627-92C6-4CC7ACCBD615}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Vector3 m_position;
        AZ::Vector2 m_uv;
    };

    //! Triangle primitive with face normals
    struct WhiteBoxFace
    {
        AZ_TYPE_INFO(WhiteBoxFace, "{31293BF0-5789-489B-882A-119AC1797F9E}");
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxVertex m_v1;
        WhiteBoxVertex m_v2;
        WhiteBoxVertex m_v3;
        AZ::Vector3 m_normal;
    };

    //! Builds a vector of visible faces by removing the degenerate faces from the source data
    WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& sourceData);

} // namespace WhiteBox
