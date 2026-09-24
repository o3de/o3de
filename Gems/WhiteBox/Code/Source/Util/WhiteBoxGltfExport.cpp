/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxGltfExport.h"
#include "Rendering/WhiteBoxRenderData.h"

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/conversions.h>
#include <array>
#include <cstring>
#include <map>

namespace WhiteBox
{
    namespace
    {
        // position xyz, normal xyz, uv, colour rgba: the full identity of a glTF vertex
        using VertexKey = std::array<float, 12>;

        struct Primitive
        {
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_material;
            std::map<VertexKey, AZ::u32> m_lookup;
            AZStd::vector<VertexKey> m_vertices;
            AZStd::vector<AZ::u32> m_indices;
        };

        // White Box is Z-up; glTF is Y-up. This is a rotation, so winding is unchanged.
        AZ::Vector3 ToYUp(const AZ::Vector3& v)
        {
            return AZ::Vector3(v.GetX(), v.GetZ(), -v.GetY());
        }

        AZ::Vector4 FaceColor(const WhiteBoxFace& face)
        {
            if (face.m_paintColor != 0)
            {
                return AZ::Vector4(
                    float(face.m_paintColor & 255) / 255.0f, float((face.m_paintColor >> 8) & 255) / 255.0f,
                    float((face.m_paintColor >> 16) & 255) / 255.0f, 1.0f);
            }
            return face.m_color;
        }

        AZStd::string Escape(const AZStd::string& text)
        {
            AZStd::string escaped;
            for (const char c : text)
            {
                if (c == '"' || c == '\\')
                {
                    escaped += '\\';
                    escaped += c;
                }
                else if (static_cast<unsigned char>(c) >= 0x20)
                {
                    escaped += c;
                }
            }
            return escaped;
        }

        AZStd::string Base64(const AZStd::vector<AZ::u8>& bytes)
        {
            static const char* const Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            AZStd::string out;
            out.reserve((bytes.size() + 2) / 3 * 4);
            for (size_t i = 0; i < bytes.size(); i += 3)
            {
                const AZ::u32 a = bytes[i];
                const AZ::u32 b = i + 1 < bytes.size() ? bytes[i + 1] : 0;
                const AZ::u32 c = i + 2 < bytes.size() ? bytes[i + 2] : 0;
                const AZ::u32 triple = (a << 16) | (b << 8) | c;
                out += Alphabet[(triple >> 18) & 63];
                out += Alphabet[(triple >> 12) & 63];
                out += i + 1 < bytes.size() ? Alphabet[(triple >> 6) & 63] : '=';
                out += i + 2 < bytes.size() ? Alphabet[triple & 63] : '=';
            }
            return out;
        }

        void Append(AZStd::vector<AZ::u8>& buffer, const void* data, const size_t size)
        {
            const auto* bytes = static_cast<const AZ::u8*>(data);
            buffer.insert(buffer.end(), bytes, bytes + size);
        }

        void Align4(AZStd::vector<AZ::u8>& buffer, const AZ::u8 fill)
        {
            while (buffer.size() % 4 != 0)
            {
                buffer.push_back(fill);
            }
        }

        AZStd::string Float(const float value)
        {
            return AZStd::string::format("%.9g", value);
        }
    } // namespace

