/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxRenderData.h"

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/typetraits/is_trivially_copyable.h>
#include <cstring>

namespace WhiteBox
{
    void WhiteBoxRenderData::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxFace::Reflect(context);
        WhiteBoxMaterial::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxRenderData>()
                ->Version(2)
                ->Field("Faces", &WhiteBoxRenderData::m_faces)
                ->Field("Material", &WhiteBoxRenderData::m_material);
        }
    }

    void WhiteBoxVertex::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxVertex>()
                ->Version(1)
                ->Field("Position", &WhiteBoxVertex::m_position)
                ->Field("UV", &WhiteBoxVertex::m_uv);
        }
    }

    void WhiteBoxFace::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxVertex::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxFace>()
                ->Version(3)
                ->Field("PaintColor", &WhiteBoxFace::m_paintColor)
                ->Field("Vertex1", &WhiteBoxFace::m_v1)
                ->Field("Vertex2", &WhiteBoxFace::m_v2)
                ->Field("Vertex3", &WhiteBoxFace::m_v3)
                ->Field("Normal", &WhiteBoxFace::m_normal)
                ->Field("Color", &WhiteBoxFace::m_color)
                ->Field("MaterialAsset", &WhiteBoxFace::m_materialAsset);
        }
    }

    WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& inFaces)
    {
        // actual face count can be less than the original face count if any degenerate
        // faces are detected and removed
        const size_t inFaceCount = inFaces.size();
        size_t outFaceCount = 0;

        WhiteBoxFaces outFaces;

        // resize to the worst case number of faces
        outFaces.resize(inFaceCount);

        for (size_t inFaceIndex = 0; inFaceIndex < inFaceCount; ++inFaceIndex)
        {
            const WhiteBoxFace face = inFaces[inFaceIndex];
            const AZ::Vector3& vertex1 = face.m_v1.m_position;
            const AZ::Vector3& vertex2 = face.m_v2.m_position;
            const AZ::Vector3& vertex3 = face.m_v3.m_position;

            const auto areaSquared = 0.5f * ((vertex2 - vertex1).Cross(vertex3 - vertex1)).GetLengthSq();

            // only copy non-degenerate triangles (area is less than zero)
            if (areaSquared > DegenerateTriangleAreaSquareEpsilon)
            {
                outFaces[outFaceCount] = face;
                outFaceCount++;
            }
        }

        // now that we know the number of faces we need we can safely resize the vector without any
        // additional allocations as the number of faces will be equal to or less than the original size
        outFaces.resize(outFaceCount);

        return outFaces;
    }
} // namespace WhiteBox

namespace
{
    constexpr AZ::u32 RenderDataBlobMagic = 0x44524257; // 'WBRD'
    constexpr AZ::u32 RenderDataBlobVersion = 1;
    constexpr AZ::u32 NoMaterialIndex = 0xFFFFFFFFu;

    // Per-face flag bits. Everything they gate is defaulted on the vast majority of faces, so leaving
    // those fields out of the stream keeps a plain face down to its 27 geometry floats plus this byte.
    enum FaceFlags : AZ::u8
    {
        FaceFlag_Color = 1 << 0,
        FaceFlag_PaintColor = 1 << 1,
        FaceFlag_MaterialAsset = 1 << 2,
    };

    template<typename T>
    void BlobWrite(AZStd::vector<AZ::u8>& blob, const T& value)
    {
        static_assert(AZStd::is_trivially_copyable_v<T>, "BlobWrite requires a trivially copyable type");
        const auto* bytes = reinterpret_cast<const AZ::u8*>(&value);
        blob.insert(blob.end(), bytes, bytes + sizeof(T));
    }

