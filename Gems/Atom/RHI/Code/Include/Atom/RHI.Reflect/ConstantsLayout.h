/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
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
    //! ConstantsLayout defines a the layout of a set of constant shader inputs.
    //! Each constant input spans a range of bytes in an byte array.
    //! The array could represent a constant buffer or an inline structure
    //! depending on where the constants are being use.
    //!
    //! To use the class, assign shader inputs using AddShaderInput, and call Finalize to
    //! complete construction of the layout. This class is intended to be built using an offline shader
    //! compiler, and serialized to / from disk.
    class ConstantsLayout
        : public AZStd::intrusive_base
    {
    public:
        AZ_TYPE_INFO(ConstantsLayout, "{66EDAC32-7730-4F05-AF9D-B3CB0F5D90E0}");
        AZ_CLASS_ALLOCATOR(ConstantsLayout, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        static RHI::Ptr<ConstantsLayout> Create();

        //! Adds a shader input to the constants layout.
        void AddShaderInput(const ShaderInputConstantDescriptor& descriptor);

        //! Clears the layout to an empty state. The layout must be finalized prior to usage.
        void Clear();

        bool IsFinalized() const;

        //! Finalizes the layout for access. Must be called prior to usage or serialization.
        //! It is not permitted to mutate the layout once Finalize is called. Clear must
        //! be called first. If the call fails, the layout is cleared back to an empty state
        //! and false is returned. Otherwise, true is returned and the layout is considered
        //! finalized.
        bool Finalize();

        //////////////////////////////////////////////////////////////////////////
        // The following methods are only permitted on a finalized layout.

        //! Returns the hash of the layout.
        HashValue64 GetHash() const;

        //! Resolves a shader input name to an index. To maximize performance,
        //! the name to index resolve should be done as an initialization step and the indices
        //! cached.
        ShaderInputConstantIndex FindShaderInputIndex(const Name& name) const;

        //! The interval of a constant is its byte [min, max) into the constants data.
        Interval GetInterval(ShaderInputConstantIndex inputIndex) const;

        //! Returns the shader input associated with the requested index. It is not permitted
        //! to call this method with a null index. An assert will fire otherwise.
        const ShaderInputConstantDescriptor& GetShaderInput(ShaderInputConstantIndex inputIndex) const;

        //! Returns the full lists of shader input added to the layout. Inputs
        //! maintain their original order with respect to AddShaderInput.
        AZStd::span<const ShaderInputConstantDescriptor> GetShaderInputList() const;

        //! Returns the total size in bytes used by the constants.
        uint32_t GetDataSize() const;

        //! Validates that the inputIndex is valid.
        //! Emits an assert and returns false on failure; returns true on success.
        //! If validation is disabled, true is always returned.
        bool ValidateAccess(ShaderInputConstantIndex inputIndex) const;

        //! Prints to the console the shader input names specified by input list of indices
        //! Will ignore any indices outside of the inputs array bounds
        void DebugPrintNames(AZStd::span<const ShaderInputConstantIndex> constantList) const;

    protected:
        ConstantsLayout() = default;

    private:
        template <typename, typename>
        friend struct AnyTypeInfoConcept;
        template <typename, bool, bool>
        friend struct Serialize::InstanceFactory;

        bool ValidateConstantInputs() const;

        static constexpr HashValue64 InvalidHash = ~HashValue64{ 0 };
        using IdReflectionMapForConstants = NameIdReflectionMap<ShaderInputConstantIndex>;

        AZStd::vector<ShaderInputConstantDescriptor> m_inputs;
        IdReflectionMapForConstants m_idReflection;
        uint32_t m_sizeInBytes = 0;
        HashValue64 m_hash = InvalidHash;
    };
}
