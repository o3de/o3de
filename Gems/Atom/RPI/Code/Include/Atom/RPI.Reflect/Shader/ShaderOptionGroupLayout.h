/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionTypes.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

#include <Atom/RHI.Reflect/NameIdReflectionMap.h>

#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/Memory/PoolAllocator.h>
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

    namespace RPI
    {
        class ShaderOptionGroupLayout;
        class ShaderOptionGroup;

        /**
         * This struct describes compile time hints for the shader option group layout building.
         * The builder (ShaderAssetBuilder or other) is free to ignore or enforce these options.
         */
        struct ATOM_RPI_REFLECT_API ShaderOptionGroupHints
        {
            AZ_TYPE_INFO(AZ::RPI::ShaderOptionGroupHints, "{09FB2541-DD10-46B9-AAF0-FF8EE8B59FEB}");

            static void Reflect(ReflectContext* context);

            //! Hints the ShaderAssetBuilder that all variant nodes which precede any node should also be baked
            bool m_bakePrecedingVariants = false;

            //! Hints the ShaderAssetBuilder that empty preceding options should assume default values when baked
            bool m_bakeEmptyAsDefault = false;
        };

        //! Creates a list of shader option values that can be used to construct a ShaderOptionDescriptor.
        ATOM_RPI_REFLECT_API ShaderOptionValues CreateEnumShaderOptionValues(AZStd::span<const AZStd::string_view> enumNames);
        ATOM_RPI_REFLECT_API ShaderOptionValues CreateEnumShaderOptionValues(AZStd::initializer_list<AZStd::string_view> enumNames);
        ATOM_RPI_REFLECT_API ShaderOptionValues CreateBoolShaderOptionValues();
        ATOM_RPI_REFLECT_API ShaderOptionValues CreateIntRangeShaderOptionValues(uint32_t min, uint32_t max);

        //! Describes a shader option to the ShaderOptionGroupLayout class. Maps a shader option
        //! to a set of bits in a mask in order to facilitate packing values into a mask to
        //! form a ShaderKey.
        class ATOM_RPI_REFLECT_API ShaderOptionDescriptor final
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::ShaderOptionDescriptor, "{07B9E2F7-5408-49E9-904D-CC1A9C33230E}");

            static void Reflect(ReflectContext* context);

            ShaderOptionDescriptor() = default;

            //! ShaderOptionDescriptor constructor.
            //! This is the preferred constructor for ShaderOptionDescriptor.
            //! @param name           variable name for this Option
            //! @param optionType     Type hint for the option - bool, enum, integer range, etc.
            //! @param bitOffset      Bit offset must match the ShaderOptionGroupLayout where this Option will be added
            //! @param order          The order (rank) of the shader option. Must be unique within a group. Lower order is higher priority.
            //! @param cost           The cost is the statically-analyzed estimated performance impact
            //! @param nameIndexList  List of valid (valueName, value) pairs for this Option. See "Create*ShaderOptionValues" utility functions above.
            //! @param defaultValue   Default value name, which must also be in the nameIndexList. In the cases where the list
            //!                       defines a range (IntegerRange for instance) defaultValue must be within the range instead.
            //!                       If omitted, the first entry in @nameIndexList will be used.
            ShaderOptionDescriptor(const Name& name,
                                   const ShaderOptionType& optionType,
                                   uint32_t bitOffset,
                                   uint32_t order,
                                   const ShaderOptionValues& nameIndexList,
                                   const Name& defaultValue = {},
                                   uint32_t cost = 0,
                                   int specializationId = -1);

            AZ_DEFAULT_COPY_MOVE(ShaderOptionDescriptor);

            const Name& GetName() const;

            //! Returns the offset and count of bits comprising the local mask for the option.
            uint32_t GetBitOffset() const;
            uint32_t GetBitCount() const;

            //! Returns the order (rank) for this option. Lower order means higher priority.
            uint32_t GetOrder() const;

            uint32_t GetCostEstimate() const;

            //! Return the specialization id. -1 if this option can't be specialize.
            int GetSpecializationId() const;

            //! Returns the mask comprising bits specific to this option.
            ShaderVariantKey GetBitMask() const;

            //! Returns the reverse mask for this option, used to unset the mask
            ShaderVariantKey GetBitMaskNot() const;

            //! Returns a unique hash value describing this descriptor.
            HashValue64 GetHash() const;

            //! Sets the corresponding option in the option group to the specified named option value.
            //! For performance reasons consider caching the index for valueName and calling
            //!    void Set(ShaderOptionGroup& group, const ShaderOptionValue value) instead.
            bool Set(ShaderOptionGroup& group, const Name& valueName) const;

            //! Sets the corresponding option in the option group to the specified option value.
            bool Set(ShaderOptionGroup& group, const ShaderOptionValue value) const;

            //! Sets the corresponding option in the variant key directly to the specified option value.
            bool Set(ShaderVariantKey& key, const ShaderOptionValue value) const;

            //! Gets the option value for the corresponding option in the option group.
            ShaderOptionValue Get(const ShaderOptionGroup& group) const;

            //! Sets the corresponding option in the option group to an uninitialized state.
            void Clear(ShaderOptionGroup& group) const;

            //! Sets the default value for this option by name.
            void SetDefaultValue(const Name& valueName);

            //! Gets the default value for this option by name.
            const Name& GetDefaultValue() const;

            //! Gets the number of valid values for this option
            uint32_t GetValuesCount() const;

            //! Sets the type for the shader option (bool, enum, integer range, etc.).
            void SetType(ShaderOptionType optionType);

            //! Gets the type for the shader option (bool, enum, integer range, etc.).
            const ShaderOptionType& GetType() const;

            //! Gets the minimal possible option value for the corresponding option .
            ShaderOptionValue GetMinValue() const;

            //! Gets the maximum possible option value for the corresponding option.
            ShaderOptionValue GetMaxValue() const;

            //! Finds a shader value index from a value name. Returns an empty handle if the value name was not found.
            ShaderOptionValue FindValue(const Name& valueName) const;

            //! Gets the name for the option value.
            Name GetValueName(ShaderOptionValue value) const;

            //! Gets the name for the option value.
            Name GetValueName(uint32_t valueIndex) const;

            bool operator == (const ShaderOptionDescriptor&) const;
            bool operator != (const ShaderOptionDescriptor&) const;

            //! True if the order of first option has higher priority than the second option
            static bool CompareOrder(const ShaderOptionDescriptor& first, const ShaderOptionDescriptor& second);

            //! True if the order of this option has the same priority as the other option
            static bool SameOrder(const ShaderOptionDescriptor& first, const ShaderOptionDescriptor& second);

            //! Decodes the value stored in the shader key associated with the bit region defined by the descriptor.
            //! The method will not test the bit mask, this responsibility lies with the caller.
            uint32_t DecodeBits(ShaderVariantKey key) const;

        private:
            static constexpr const char* DebugCategory = "ShaderOption";

            //! Adds a new option value to the (name, index) map. Only to use from the constructor.
            void AddValue(const Name& name, const ShaderOptionValue value);

            //! Encodes a value into the bit region of the provided shader key defined by the descriptor.
            void EncodeBits(ShaderVariantKey& key, uint32_t value) const;

            Name               m_name;
            ShaderOptionType   m_type = ShaderOptionType::Unknown;
            Name               m_defaultValue;
            ShaderOptionValue  m_minValue; //!< Min possible value, used for validation (when the type is IntegerRange for example).
            ShaderOptionValue  m_maxValue; //!< Max possible value, used for validation (when the type is IntegerRange for example).
            uint32_t m_bitOffset = 0;
            uint32_t m_bitCount = 0;
            uint32_t m_order = 0;          //!< The order (or rank) of the shader option dictates its priority. Lower order (rank) is higher priority.
            uint32_t m_costEstimate = 0;
            int m_specializationId = -1; //< Specialization id. A value of -1 means no specialization.
            ShaderVariantKey m_bitMask;
            ShaderVariantKey m_bitMaskNot;

            //! Reflection information for each kind of shader input, stored in
            //! sorted vectors. Binary search is used to find entries.
            using NameReflectionMapForValues = RHI::NameIdReflectionMap<ShaderOptionValue>;
            NameReflectionMapForValues m_nameReflectionForValues;

            HashValue64 m_hash = HashValue64{ 0 };
        };

        //! Describes a complete layout of shader options and how they map to a ShaderKey.
        //! Contains information on how to construct shader keys from shader option key/value
        //! pair data. Does not contain actual shader option values (those reside in ShaderOptionGroup).
        class ATOM_RPI_REFLECT_API ShaderOptionGroupLayout final
            : public AZStd::intrusive_base
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::ShaderOptionGroupLayout, "{32E269DE-12A2-4B65-B4F8-BAE93DD39D7E}");
            AZ_CLASS_ALLOCATOR(ShaderOptionGroupLayout, AZ::ThreadPoolAllocator);
            static void Reflect(ReflectContext* context);

            static Ptr<ShaderOptionGroupLayout> Create();

            AZ_DISABLE_COPY_MOVE(ShaderOptionGroupLayout);

            //! Returns whether the layout is finalized.
            bool IsFinalized() const;

            //! Clears the layout to an empty state.
            void Clear();

            //! Finalizes the layout. Clear must be called before mutating the layout again.
            //! Finalization is preserved when serializing; it is unnecessary to call Finalize
            //! after serialization assuming it was called prior to serialization.
            void Finalize();

            //! Adds a new shader option paired with a mask. If the mask overlaps an existing mask,
            //! the function fails and returns false. Otherwise, the mask is added to the set and
            //! the function returns true.
            bool AddShaderOption(const ShaderOptionDescriptor& shaderOption);

            //! Finds a shader option index from an option name. Returns an empty handle if the option name was not found.
            ShaderOptionIndex FindShaderOptionIndex(const Name& optionName) const;

            //! Finds a shader value from an option name. Returns an empty handle if the value name was not found.
            ShaderOptionValue FindValue(const Name& optionName, const Name& valueName) const;

            //! Finds a shader value from a value name. Returns an empty handle if the value name was not found.
            ShaderOptionValue FindValue(const ShaderOptionIndex& optionIndex, const Name& valueName) const;

            //! Returns the number of ShaderVariantKey bits used by this layout. The max is ShaderVariantKeyBitCount.
            uint32_t GetBitSize() const;

            //! Returns a list of all shader options in the ShaderOptionGroupLayout.
            const AZStd::vector<ShaderOptionDescriptor>& GetShaderOptions() const;

            //! Returns the shader option descriptor associated with the requested index.
            const ShaderOptionDescriptor& GetShaderOption(ShaderOptionIndex optionIndex) const;

            //! Returns the total number of shader options.
            size_t GetShaderOptionCount() const;

            //! Returns the mask that is the complete set of bits used by options in the layout.
            ShaderVariantKey GetBitMask() const;

            //! Returns whether the key contains all valid bits for the given layout.
            bool IsValidShaderVariantKey(const ShaderVariantKey& shaderVariantKey) const;

            HashValue64 GetHash() const;

            //! Returns true if all shader options of the layout are using specialization constants. Please note that each
            //! supervariant can have specialization constants off even if the layout is IsFullySpecialized.
            bool IsFullySpecialized() const;

            //! Returns true if at least one shader option is using specialization constant.
            bool UseSpecializationConstants() const;

        private:
            ShaderOptionGroupLayout() = default;

            template <typename, typename>
            friend struct AnyTypeInfoConcept;
            template <typename, bool, bool>
            friend struct Serialize::InstanceFactory;

            static constexpr const char* DebugCategory = "ShaderOption";

            bool ValidateIsFinalized() const;
            bool ValidateIsNotFinalized() const;

            ShaderVariantKey m_bitMask;
            AZStd::vector<ShaderOptionDescriptor> m_options;
            using NameReflectionMapForOptions = RHI::NameIdReflectionMap<ShaderOptionIndex>;
            NameReflectionMapForOptions m_nameReflectionForOptions;
            HashValue64 m_hash = HashValue64{ 0 };

            // True if all shader options are using specialization constants
            bool m_isFullySpecialized = false;
            // True if at least one shader options is using specialization constants
            bool m_useSpecializationConstants = false;
        };
    } // namespace RPI

} // namespace AZ