    template<typename T>
    bool BlobRead(const AZStd::vector<AZ::u8>& blob, size_t& offset, T& value)
    {
        static_assert(AZStd::is_trivially_copyable_v<T>, "BlobRead requires a trivially copyable type");
        if (offset + sizeof(T) > blob.size())
        {
            return false;
        }
        std::memcpy(&value, blob.data() + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }

    // AZ::Vector2/3/4 are SIMD backed (padded, and not bitwise copyable on every platform), so their
    // components go through the stream individually rather than as a raw struct copy.
    void BlobWriteVector2(AZStd::vector<AZ::u8>& blob, const AZ::Vector2& v)
    {
        BlobWrite(blob, v.GetX());
        BlobWrite(blob, v.GetY());
    }

    void BlobWriteVector3(AZStd::vector<AZ::u8>& blob, const AZ::Vector3& v)
    {
        BlobWrite(blob, v.GetX());
        BlobWrite(blob, v.GetY());
        BlobWrite(blob, v.GetZ());
    }

    void BlobWriteVector4(AZStd::vector<AZ::u8>& blob, const AZ::Vector4& v)
    {
        BlobWrite(blob, v.GetX());
        BlobWrite(blob, v.GetY());
        BlobWrite(blob, v.GetZ());
        BlobWrite(blob, v.GetW());
    }

    bool BlobReadVector2(const AZStd::vector<AZ::u8>& blob, size_t& offset, AZ::Vector2& v)
    {
        float x, y;
        if (!BlobRead(blob, offset, x) || !BlobRead(blob, offset, y))
        {
            return false;
        }
        v = AZ::Vector2(x, y);
        return true;
    }

    bool BlobReadVector3(const AZStd::vector<AZ::u8>& blob, size_t& offset, AZ::Vector3& v)
    {
        float x, y, z;
        if (!BlobRead(blob, offset, x) || !BlobRead(blob, offset, y) || !BlobRead(blob, offset, z))
        {
            return false;
        }
        v = AZ::Vector3(x, y, z);
        return true;
    }

    bool BlobReadVector4(const AZStd::vector<AZ::u8>& blob, size_t& offset, AZ::Vector4& v)
    {
        float x, y, z, w;
        if (!BlobRead(blob, offset, x) || !BlobRead(blob, offset, y) || !BlobRead(blob, offset, z) ||
            !BlobRead(blob, offset, w))
        {
            return false;
        }
        v = AZ::Vector4(x, y, z, w);
        return true;
    }

    void BlobWriteString(AZStd::vector<AZ::u8>& blob, const AZStd::string& text)
    {
        BlobWrite(blob, aznumeric_cast<AZ::u32>(text.size()));
        blob.insert(blob.end(), text.begin(), text.end());
    }

    bool BlobReadString(const AZStd::vector<AZ::u8>& blob, size_t& offset, AZStd::string& text)
    {
        AZ::u32 length = 0;
        if (!BlobRead(blob, offset, length) || offset + length > blob.size())
        {
            return false;
        }
        text.assign(reinterpret_cast<const char*>(blob.data() + offset), length);
        offset += length;
        return true;
    }

    // Faces reference materials by table index: a mesh has at most a handful of distinct material
    // overrides, so the ids (and their hints) are written once instead of once per triangle.
    struct MaterialTable
    {
        AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>> m_assets;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::u32> m_lookup;

        AZ::u32 Add(const AZ::Data::Asset<AZ::RPI::MaterialAsset>& asset)
        {
            if (!asset.GetId().IsValid())
            {
                return NoMaterialIndex;
            }
            const auto inserted = m_lookup.try_emplace(asset.GetId(), aznumeric_cast<AZ::u32>(m_assets.size()));
            if (inserted.second)
            {
                m_assets.push_back(asset);
            }
            return inserted.first->second;
        }
    };

    void BlobWriteAssetTable(AZStd::vector<AZ::u8>& blob, const MaterialTable& table)
    {
        BlobWrite(blob, aznumeric_cast<AZ::u32>(table.m_assets.size()));
        for (const auto& asset : table.m_assets)
        {
            const AZ::Data::AssetId assetId = asset.GetId();
            blob.insert(blob.end(),
                reinterpret_cast<const AZ::u8*>(assetId.m_guid.begin()),
                reinterpret_cast<const AZ::u8*>(assetId.m_guid.end()));
            BlobWrite(blob, assetId.m_subId);
            BlobWriteString(blob, AZStd::string(asset.GetHint().c_str()));
        }
    }

    bool BlobReadAssetTable(
        const AZStd::vector<AZ::u8>& blob, size_t& offset,
        AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>>& assets)
    {
        AZ::u32 count = 0;
        if (!BlobRead(blob, offset, count))
        {
            return false;
        }
        assets.clear();
        assets.reserve(count);
        for (AZ::u32 i = 0; i < count; ++i)
        {
            AZ::Data::AssetId assetId;
            const size_t guidSize = assetId.m_guid.size();
            if (offset + guidSize > blob.size())
            {
                return false;
            }
            std::memcpy(assetId.m_guid.begin(), blob.data() + offset, guidSize);
            offset += guidSize;
            if (!BlobRead(blob, offset, assetId.m_subId))
            {
                return false;
            }
            AZStd::string hint;
            if (!BlobReadString(blob, offset, hint))
            {
                return false;
            }
            AZ::Data::Asset<AZ::RPI::MaterialAsset> asset(assetId, azrtti_typeid<AZ::RPI::MaterialAsset>(), hint);
            asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
            assets.push_back(AZStd::move(asset));
        }
        return true;
    }

    void BlobWriteMaterial(AZStd::vector<AZ::u8>& blob, const WhiteBox::WhiteBoxMaterial& material)
    {
        BlobWriteVector3(blob, material.m_tint);
        BlobWrite(blob, aznumeric_cast<AZ::u8>(material.m_useTexture));
        BlobWrite(blob, aznumeric_cast<AZ::u8>(material.m_visible));
        BlobWrite(blob, aznumeric_cast<AZ::u8>(material.m_useVertexColor));

        MaterialTable table;
        table.Add(material.m_materialAsset);
        BlobWriteAssetTable(blob, table);
    }

    bool BlobReadMaterial(const AZStd::vector<AZ::u8>& blob, size_t& offset, WhiteBox::WhiteBoxMaterial& material)
    {
        AZ::u8 useTexture = 0, visible = 0, useVertexColor = 0;
        if (!BlobReadVector3(blob, offset, material.m_tint) || !BlobRead(blob, offset, useTexture) ||
            !BlobRead(blob, offset, visible) || !BlobRead(blob, offset, useVertexColor))
        {
            return false;
        }
        material.m_useTexture = useTexture != 0;
        material.m_visible = visible != 0;
        material.m_useVertexColor = useVertexColor != 0;

        AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>> assets;
        if (!BlobReadAssetTable(blob, offset, assets))
        {
            return false;
        }
        material.m_materialAsset = assets.empty()
            ? AZ::Data::Asset<AZ::RPI::MaterialAsset>{AZ::Data::AssetLoadBehavior::PreLoad}
            : assets.front();
        return true;
    }
} // namespace

namespace WhiteBox
{
    AZStd::vector<AZ::u8> PackWhiteBoxRenderData(const AZStd::vector<const WhiteBoxRenderData*>& renderDataSets)
    {
        AZStd::vector<AZ::u8> blob;

        size_t totalFaces = 0;
        for (const WhiteBoxRenderData* renderData : renderDataSets)
        {
            totalFaces += renderData ? renderData->m_faces.size() : 0;
        }
        if (totalFaces == 0)
        {
            return blob; // nothing baked - an empty blob round-trips to empty sets
        }
        // 109 bytes covers a face with no material override and default colours; the reserve only has
        // to be close, a face that does carry them simply grows the vector once more.
        blob.reserve(64 + totalFaces * 109);

        BlobWrite(blob, RenderDataBlobMagic);
        BlobWrite(blob, RenderDataBlobVersion);
        BlobWrite(blob, aznumeric_cast<AZ::u32>(renderDataSets.size()));

        const WhiteBoxRenderData empty;
        for (const WhiteBoxRenderData* renderData : renderDataSets)
        {
            const WhiteBoxRenderData& set = renderData ? *renderData : empty;

            BlobWriteMaterial(blob, set.m_material);

            MaterialTable table;
            AZStd::vector<AZ::u32> faceMaterials;
            faceMaterials.reserve(set.m_faces.size());
            for (const WhiteBoxFace& face : set.m_faces)
            {
                faceMaterials.push_back(table.Add(face.m_materialAsset));
            }
            BlobWriteAssetTable(blob, table);

            BlobWrite(blob, aznumeric_cast<AZ::u32>(set.m_faces.size()));
            for (size_t faceIndex = 0; faceIndex < set.m_faces.size(); ++faceIndex)
            {
                const WhiteBoxFace& face = set.m_faces[faceIndex];

                AZ::u8 flags = 0;
                if (!face.m_color.IsClose(AZ::Vector4::CreateOne()))
                {
                    flags |= FaceFlag_Color;
                }
                if (face.m_paintColor != 0)
                {
                    flags |= FaceFlag_PaintColor;
                }
                if (faceMaterials[faceIndex] != NoMaterialIndex)
                {
                    flags |= FaceFlag_MaterialAsset;
                }
                BlobWrite(blob, flags);

                BlobWriteVector3(blob, face.m_v1.m_position);
                BlobWriteVector2(blob, face.m_v1.m_uv);
                BlobWriteVector3(blob, face.m_v2.m_position);
                BlobWriteVector2(blob, face.m_v2.m_uv);
                BlobWriteVector3(blob, face.m_v3.m_position);
                BlobWriteVector2(blob, face.m_v3.m_uv);
                BlobWriteVector3(blob, face.m_normal);

                if (flags & FaceFlag_Color)
                {
                    BlobWriteVector4(blob, face.m_color);
                }
                if (flags & FaceFlag_PaintColor)
                {
                    BlobWrite(blob, face.m_paintColor);
                }
                if (flags & FaceFlag_MaterialAsset)
                {
                    BlobWrite(blob, faceMaterials[faceIndex]);
                }
            }
        }

        return blob;
    }