    AZStd::vector<AZ::u8> BuildGltf(const WhiteBoxRenderData& renderData, const AZStd::string& name, const bool binary)
    {
        // One primitive per material, vertices welded where every attribute matches.
        AZStd::vector<Primitive> primitives;
        bool hasColors = false;
        for (const WhiteBoxFace& face : BuildCulledWhiteBoxFaces(renderData.m_faces))
        {
            const AZ::Data::AssetId materialId = face.m_materialAsset.GetId();
            auto primitive = AZStd::find_if(
                primitives.begin(), primitives.end(),
                [&materialId](const Primitive& candidate) { return candidate.m_material.GetId() == materialId; });
            if (primitive == primitives.end())
            {
                primitives.push_back({});
                primitives.back().m_material = face.m_materialAsset;
                primitive = primitives.end() - 1;
            }
            const AZ::Vector4 color = FaceColor(face);
            hasColors = hasColors || !color.IsClose(AZ::Vector4::CreateOne());
            for (const WhiteBoxVertex* vertex : { &face.m_v1, &face.m_v2, &face.m_v3 })
            {
                const AZ::Vector3 position = ToYUp(vertex->m_position);
                const AZ::Vector3 normal = ToYUp((vertex->m_normal.IsZero() ? face.m_normal : vertex->m_normal).GetNormalizedSafe());
                const VertexKey key{ { position.GetX(), position.GetY(), position.GetZ(), normal.GetX(), normal.GetY(), normal.GetZ(),
                                       vertex->m_uv.GetX(), vertex->m_uv.GetY(), color.GetX(), color.GetY(), color.GetZ(), color.GetW() } };
                const auto found = primitive->m_lookup.find(key);
                if (found != primitive->m_lookup.end())
                {
                    primitive->m_indices.push_back(found->second);
                    continue;
                }
                const auto index = static_cast<AZ::u32>(primitive->m_vertices.size());
                primitive->m_lookup.emplace(key, index);
                primitive->m_vertices.push_back(key);
                primitive->m_indices.push_back(index);
            }
        }

        // Every attribute gets its own tightly packed view, four-byte aligned as glTF requires.
        AZStd::vector<AZ::u8> buffer;
        AZStd::string bufferViews;
        AZStd::string accessors;
        AZStd::string meshPrimitives;
        AZStd::string materials;
        int viewCount = 0;
        const auto addView = [&](const size_t offset, const size_t length, const bool indices)
        {
            bufferViews += AZStd::string::format(
                "%s{\"buffer\":0,\"byteOffset\":%zu,\"byteLength\":%zu,\"target\":%d}", viewCount > 0 ? "," : "", offset, length,
                indices ? 34963 : 34962);
            return viewCount++;
        };
        int accessorCount = 0;
        const auto addAccessor = [&](const int view, const int componentType, const size_t count, const char* type, const AZStd::string& extra)
        {
            accessors += AZStd::string::format(
                "%s{\"bufferView\":%d,\"componentType\":%d,\"count\":%zu,\"type\":\"%s\"%s}", accessorCount > 0 ? "," : "", view,
                componentType, count, type, extra.c_str());
            return accessorCount++;
        };
        // Attribute ranges inside a VertexKey: offset, component count, glTF type.
        struct Attribute
        {
            const char* m_name;
            size_t m_first;
            size_t m_count;
            const char* m_type;
        };
        const Attribute attributes[] = {
            { "POSITION", 0, 3, "VEC3" }, { "NORMAL", 3, 3, "VEC3" }, { "TEXCOORD_0", 6, 2, "VEC2" }, { "COLOR_0", 8, 4, "VEC4" }
        };

        for (size_t p = 0; p < primitives.size(); ++p)
        {
            const Primitive& primitive = primitives[p];
            AZStd::string attributeJson;
            for (const Attribute& attribute : attributes)
            {
                if (AZStd::string_view(attribute.m_name) == "COLOR_0" && !hasColors)
                {
                    continue;
                }
                Align4(buffer, 0);
                const size_t offset = buffer.size();
                AZStd::array<float, 3> low{ { 1e30f, 1e30f, 1e30f } };
                AZStd::array<float, 3> high{ { -1e30f, -1e30f, -1e30f } };
                for (const VertexKey& vertex : primitive.m_vertices)
                {
                    Append(buffer, vertex.data() + attribute.m_first, attribute.m_count * sizeof(float));
                    for (size_t c = 0; c < 3 && attribute.m_first == 0; ++c)
                    {
                        low[c] = AZStd::min(low[c], vertex[c]);
                        high[c] = AZStd::max(high[c], vertex[c]);
                    }
                }
                const int view = addView(offset, buffer.size() - offset, false);
                // POSITION must carry its bounds.
                const AZStd::string bounds = attribute.m_first == 0
                    ? AZStd::string::format(
                          ",\"min\":[%s,%s,%s],\"max\":[%s,%s,%s]", Float(low[0]).c_str(), Float(low[1]).c_str(), Float(low[2]).c_str(),
                          Float(high[0]).c_str(), Float(high[1]).c_str(), Float(high[2]).c_str())
                    : AZStd::string{};
                const int accessor = addAccessor(view, 5126, primitive.m_vertices.size(), attribute.m_type, bounds);
                attributeJson += AZStd::string::format("%s\"%s\":%d", attributeJson.empty() ? "" : ",", attribute.m_name, accessor);
            }
            Align4(buffer, 0);
            const size_t indexOffset = buffer.size();
            Append(buffer, primitive.m_indices.data(), primitive.m_indices.size() * sizeof(AZ::u32));
            const int indexView = addView(indexOffset, buffer.size() - indexOffset, true);
            const int indexAccessor = addAccessor(indexView, 5125, primitive.m_indices.size(), "SCALAR", {});
            meshPrimitives += AZStd::string::format(
                "%s{\"attributes\":{%s},\"indices\":%d,\"material\":%zu,\"mode\":4}", p > 0 ? "," : "", attributeJson.c_str(),
                indexAccessor, p);

            // The default material carries the entity tint; overrides are named after their asset so they can be relinked.
            const bool isDefault = !primitive.m_material.GetId().IsValid();
            const AZ::Vector3 tint = isDefault ? renderData.m_material.m_tint : AZ::Vector3::CreateOne();
            const AZStd::string hint = primitive.m_material.GetHint();
            const AZStd::string materialName = isDefault ? AZStd::string("WhiteBoxDefault") : (hint.empty() ? primitive.m_material.GetId().ToString<AZStd::string>() : hint);
            materials += AZStd::string::format(
                "%s{\"name\":\"%s\",\"pbrMetallicRoughness\":{\"baseColorFactor\":[%s,%s,%s,1],\"metallicFactor\":0,\"roughnessFactor\":1}}",
                p > 0 ? "," : "", Escape(materialName).c_str(), Float(tint.GetX()).c_str(), Float(tint.GetY()).c_str(),
                Float(tint.GetZ()).c_str());
        }
        Align4(buffer, 0);

        const AZStd::string uri = binary ? AZStd::string{}
                                         : AZStd::string::format(",\"uri\":\"data:application/octet-stream;base64,%s\"", Base64(buffer).c_str());
        AZStd::string json = AZStd::string::format(
            "{\"asset\":{\"version\":\"2.0\",\"generator\":\"O3DE White Box\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],"
            "\"nodes\":[{\"name\":\"%s\",\"mesh\":0}],\"meshes\":[{\"name\":\"%s\",\"primitives\":[%s]}],\"materials\":[%s],"
            "\"accessors\":[%s],\"bufferViews\":[%s],\"buffers\":[{\"byteLength\":%zu%s}]}",
            Escape(name).c_str(), Escape(name).c_str(), meshPrimitives.c_str(), materials.c_str(), accessors.c_str(), bufferViews.c_str(),
            buffer.size(), uri.c_str());

        AZStd::vector<AZ::u8> out;
        if (!binary)
        {
            out.assign(json.begin(), json.end());
            return out;
        }
        // GLB: 12-byte header, then a space-padded JSON chunk and a zero-padded BIN chunk.
        AZStd::vector<AZ::u8> jsonChunk(json.begin(), json.end());
        Align4(jsonChunk, ' ');
        const auto write32 = [&out](const AZ::u32 value) { Append(out, &value, sizeof(value)); };
        write32(0x46546C67); // "glTF"
        write32(2);
        write32(static_cast<AZ::u32>(12 + 8 + jsonChunk.size() + 8 + buffer.size()));
        write32(static_cast<AZ::u32>(jsonChunk.size()));
        write32(0x4E4F534A); // "JSON"
        out.insert(out.end(), jsonChunk.begin(), jsonChunk.end());
        write32(static_cast<AZ::u32>(buffer.size()));
        write32(0x004E4942); // "BIN\0"
        out.insert(out.end(), buffer.begin(), buffer.end());
        return out;
    }

    bool SaveToGltf(const WhiteBoxRenderData& renderData, const AZStd::string& name, const AZStd::string& filePath, AZStd::string& error)
    {
        AZStd::string lower = filePath;
        AZStd::to_lower(lower.begin(), lower.end());
        const bool binary = lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".glb") == 0;
        const AZStd::vector<AZ::u8> bytes = BuildGltf(renderData, name, binary);

        AZ::IO::SystemFile file;
        if (!file.Open(
                filePath.c_str(),
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            error = AZStd::string::format("Could not open %s for writing.", filePath.c_str());
            return false;
        }
        const bool written = file.Write(bytes.data(), bytes.size()) == bytes.size();
        file.Close();
        if (!written)
        {
            error = AZStd::string::format("Could not write %s.", filePath.c_str());
        }
        return written;
    }
} // namespace WhiteBox
