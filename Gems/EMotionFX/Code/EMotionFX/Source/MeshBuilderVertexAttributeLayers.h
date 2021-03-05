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

// include required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/Endian.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    struct EMFX_API MeshBuilderVertexLookup
    {
        AZ_CLASS_ALLOCATOR_DECL

        uint32  mOrgVtx;
        uint32  mDuplicateNr;

        MCORE_INLINE MeshBuilderVertexLookup()
            : mOrgVtx(MCORE_INVALIDINDEX32)
            , mDuplicateNr(MCORE_INVALIDINDEX32) {}
        MCORE_INLINE MeshBuilderVertexLookup(uint32 orgVtx, uint32 duplicateNr)
            : mOrgVtx(orgVtx)
            , mDuplicateNr(duplicateNr) {}
    };


    class EMFX_API MeshBuilderVertexAttributeLayer
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        uint32  GetTypeID() const           { return mLayerTypeID; };
        bool    GetIsScale() const          { return mIsScale; }
        bool    GetIsDeformable() const     { return mDeformable; }

        void SetName(const char* name)              { mName = name; }
        const char* GetName() const                 { return mName.c_str(); }
        const AZStd::string& GetNameString() const  { return mName; }

        virtual uint32 GetAttributeSizeInBytes() const = 0;
        virtual uint32 GetNumOrgVertices() const = 0;
        virtual uint32 GetNumDuplicates(uint32 orgVertexNr) const = 0;
        virtual uint32 CalcLayerSizeInBytes() const     { return GetAttributeSizeInBytes() * CalcNumVertices(); }
        virtual uint32 CalcNumVertices() const = 0;
        virtual bool CheckIfIsVertexEqual(uint32 orgVtx, uint32 duplicate) const = 0;
        virtual void SetCurrentVertexValue(const void* value) = 0;
        virtual void AddVertex(uint32 orgVertexNr) = 0;
        virtual void AddVertexValue(uint32 orgVertexNr, void* value) = 0;
        virtual void OptimizeMemoryUsage() {};
        virtual const void* GetVertexValue(uint32 orgVertexNr, uint32 duplicateNr) const = 0;
        virtual void ConvertEndian(uint32 orgVtx, uint32 duplicate, MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian) = 0;
        virtual void ConvertEndian(MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian) = 0;

    protected:
        uint32          mLayerTypeID;
        AZStd::string   mName;
        bool            mIsScale;
        bool            mDeformable;

        MeshBuilderVertexAttributeLayer(uint32 numOrgVerts, uint32 layerTypeID, bool isScale = false, bool isDeformable = false)
            : mLayerTypeID(layerTypeID)
            , mIsScale(isScale)
            , mDeformable(isDeformable) { MCORE_UNUSED(numOrgVerts); }
        virtual ~MeshBuilderVertexAttributeLayer() {}
    };