    bool UnpackWhiteBoxRenderData(
        const AZStd::vector<AZ::u8>& blob, const AZStd::vector<WhiteBoxRenderData*>& renderDataSets)
    {
        const auto clearAll = [&renderDataSets]()
        {
            for (WhiteBoxRenderData* renderData : renderDataSets)
            {
                if (renderData)
                {
                    *renderData = WhiteBoxRenderData{};
                }
            }
        };

        clearAll();
        if (blob.empty())
        {
            return true; // nothing was baked, which is a valid state rather than a failure
        }

        size_t offset = 0;
        AZ::u32 magic = 0, version = 0, setCount = 0;
        if (!BlobRead(blob, offset, magic) || !BlobRead(blob, offset, version) ||
            !BlobRead(blob, offset, setCount) || magic != RenderDataBlobMagic || version != RenderDataBlobVersion)
        {
            AZ_Warning("White Box", false, "Baked render data blob is not readable - it will be rebuilt.");
            return false;
        }

        for (AZ::u32 setIndex = 0; setIndex < setCount; ++setIndex)
        {
            // Sets past the end of the caller's list still have to be walked to stay in sync with the
            // stream (a newer build may have packed more of them), they just have nowhere to land.
            WhiteBoxRenderData scratch;
            WhiteBoxRenderData& set = (setIndex < renderDataSets.size() && renderDataSets[setIndex] != nullptr)
                ? *renderDataSets[setIndex]
                : scratch;

            if (!BlobReadMaterial(blob, offset, set.m_material))
            {
                clearAll();
                return false;
            }

            AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>> faceAssets;
            if (!BlobReadAssetTable(blob, offset, faceAssets))
            {
                clearAll();
                return false;
            }

            AZ::u32 faceCount = 0;
            if (!BlobRead(blob, offset, faceCount))
            {
                clearAll();
                return false;
            }
            set.m_faces.clear();
            set.m_faces.reserve(faceCount);

            for (AZ::u32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
            {
                WhiteBoxFace face;
                AZ::u8 flags = 0;
                if (!BlobRead(blob, offset, flags) || !BlobReadVector3(blob, offset, face.m_v1.m_position) ||
                    !BlobReadVector2(blob, offset, face.m_v1.m_uv) ||
                    !BlobReadVector3(blob, offset, face.m_v2.m_position) ||
                    !BlobReadVector2(blob, offset, face.m_v2.m_uv) ||
                    !BlobReadVector3(blob, offset, face.m_v3.m_position) ||
                    !BlobReadVector2(blob, offset, face.m_v3.m_uv) ||
                    !BlobReadVector3(blob, offset, face.m_normal))
                {
                    clearAll();
                    return false;
                }
                if ((flags & FaceFlag_Color) != 0 && !BlobReadVector4(blob, offset, face.m_color))
                {
                    clearAll();
                    return false;
                }
                if ((flags & FaceFlag_PaintColor) != 0 && !BlobRead(blob, offset, face.m_paintColor))
                {
                    clearAll();
                    return false;
                }
                if ((flags & FaceFlag_MaterialAsset) != 0)
                {
                    AZ::u32 materialIndex = NoMaterialIndex;
                    if (!BlobRead(blob, offset, materialIndex) || materialIndex >= faceAssets.size())
                    {
                        clearAll();
                        return false;
                    }
                    face.m_materialAsset = faceAssets[materialIndex];
                }
                set.m_faces.push_back(AZStd::move(face));
            }
        }

        return true;
    }
} // namespace WhiteBox
