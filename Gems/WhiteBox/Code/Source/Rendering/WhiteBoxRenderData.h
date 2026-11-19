/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxMaterial.h"

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/base.h>
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
        AZ::Vector3 m_normal = AZ::Vector3::CreateZero(); //!< Smoothed corner normal; zero uses the flat face normal.
        AZ::Vector3 m_blend = AZ::Vector3::CreateZero();  //!< Vertex-blend layer weights (x layer 2, y layer 3).
        AZ::Vector2 m_lightmapUv = AZ::Vector2::CreateZero(); //!< Render-time only: the UV1 / lightmap corner.
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
        AZ::u32 m_paintColor = 0; //!< Packed face paint override; zero inherits the material/layer color.
        //! Polygon override, or empty to inherit WhiteBoxMaterial::m_materialAsset.
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset{AZ::Data::AssetLoadBehavior::PreLoad};
        AZ::Vector4 m_color = AZ::Vector4::CreateOne(); //!< Per-face tint (per-layer colour); white = untinted.
        //! Render-time only, not stored: COLOR0 carries the corners' blend weights instead of the tint, because the face
        //! is drawn with a custom material (the built-in one is the only user of the tint).
        bool m_colorFromBlend = false;
        //! Render-time only: the corners carry lightmap UVs (else UV1 repeats the texture UVs).
        bool m_hasLightmapUv = false;
    };

    //! Builds a vector of visible faces by removing the degenerate faces from the source data
    WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& sourceData);

    //! Packs render data sets into one byte blob, so serialization is a single string rather than a
    //! prefab DOM node per triangle.
    AZStd::vector<AZ::u8> PackWhiteBoxRenderData(const AZStd::vector<const WhiteBoxRenderData*>& renderDataSets);

    //! Unpacks a blob produced by PackWhiteBoxRenderData into @p renderDataSets, in the packed order.
    //! Sets beyond the blob's contents (and every set when the blob is empty or malformed) are cleared.
    bool UnpackWhiteBoxRenderData(
        const AZStd::vector<AZ::u8>& blob, const AZStd::vector<WhiteBoxRenderData*>& renderDataSets);

} // namespace WhiteBox