#define DECLARE_MESHBUILDERATTRIBUTELAYER(ClassName, AttribType)                                                                                           \
    class EMFX_API ClassName                                                                                                                               \
        : public MeshBuilderVertexAttributeLayer                                                                                                           \
    {                                                                                                                                                      \
        AZ_CLASS_ALLOCATOR(ClassName, EMotionFX::MeshAllocator, 0);                                                                                        \
    public:                                                                                                                                                \
        struct EMFX_API Vertex                                                                                                                             \
        {                                                                                                                                                  \
            MCORE_MEMORYOBJECTCATEGORY(ClassName::Vertex, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER);                      \
            AttribType  mValue;                                                                                                                            \
            uint32      mOrgVertex;                                                                                                                        \
            MCORE_INLINE Vertex()                                                                                                                          \
                : mOrgVertex(MCORE_INVALIDINDEX32) {}                                                                                                      \
            MCORE_INLINE Vertex(const AttribType&value, uint32 orgVtx)                                                                                     \
                : mValue(value)                                                                                                                            \
                , mOrgVertex(orgVtx) {}                                                                                                                    \
        };                                                                                                                                                 \
                                                                                                                                                           \
        static ClassName* Create(uint32 numOrgVerts, uint32 layerTypeID, bool isScale = false, bool isDeformable = false);                                 \
                                                                                                                                                           \
        MCORE_INLINE uint32 GetAttributeSizeInBytes() const override                                        { return sizeof(AttribType); }                 \
        MCORE_INLINE uint32 GetNumOrgVertices() const override                                              { return mVertices.GetLength(); }              \
        MCORE_INLINE uint32 GetNumDuplicates(uint32 orgVertexNr) const override                             { return mVertices[orgVertexNr].GetLength(); } \
        uint32 CalcNumVertices() const override                                                                                                            \
        {                                                                                                                                                  \
            uint32 totalVerts = 0;                                                                                                                         \
            const uint32 numOrgVerts = mVertices.GetLength();                                                                                              \
            for (uint32 i = 0; i < numOrgVerts; ++i) {                                                                                                     \
                totalVerts += mVertices[i].GetLength(); }                                                                                                  \
            return totalVerts;                                                                                                                             \
        }                                                                                                                                                  \
                                                                                                                                                           \
        bool CheckIfIsVertexEqual(uint32 orgVtx, uint32 duplicate) const override;                                                                         \
                                                                                                                                                           \
        MCORE_INLINE void SetCurrentVertexValue(const void* value) override       { mVertexValue = *((AttribType*)value); }                                \
        MCORE_INLINE AttribType GetCurrentVertexValue() const                     { return mVertexValue; }                                                 \
                                                                                                                                                           \
        MCORE_INLINE void AddVertex(uint32 orgVertexNr) override                                                                                           \
        {                                                                                                                                                  \
            mVertices[orgVertexNr].Add(Vertex(mVertexValue, orgVertexNr));                                                                                 \
        }                                                                                                                                                  \
        MCORE_INLINE void AddVertexValue(uint32 orgVertexNr, void* value) override                                                                         \
        {                                                                                                                                                  \
            mVertices[orgVertexNr].Add(Vertex(*((AttribType*)value), orgVertexNr));                                                                        \
        }                                                                                                                                                  \
                                                                                                                                                           \
        MCORE_INLINE void OptimizeMemoryUsage() override                                                                                                   \
        {                                                                                                                                                  \
        }                                                                                                                                                  \
                                                                                                                                                           \
        MCORE_INLINE const void* GetVertexValue(uint32 orgVertexNr, uint32 duplicateNr) const override                                                     \
        {                                                                                                                                                  \
            return &mVertices[orgVertexNr][duplicateNr].mValue;                                                                                            \
        }                                                                                                                                                  \
                                                                                                                                                           \
        void ConvertEndian(MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian) override                                        \
        {                                                                                                                                                  \
            const uint32 numOrgVerts = mVertices.GetLength();                                                                                              \
            for (uint32 i = 0; i < numOrgVerts; ++i)                                                                                                       \
            {                                                                                                                                              \
                const uint32 numDups = mVertices[i].GetLength();                                                                                           \
                for (uint32 d = 0; d < numDups; ++d) {                                                                                                     \
                    ConvertEndian(i, d, fromEndian, targetEndian); }                                                                                       \
            }                                                                                                                                              \
        }                                                                                                                                                  \
                                                                                                                                                           \
        void ConvertEndian(uint32 orgVtx, uint32 duplicate, MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian) override;      \
                                                                                                                                                           \
    private:                                                                                                                                               \
        MCore::Array< MCore::Array<Vertex> > mVertices;                                                                                                    \
        AttribType mVertexValue;                                                                                                                           \
        ClassName(uint32 numOrgVerts, uint32 layerTypeID, bool isScale = false, bool isDeformable = false)                                                 \
            : MeshBuilderVertexAttributeLayer(numOrgVerts, layerTypeID, isScale, isDeformable)                                                             \
        {                                                                                                                                                  \
            mVertices.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER);                                                                \
            mVertices.Resize(numOrgVerts);                                                                                                                 \
            mLayerTypeID = layerTypeID;                                                                                                                    \
            for (uint32 i = 0; i < numOrgVerts; ++i) {                                                                                                     \
                mVertices[i].SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER); }                                                       \
        }                                                                                                                                                  \
                                                                                                                                                           \
        ~ClassName()                                                                                                                                       \
        {                                                                                                                                                  \
            mVertices.Clear();                                                                                                                             \
        }                                                                                                                                                  \
    };


    // some standard layer types
    DECLARE_MESHBUILDERATTRIBUTELAYER(MeshBuilderVertexAttributeLayerVector2,      AZ::Vector2)
    DECLARE_MESHBUILDERATTRIBUTELAYER(MeshBuilderVertexAttributeLayerVector3,      AZ::Vector3)
    DECLARE_MESHBUILDERATTRIBUTELAYER(MeshBuilderVertexAttributeLayerVector4,      AZ::Vector4)
    DECLARE_MESHBUILDERATTRIBUTELAYER(MeshBuilderVertexAttributeLayerUInt32,       uint32)
    DECLARE_MESHBUILDERATTRIBUTELAYER(MeshBuilderVertexAttributeLayerFloat,        float)
}   // namespace EMotionFX
