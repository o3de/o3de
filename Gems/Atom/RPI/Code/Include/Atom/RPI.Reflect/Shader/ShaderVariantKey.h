/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/bitset.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RHI.Reflect/Handle.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! Defines the maximum bit size of the shader variant key on the runtime.
        //! The number of bits in the shader key are configurable at compile-time by tweaking this value.
        static constexpr uint32_t ShaderVariantKeyBitCount = 128;

        //! Defines the discrete element size used when mapping the ShaderVariantKey value to the shader.
        //! This value is fixed at 32 bits (size of uint in the shader program) and should not change.
        static constexpr uint32_t ShaderElementBitSize = 32;

        //! Defines the discrete register size used when mapping the ShaderVariantKey value to the shader.
        //! This value is fixed at 128 bits (size of uint4 in the shader program) and should not change.
        //! ShaderVariantKeyBitCount can exceed this value in which case it will occupy multiple registers.
        static constexpr uint32_t ShaderRegisterBitSize = 128;

        //! This value should evaluate as 16 bytes (size of uint4 in the shader program) and should not change.
        static constexpr uint32_t ShaderRegisterByteSize = ShaderRegisterBitSize / 8;

        //! This value should evaluate as 4 (there are 4 elements per register in the shader program) and should not change.
        static constexpr uint32_t ShaderElementsPerRegister = ShaderRegisterBitSize / ShaderElementBitSize;

        static_assert(ShaderRegisterBitSize% ShaderElementBitSize == 0, "The register size must be a multiple of its elements!");
        static_assert(ShaderElementsPerRegister == 4, "A register must contain 4 elements!");
        static_assert(sizeof(uint32_t) == 4, "Please review the use of ShaderElementBitSize in the code!");

        //! A bitset of packed shader option values. Used to acquire shader variants.
        using ShaderVariantKey = AZStd::bitset<ShaderVariantKeyBitCount>;

        struct ATOM_RPI_REFLECT_API ShaderVariantId final
        {
            AZ_RTTI(ShaderVariantId, "{27B1FEC2-8C8A-47D7-A034-6609FA092B34}");
            AZ_CLASS_ALLOCATOR(ShaderVariantId, AZ::SystemAllocator);

            static void Reflect(ReflectContext* context);

            bool IsEmpty() const;
            ShaderVariantId& Reset();

            bool operator==(const ShaderVariantId& other) const;
            bool operator!=(const ShaderVariantId& other) const;
            bool operator< (const ShaderVariantId& other) const;
            bool operator> (const ShaderVariantId& other) const;
            bool operator<=(const ShaderVariantId& other) const;
            bool operator>=(const ShaderVariantId& other) const;

            ShaderVariantKey m_key;
            ShaderVariantKey m_mask;
        };

        //! ShaderVariantStableId is managed by the user or an external tool. Its purpose is to assign
        //! a stable identifier that we can use to efficiently identify the variants that have changed
        //! each time a .shadervariantlist file changes. Imagine having 10K variants declared in one of these files,
        //! then by virtue of this StableId We can quickly diff and figure out which variant was added or modified and recompile
        //! only that variant instead of recompiling 10K variants.
        //! Also the ShaderVariantStableId is used to make the Asset SubId of ShaderVariantAssets. See ShaderVariantAsset::GetAssetSubId()
        using ShaderVariantStableId = RHI::Handle<uint32_t, ShaderVariantId>;

        static constexpr ShaderVariantStableId RootShaderVariantStableId{ 0 };

        //! Suggests the shader binary which best fits a requested variant
        //! The suggested binary is given as an index in the asset where the search was performed
        struct ATOM_RPI_REFLECT_API ShaderVariantSearchResult
        {
        public:
            //! ShaderVariantSearchResult constructor.
            //! @param index               The StableId of the variant found in the shader variant tree asset.
            //! @param dynamicOptionCount  The number of dynamic (non-baked) options in this shader variant
            ShaderVariantSearchResult(ShaderVariantStableId stableId, uint32_t dynamicOptionCount);

            //! Returns the StableId of the variant found within the shader variant tree asset.
            //! It always returns a valid shader, but not always fully baked (static). Also check IsFullyBaked().
            ShaderVariantStableId GetStableId() const;

            //! True if the search returned the root shader variant.
            bool IsRoot() const;

            //! True if the search found a fully baked (static) variant, false if the variant contains dynamic branches
            //! If the shader is not fully baked, the ShaderVariantKeyFallbackValue must be correctly set when drawing
            bool IsFullyBaked() const;

            uint32_t GetDynamicOptionCount() const;

        private:
            ShaderVariantStableId m_shaderVariantStableId;
            uint32_t m_dynamicOptionCount;
        };

        //! Comparator which performs a less-than operation on two shader keys. Used to sort a container of keys.
        struct ATOM_RPI_REFLECT_API ShaderVariantKeyComparator
        {
        public:
            static int Compare(const ShaderVariantKey& lhs, const ShaderVariantKey& rhs);
            bool operator () (const ShaderVariantKey& lhs, const ShaderVariantKey& rhs) const;

        private:
            static int CompareSmallKey(const ShaderVariantKey& lhs, const ShaderVariantKey& rhs);
            static int CompareLargeKey(const ShaderVariantKey& lhs, const ShaderVariantKey& rhs);
        };

        struct ATOM_RPI_REFLECT_API ShaderVariantIdComparator
        {
        public:
            static int Compare(const ShaderVariantId& lhs, const ShaderVariantId& rhs);
            bool operator () (const ShaderVariantId& lhs, const ShaderVariantId& rhs) const;
        };
    } // namespace RPI
} // namespace AZ
