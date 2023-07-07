/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ConstantsLayout.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::Serialize
{
    template<class T, bool U, bool A>
    struct InstanceFactory;
}
namespace AZ
{
    template<typename ValueType, typename>
    struct AnyTypeInfoConcept;
}

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! ShaderResourceGroupLayout defines a set of valid shader inputs to a ShaderResourceGroup.
    //!
    //! Each 'shader input' adheres to the following type:
    //!  - Buffer (Constant, Read, Read-Write).
    //!  - Image (Read, Read-Write)
    //!  - Sampler
    //!  - Constant
    //!
    //! Buffers, images, and samplers are collectively called 'resources'. These three types form disjoint
    //! groups. Each resource shader input has an array of resources. These arrays are flattened into a
    //! 'resource group' for each resource type. For example, if a buffer input at index '0' has two elements,
    //! and the buffer input at index '1' has three, the "buffer shader resource group" forms a list of five
    //! elements.
    //!
    //! Each shader input maps to a named definition in a shader source file. This name is retained in
    //! order to support a basic name-to-index reflection API.
    //!
    //! Constant shader inputs are treated a bit differently. Each constant input spans a range of bytes
    //! in an implicit constant buffer.
    //!
    //! To use the class, assign shader inputs using AddShaderInput, and call Finalize to
    //! complete construction of the layout. This class is intended to be built using an offline shader
    //! compiler, and serialized to / from disk.
    class ShaderResourceGroupLayout
        : public AZStd::intrusive_base
    {
    public:
        AZ_RTTI(ShaderResourceGroupLayout, "{1F92C651-9B83-4379-AB5C-5201F1B2C278}");
        AZ_CLASS_ALLOCATOR(ShaderResourceGroupLayout, SystemAllocator);
        static void Reflect(ReflectContext* context);

        static Ptr<ShaderResourceGroupLayout> Create();

        bool IsFinalized() const;

        //! Clears the layout to an empty state. The layout must be finalized prior to usage.
        void Clear();

        //! Finalizes the layout for access. Must be called prior to usage or serialization.
        //! It is not permitted to mutate the layout once Finalize is called. Clear must
        //! be called first. If the call fails, the layout is cleared back to an empty state
        //! and false is returned. Otherwise, true is returned and the layout is considered
        //! finalized.
        bool Finalize();

        void SetName(const Name& name) {  m_name = name; }
        const Name& GetName() const { return m_name; }

        //! This string will be used at runtime for both ShaderResourceGroup and ShaderResourceGroupPool to
        //! create a unique InstanceId to avoid redundant copies in memory.
        const AZStd::string& GetUniqueId() const { return m_uniqueId; }

        //! The Set function as described above.
        //! It is usually the Source azsl/azsli/srgi file where this SRG comes from.
        void SetUniqueId(const AZStd::string& uniqueId) { m_uniqueId = uniqueId; }

        //! Designates this SRG as ShaderVariantKey fallback by providing the generated
        //! shader constant name and its length in bits.
        void SetShaderVariantKeyFallback(const Name& shaderConstantName, uint32_t bitSize);

        //! Adds a static sampler to the layout. Static samplers are immutable and cannot
        //! be assigned at runtime.
        void AddStaticSampler(const ShaderInputStaticSamplerDescriptor& sampler);

        //! Adds a shader input to the shader resource group layout. The layout maintains
        //! a separate list for each type of shader input. Order in each list is preserved.
        void AddShaderInput(const ShaderInputBufferDescriptor& buffer);
        void AddShaderInput(const ShaderInputImageDescriptor& image);
        void AddShaderInput(const ShaderInputBufferUnboundedArrayDescriptor& bufferUnboundedArray);
        void AddShaderInput(const ShaderInputImageUnboundedArrayDescriptor& imageUnboundedArray);
        void AddShaderInput(const ShaderInputSamplerDescriptor& sampler);
        void AddShaderInput(const ShaderInputConstantDescriptor& constant);

        //! Assigns the shader resource group binding slot to the layout. The binding slot is
        //! defined by the shader content and dictates which logical slot to use when binding
        //! shader resource groups to command lists.
        void SetBindingSlot(uint32_t bindingSlot);

        //////////////////////////////////////////////////////////////////////////
        // The following methods are only permitted on a finalized layout.

        /// Returns the full list of static samplers descriptors declared on the layout.
        AZStd::span<const ShaderInputStaticSamplerDescriptor> GetStaticSamplers() const;

        //! Resolves an shader input name to an index for each type of shader input. To maximize performance,
        //! the name to index resolve should be done as an initialization step and the indices
        //! cached.
        ShaderInputBufferIndex    FindShaderInputBufferIndex(const Name& name) const;
        ShaderInputImageIndex     FindShaderInputImageIndex(const Name& name) const;
        ShaderInputSamplerIndex   FindShaderInputSamplerIndex(const Name& name) const;
        ShaderInputConstantIndex  FindShaderInputConstantIndex(const Name& name) const;

        ShaderInputBufferUnboundedArrayIndex FindShaderInputBufferUnboundedArrayIndex(const Name& name) const;
        ShaderInputImageUnboundedArrayIndex  FindShaderInputImageUnboundedArrayIndex(const Name& name) const;

        //! Returns the shader input associated with the requested index. It is not permitted
        //! to call this method with a null index. An assert will fire otherwise.
        const ShaderInputBufferDescriptor&    GetShaderInput(ShaderInputBufferIndex index) const;
        const ShaderInputImageDescriptor&     GetShaderInput(ShaderInputImageIndex index) const;
        const ShaderInputSamplerDescriptor&   GetShaderInput(ShaderInputSamplerIndex index) const;
        const ShaderInputConstantDescriptor&  GetShaderInput(ShaderInputConstantIndex index) const;

        const ShaderInputBufferUnboundedArrayDescriptor& GetShaderInput(ShaderInputBufferUnboundedArrayIndex index) const;
        const ShaderInputImageUnboundedArrayDescriptor& GetShaderInput(ShaderInputImageUnboundedArrayIndex index) const;

        //! Returns the full lists of each kind of shader input added to the layout. Inputs
        //! maintain their original order with respect to AddShaderInput. Each type
        //! of shader input has its own separate list.
        AZStd::span<const ShaderInputBufferDescriptor>   GetShaderInputListForBuffers() const;
        AZStd::span<const ShaderInputImageDescriptor>    GetShaderInputListForImages() const;
        AZStd::span<const ShaderInputSamplerDescriptor>  GetShaderInputListForSamplers() const;
        AZStd::span<const ShaderInputConstantDescriptor> GetShaderInputListForConstants() const;

        AZStd::span<const ShaderInputBufferUnboundedArrayDescriptor> GetShaderInputListForBufferUnboundedArrays() const;
        AZStd::span<const ShaderInputImageUnboundedArrayDescriptor>  GetShaderInputListForImageUnboundedArrays() const;

        //! Each shader input may contain multiple shader resources. The layout computes
        //! a separate shader resource group for each resource type. For example, given
        //! shader inputs 'A', 'B' and 'C':
        //!
        //!  Shader Input List:       (A)       (B)    (C)
        //!  Shader Resource Group:   [0, 1, 2] [3, 4] [5]
        //!
        //! [A, B, C] form a list of three shader inputs. But the shader resource group
        //! forms a group of six resources. The following methods provide a mapping from
        //! a shader input index to an interval of resources in the resource group.
        //!
        //! The following methods return an interval - [min, max) - into the shader resource
        //! group for that resource type.
        Interval GetGroupInterval(ShaderInputBufferIndex inputIndex) const;
        Interval GetGroupInterval(ShaderInputImageIndex inputIndex) const;
        Interval GetGroupInterval(ShaderInputSamplerIndex inputIndex) const;

        //! The interval of a constant is its byte [min, max) into the constant data.
        Interval GetConstantInterval(ShaderInputConstantIndex inputIndex) const;

        //! Returns the total size of the flat resource table for each type of resource.
        //! Note that this size is not 1-to-1 with the 'shader input list' for that type
        //! of resource, since a shader input may be an array of resources.
        //!
        //! NOTE: The resource table maps to the following types per resource:
        //!  - Buffer:   BufferView
        //!  - Image:    ImageView
        //!  - Sampler:  SamplerState
        uint32_t GetGroupSizeForBuffers() const;
        uint32_t GetGroupSizeForImages() const;
        uint32_t GetGroupSizeForBufferUnboundedArrays() const;
        uint32_t GetGroupSizeForImageUnboundedArrays() const;
        uint32_t GetGroupSizeForSamplers() const;

        //! Constants are different and live in an opaque buffer of bytes instead of a resource group.
        uint32_t GetConstantDataSize() const;

        //! Returns the binding slot allocated to this layout. Each layout occupies a logical binding slot
        //! on a command list. All shader resource groups which use the layout are bound to that slot.
        uint32_t GetBindingSlot() const;

        //! Returns the ShaderVariantKey fallback size in bits, or 0 if this SRG can't handle that function
        uint32_t GetShaderVariantKeyFallbackSize() const;

        //! Returns true if the ShaderResourceGroup has been designated as a ShaderVariantKey fallback
        bool HasShaderVariantKeyFallbackEntry() const;

        //! Returns the ShaderVariantKey fallback index, or invalid index if this SRG is not designated as fallback
        const RHI::ShaderInputConstantIndex& GetShaderVariantKeyFallbackConstantIndex() const;

        //! Returns the hash computed in Finalize.
        HashValue64 GetHash() const;

        //! Returns the constants data layout;
        const ConstantsLayout* GetConstantsLayout() const;

        //! Validates that the inputIndex is valid.
        //! Emits an assert and returns false on failure; returns true on success. If validation is disabled true is always returned.
        bool ValidateAccess(RHI::ShaderInputConstantIndex inputIndex) const;

        //! Validates that the inputIndex is valid and the arrayIndex is less than the total array size of the shader input.
        //! Emits an assert and returns false on failure; returns true on success. If validation is disabled true is always returned.
        bool ValidateAccess(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const;
        bool ValidateAccess(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const;
        bool ValidateAccess(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const;

        //! Validates that the inputIndex is valid.
        //! Emits an assert and returns false on failure; returns true on success. If validation is disabled true is always returned.
        bool ValidateAccess(RHI::ShaderInputBufferUnboundedArrayIndex inputIndex) const;
        bool ValidateAccess(RHI::ShaderInputImageUnboundedArrayIndex inputIndex) const;

    private:
        ShaderResourceGroupLayout();

        enum class ValidateFinalizeStateExpect
        {
            NotFinalized = 0,
            Finalized
        };

        bool ValidateFinalizeState(ValidateFinalizeStateExpect expect) const;

        template<typename IndexType>
        bool ValidateAccess(IndexType inputIndex, size_t inputIndexLimit, const char* inputArrayTypeName) const;

        template<typename IndexType>
        bool ValidateAccess(IndexType inputIndex, uint32_t arrayIndex, size_t inputIndexLimit, const char* inputArrayTypeName) const;

        using IdReflectionMapForBuffers = NameIdReflectionMap<ShaderInputBufferIndex>;
        using IdReflectionMapForImages = NameIdReflectionMap<ShaderInputImageIndex>;
        using IdReflectionMapForBufferUnboundedArrays = NameIdReflectionMap<ShaderInputBufferUnboundedArrayIndex>;
        using IdReflectionMapForImageUnboundedArrays = NameIdReflectionMap<ShaderInputImageUnboundedArrayIndex>;
        using IdReflectionMapForSamplers = NameIdReflectionMap<ShaderInputSamplerIndex>;

        /// Helper functions for building up data caches for a single group of shader inputs.
        template<typename NameIdReflectionMapT, typename ShaderInputDescriptorT, typename ShaderInputIndexT>
        bool FinalizeShaderInputGroup(
            const AZStd::vector<ShaderInputDescriptorT>& shaderInputDescriptors,
            AZStd::vector<Interval>& intervals,
            NameIdReflectionMapT& nameIdReflectionMap,
            uint32_t& groupSize);

        template<typename NameIdReflectionMapT, typename ShaderInputDescriptorT, typename ShaderInputIndexT>
        bool FinalizeUnboundedArrayShaderInputGroup(
            const AZStd::vector<ShaderInputDescriptorT>& shaderInputDescriptors,
            NameIdReflectionMapT& nameIdReflectionMap,
            uint32_t& groupSize);

        template <typename, typename>
        friend struct AnyTypeInfoConcept;
        template <typename, bool, bool>
        friend struct Serialize::InstanceFactory;

        //! Name of the ShaderResourceGroup as specified in the original *.azsl/*.azsli file.
        Name m_name;

        //! Usually the AZSL file of origin/definition.
        AZStd::string m_uniqueId;

        AZStd::vector<ShaderInputStaticSamplerDescriptor> m_staticSamplers;

        AZStd::vector<ShaderInputBufferDescriptor> m_inputsForBuffers;
        AZStd::vector<ShaderInputImageDescriptor> m_inputsForImages;
        AZStd::vector<ShaderInputSamplerDescriptor> m_inputsForSamplers;

        AZStd::vector<ShaderInputBufferUnboundedArrayDescriptor> m_inputsForBufferUnboundedArrays;
        AZStd::vector<ShaderInputImageUnboundedArrayDescriptor>  m_inputsForImageUnboundedArrays;

        AZStd::vector<Interval> m_intervalsForBuffers;
        AZStd::vector<Interval> m_intervalsForImages;
        AZStd::vector<Interval> m_intervalsForSamplers;

        uint32_t m_groupSizeForBuffers = 0;
        uint32_t m_groupSizeForImages = 0;
        uint32_t m_groupSizeForBufferUnboundedArrays = 0;
        uint32_t m_groupSizeForImageUnboundedArrays = 0;
        uint32_t m_groupSizeForSamplers = 0;

        uint32_t m_shaderVariantKeyFallbackSize = 0;
        RHI::ShaderInputConstantIndex m_shaderVariantKeyFallbackConstantIndex;
        /// m_shaderVariantKeyFallbackConstantId is not serialized and is only used for
        /// resolving the index during the Finalize() step
        Name m_shaderVariantKeyFallbackConstantId;

        /// Reflection information for each kind of shader input, stored in
        /// sorted vectors. Binary search is used to find entries.
        IdReflectionMapForBuffers m_idReflectionForBuffers;
        IdReflectionMapForImages m_idReflectionForImages;
        IdReflectionMapForBufferUnboundedArrays m_idReflectionForBufferUnboundedArrays;
        IdReflectionMapForImageUnboundedArrays m_idReflectionForImageUnboundedArrays;
        IdReflectionMapForSamplers m_idReflectionForSamplers;

        /// The logical binding slot used by all groups in this layout.
        Handle<uint32_t> m_bindingSlot;

        /// The layout of the constants data.
        Ptr<ConstantsLayout> m_constantsDataLayout;

        /// The computed hash value.
        HashValue64 m_hash = HashValue64{ 0 };
    };

    // Suitable for functions that return a null const RHI::Ptr<RHI::ShaderResourceGroupLayout>&.
    static const RHI::Ptr<RHI::ShaderResourceGroupLayout> NullSrgLayout;
}
