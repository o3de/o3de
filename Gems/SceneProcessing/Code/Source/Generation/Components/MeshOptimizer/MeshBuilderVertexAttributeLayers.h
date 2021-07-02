/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/typetraits/is_floating_point.h>
#include <AzCore/std/typetraits/is_integral.h>
#include "AzCore/std/numeric.h"
#include "MeshBuilderInvalidIndex.h"

namespace AZ::SceneAPI::DataTypes{ struct Color; }

namespace AZ::MeshBuilder
{
    struct MeshBuilderVertexLookup
    {
        size_t mOrgVtx = InvalidIndex;
        size_t mDuplicateNr = InvalidIndex;

        MeshBuilderVertexLookup() = default;
        MeshBuilderVertexLookup(size_t orgVtx, size_t duplicateNr)
            : mOrgVtx(orgVtx)
            , mDuplicateNr(duplicateNr)
        {}
    };

    class MeshBuilderVertexAttributeLayer
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        MeshBuilderVertexAttributeLayer(bool isScale = false, bool isDeformable = false)
            : mIsScale(isScale)
            , mDeformable(isDeformable)
        {
        }

        virtual ~MeshBuilderVertexAttributeLayer() = default;

        bool GetIsScale() const { return mIsScale; }
        bool GetIsDeformable() const { return mDeformable; }
        void SetName(AZStd::string name) { mName = AZStd::move(name); }
        const AZStd::string& GetName() const { return mName; }

        virtual size_t GetAttributeSizeInBytes() const = 0;
        virtual size_t GetNumOrgVertices() const = 0;
        virtual size_t GetNumDuplicates(size_t orgVertexNr) const = 0;
        virtual size_t CalcLayerSizeInBytes() const { return GetAttributeSizeInBytes() * CalcNumVertices(); }
        virtual size_t CalcNumVertices() const = 0;
        virtual bool CheckIfIsVertexEqual(size_t orgVtx, size_t duplicate) const = 0;
        virtual void AddVertex(size_t orgVertexNr) = 0;

    protected:
        AZStd::string mName;
        bool mIsScale;
        bool mDeformable;
    };

    template<class AttribType>
    class MeshBuilderVertexAttributeLayerT : public MeshBuilderVertexAttributeLayer
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        struct Vertex
        {
            AttribType mValue{};
            size_t mOrgVertex = InvalidIndex;
            Vertex() = default;
            Vertex(const AttribType& value, size_t orgVtx)
                : mValue(value)
                , mOrgVertex(orgVtx)
            {}
        };

        MeshBuilderVertexAttributeLayerT(size_t numOrgVerts, bool isScale = false, bool isDeformable = false)
            : MeshBuilderVertexAttributeLayer(isScale, isDeformable)
            , mVertices(numOrgVerts)
        {}

        size_t GetAttributeSizeInBytes() const override { return sizeof(AttribType); }
        size_t GetNumOrgVertices() const override { return mVertices.size(); }
        size_t GetNumDuplicates(size_t orgVertexNr) const override { return mVertices[orgVertexNr].size(); }
        size_t CalcNumVertices() const override
        {
            return AZStd::accumulate(mVertices.begin(), mVertices.end(), size_t(0), [](size_t sum, const auto& vertsPerPolygon) { return sum + vertsPerPolygon.size(); });
        }

        bool CheckIfIsVertexEqual(size_t orgVtx, size_t duplicate) const override
        {
            if constexpr (AZStd::is_integral_v<AttribType>)
            {
                return mVertices[orgVtx][duplicate].mValue == mVertexValue;
            }
            else if constexpr (AZStd::is_floating_point_v<AttribType>)
            {
                return AZ::IsClose(mVertices[orgVtx][duplicate].mValue, mVertexValue, 0.00001f);
            }
            else
            {
                return mVertices[orgVtx][duplicate].mValue.IsClose(mVertexValue, 0.00001f);
            }
        }

        void SetCurrentVertexValue(const AttribType& value) { mVertexValue = value; }
        const AttribType& GetCurrentVertexValue() const { return mVertexValue; }

        void AddVertex(size_t orgVertexNr) override
        {
            mVertices[orgVertexNr].emplace_back(mVertexValue, orgVertexNr);
        }
        void AddVertexValue(size_t orgVertexNr, const AttribType& value)
        {
            mVertices[orgVertexNr].emplace_back(value, orgVertexNr);
        }

        const AttribType& GetVertexValue(size_t orgVertexNr, size_t duplicateNr) const
        {
            return mVertices[orgVertexNr][duplicateNr].mValue;
        }

    private:
        AZStd::vector<AZStd::vector<Vertex>> mVertices;
        AttribType mVertexValue;
    };

    // some standard layer types
    using MeshBuilderVertexAttributeLayerVector2 = MeshBuilderVertexAttributeLayerT<AZ::Vector2>;
    using MeshBuilderVertexAttributeLayerVector3 = MeshBuilderVertexAttributeLayerT<AZ::Vector3>;
    using MeshBuilderVertexAttributeLayerVector4 = MeshBuilderVertexAttributeLayerT<AZ::Vector4>;
    using MeshBuilderVertexAttributeLayerUInt32 = MeshBuilderVertexAttributeLayerT<AZ::u32>;
    using MeshBuilderVertexAttributeLayerFloat = MeshBuilderVertexAttributeLayerT<float>;
} // namespace AZ::MeshBuilder
