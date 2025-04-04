/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/ConstantsLayout.h>

#include <AzCore/std/containers/span.h>

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/unordered_map.h>

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
    struct ResourceBindingInfo
    {
        AZ_TYPE_INFO(ResourceBindingInfo, "{2B25FA97-21C2-4567-8F01-6A64F7B9DFF6}");
        static void Reflect(AZ::ReflectContext* context);

        ResourceBindingInfo() = default;
        ResourceBindingInfo(const RHI::ShaderStageMask& mask, uint32_t registerId, uint32_t spaceId)
            : m_shaderStageMask{ mask }
            , m_registerId{ registerId }
            , m_spaceId{ spaceId }
        {}

        /// Returns the hash computed for the binding info.
        HashValue64 GetHash() const;

        using Register = uint32_t;
        static const Register InvalidRegister = ~0u;

        /// Usage mask of resource.
        RHI::ShaderStageMask    m_shaderStageMask = RHI::ShaderStageMask::None;
        /// Register id of a resource.
        Register                m_registerId = InvalidRegister;
        /// Space id of the resource.
        uint32_t                m_spaceId = InvalidRegister;
    };

    //! This class describes binding information about the Shader Resource Group
    //! that is part of a Pipeline. Contains the register number for each SRG resource.
    struct ShaderResourceGroupBindingInfo
    {
        AZ_TYPE_INFO(ShaderResourceGroupBindingInfo, "{FE67D6A9-57E7-4075-94F9-3E2F443D1BD3}");
        static void Reflect(AZ::ReflectContext* context);

        ShaderResourceGroupBindingInfo() = default;

        /// Returns the hash computed for the binding info.
        HashValue64 GetHash() const;

        /// Register number for the constant data. All constants have the same register number.
        ResourceBindingInfo m_constantDataBindingInfo;
        /// Register number for the Shader Resource Group resources.
        AZStd::unordered_map<Name, ResourceBindingInfo> m_resourcesRegisterMap;
    };

    //! This class describes shader bindings to the RHI platform backend when creating a PipelineState.
    //! The base class contains a ShaderResourceGroupLayout table ordered by frequency of update. The platform
    //! descriptor implementation augments this table with low-level shader binding information.
    //!
    //! In short, if the shader compiler needs to communicate platform-specific shader binding information
    //! when constructing a pipeline state, this is the place to do it. Platforms are expected to override
    //! this class in their PLATFORM.Reflect library, which is then exposed to the offline shader compiler.
    class PipelineLayoutDescriptor
        : public AZStd::intrusive_base
    {
    public:
        AZ_RTTI(PipelineLayoutDescriptor, "{F2901A0F-9700-49E9-A266-55DCF1E39CF9}");
        AZ_CLASS_ALLOCATOR(PipelineLayoutDescriptor, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        static RHI::Ptr<PipelineLayoutDescriptor> Create();

        virtual ~PipelineLayoutDescriptor() = default;

        bool IsFinalized() const;

        //! Resets the descriptor back to an empty state.
        void Reset();

        //! Adds the layout info of shader resource group, ordered by frequency of update.
        void AddShaderResourceGroupLayoutInfo(const ShaderResourceGroupLayout& layout, const ShaderResourceGroupBindingInfo& shaderResourceGroupInfo);

        //! Sets the layout of inline constants.
        void SetRootConstantsLayout(const ConstantsLayout& rootConstantsLayout);

        //! Finalizes the descriptor for use. Must be called prior to serialization. Should not be called
        //! after serialization.
        RHI::ResultCode Finalize();

        //! Returns the number of shader resource group layouts added to this pipeline layout.
        size_t GetShaderResourceGroupLayoutCount() const;

        //! Returns the shader resource group layout pointer at the requested index.
        const ShaderResourceGroupLayout* GetShaderResourceGroupLayout(size_t index) const;

        //! Returns the shader resource group binding info at the requested index.
        const ShaderResourceGroupBindingInfo& GetShaderResourceGroupBindingInfo(size_t index) const;

        //! Returns the inline constants layout.
        const ConstantsLayout* GetRootConstantsLayout() const;

        //! Returns the hash computed for the pipeline layout.
        HashValue64 GetHash() const;

        //! Converts from an SRG binding slot to a shader resource group index.
        uint32_t GetShaderResourceGroupIndexFromBindingSlot(uint32_t bindingSlot) const;

    protected:
        PipelineLayoutDescriptor() = default;

    private:
        ///////////////////////////////////////////////////////////////////
        // Platform API

        //! Called when the pipeline layout descriptor is being reset to an empty state.
        virtual void ResetInternal();

        //! Called when the pipeline layout descriptor is being finalized.
        virtual ResultCode FinalizeInternal();

        //! Computes the hash of the platform-dependent descriptor (combined with the provided seed value).
        virtual HashValue64 GetHashInternal(HashValue64 seed) const;

        ///////////////////////////////////////////////////////////////////

        template <typename, typename>
        friend struct AnyTypeInfoConcept;
        template <typename, bool, bool>
        friend struct Serialize::InstanceFactory;

        // A hash of 0 is valid if the descriptor is empty.
        static constexpr HashValue64 InvalidHash = ~HashValue64{ 0 };
        using ShaderResourceGroupLayoutInfo = AZStd::pair<Ptr<ShaderResourceGroupLayout>, ShaderResourceGroupBindingInfo>;

        /// List of layout and binding information for each Shader Resource Group that is part of this Pipeline.
        AZStd::fixed_vector<ShaderResourceGroupLayoutInfo, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroupLayoutsInfo;

        /// Layout info about inline constants.
        Ptr<ConstantsLayout> m_rootConstantsLayout;

        /// Mapping from a Shader Resource Group binding slot to the index into the m_shaderResourceGroupLayoutsInfo.
        AZStd::array<uint32_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_bindingSlotToIndex = {};

        HashValue64 m_hash = InvalidHash;
    };
}
