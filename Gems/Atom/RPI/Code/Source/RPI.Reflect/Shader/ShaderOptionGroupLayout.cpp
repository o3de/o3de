/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

#include <Atom/RHI.Reflect/Bits.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/sort.h>
#include <AzCore/Utils/TypeHash.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace RPI
    {
        const char* ToString(ShaderOptionType shaderOptionType)
        {
            switch (shaderOptionType)
            {
            case ShaderOptionType::Boolean: return "Boolean";
            case ShaderOptionType::Enumeration: return "Enumeration";
            case ShaderOptionType::IntegerRange: return "IntegerRange";
            default: return "<Unknown>";
            }
        }
        
        ShaderOptionValues CreateEnumShaderOptionValues(AZStd::span<const AZStd::string_view> enumNames)
        {
            ShaderOptionValues values;
            values.reserve(enumNames.size());
            for (size_t i = 0; i < enumNames.size(); ++i)
            {
                values.emplace_back(Name{enumNames[i]}, i);
            }
            return values;
        }
        
        ShaderOptionValues CreateEnumShaderOptionValues(AZStd::initializer_list<AZStd::string_view> enumNames)
        {
            return CreateEnumShaderOptionValues(AZStd::span(enumNames.begin(), enumNames.end()));   
        }

        ShaderOptionValues CreateBoolShaderOptionValues()
        {
            return CreateEnumShaderOptionValues({"False", "True"});
        }
        
        ShaderOptionValues CreateIntRangeShaderOptionValues(uint32_t min, uint32_t max)
        {
            AZStd::vector<RPI::ShaderOptionValuePair> intOptionRange;
            intOptionRange.push_back({Name{AZStd::string::format("%u", min)},  RPI::ShaderOptionValue{min}});
            intOptionRange.push_back({Name{AZStd::string::format("%u", max)}, RPI::ShaderOptionValue{max}});
            return intOptionRange;
        }

        void ShaderOptionGroupHints::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderOptionGroupHints>()
                    ->Version(4)
                    ->Field("BakePrecedingVariants", &ShaderOptionGroupHints::m_bakePrecedingVariants)
                    ->Field("BakeEmptyAsDefault", &ShaderOptionGroupHints::m_bakeEmptyAsDefault)
                    ;
            }
        }

        void ShaderOptionDescriptor::Reflect(AZ::ReflectContext* context)
        {
            ShaderOptionValue::Reflect(context);
            NameReflectionMapForValues::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderOptionDescriptor>()
                    ->Version(6)  // 6: addition of m_specializationId field
                    ->Field("m_name", &ShaderOptionDescriptor::m_name)
                    ->Field("m_type", &ShaderOptionDescriptor::m_type)
                    ->Field("m_defaultValue", &ShaderOptionDescriptor::m_defaultValue)
                    ->Field("m_minValue", &ShaderOptionDescriptor::m_minValue)
                    ->Field("m_maxValue", &ShaderOptionDescriptor::m_maxValue)
                    ->Field("m_bitOffset", &ShaderOptionDescriptor::m_bitOffset)
                    ->Field("m_bitCount", &ShaderOptionDescriptor::m_bitCount)
                    ->Field("m_order", &ShaderOptionDescriptor::m_order)
                    ->Field("m_costEstimate", &ShaderOptionDescriptor::m_costEstimate)
                    ->Field("m_bitMask", &ShaderOptionDescriptor::m_bitMask)
                    ->Field("m_bitMaskNot", &ShaderOptionDescriptor::m_bitMaskNot)
                    ->Field("m_hash", &ShaderOptionDescriptor::m_hash)
                    ->Field("m_nameReflectionForValues", &ShaderOptionDescriptor::m_nameReflectionForValues)
                    ->Field("m_specializationId", &ShaderOptionDescriptor::m_specializationId)
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ShaderOptionDescriptor>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                    ->Method("GetName", &ShaderOptionDescriptor::GetName)
                    ->Method("GetDefaultValue", &ShaderOptionDescriptor::GetDefaultValue)
                    ->Method("GetValueName", static_cast<Name (ShaderOptionDescriptor::*)(ShaderOptionValue) const>(&ShaderOptionDescriptor::GetValueName))
                    ->Method("FindValue", &ShaderOptionDescriptor::FindValue)
                    ->Method("GetMinValue", &ShaderOptionDescriptor::GetMinValue)
                    ->Method("GetMaxValue", &ShaderOptionDescriptor::GetMaxValue)
                    ->Method("GetValuesCount", &ShaderOptionDescriptor::GetValuesCount)
                    ->Method("GetType", &ShaderOptionDescriptor::GetType)
                    ->Method("GetValueNameByIndex", static_cast<Name (ShaderOptionDescriptor::*)(uint32_t) const>(&ShaderOptionDescriptor::GetValueName))
                    ->Method("GetOrder", &ShaderOptionDescriptor::GetOrder)
                    ->Method("GetCostEstimate", &ShaderOptionDescriptor::GetCostEstimate)
                    ;
            }   
        }

        ShaderOptionDescriptor::ShaderOptionDescriptor(const Name& name,            
                                                       const ShaderOptionType& optionType, 
                                                       uint32_t bitOffset,
                                                       uint32_t order,
                                                       const ShaderOptionValues& nameIndexList,
                                                       const Name& defaultValue,
                                                       uint32_t cost,
                                                       int specializationId)

            : m_name{name}
            , m_type{optionType}
            , m_bitOffset{bitOffset}
            , m_order{order}
            , m_costEstimate{cost}
            , m_defaultValue{defaultValue}
            , m_specializationId{specializationId}
        {
            for (auto pair : nameIndexList)
            {   // Registers the pair in the lookup table
                AddValue(pair.first, pair.second);

                if (m_defaultValue.IsEmpty())
                {
                    m_defaultValue = pair.first;
                }
            }

            uint32_t numValues = (m_type == ShaderOptionType::IntegerRange) ? (m_maxValue.GetIndex() - m_minValue.GetIndex() + 1) : (uint32_t) nameIndexList.size();
            numValues = RHI::NextPowerOfTwo(numValues) - 1;
            m_bitCount = RHI::CountBitsSet(numValues);

            ShaderVariantKey bitMask;
            bitMask = AZ_BIT_MASK(m_bitCount);
            bitMask <<= m_bitOffset;
            m_bitMask = bitMask;
            m_bitMaskNot = ~bitMask;

            m_hash = TypeHash64(m_bitMask, static_cast<HashValue64>(m_name.GetHash()));
        }

        const Name& ShaderOptionDescriptor::GetName() const
        {
            return m_name;
        }

        uint32_t ShaderOptionDescriptor::GetBitOffset() const
        {
            return m_bitOffset;
        }

        uint32_t ShaderOptionDescriptor::GetBitCount() const
        {
            return m_bitCount;
        }

        uint32_t ShaderOptionDescriptor::GetOrder() const
        {
            return m_order;
        }

        uint32_t ShaderOptionDescriptor::GetCostEstimate() const
        {
            return m_costEstimate;
        }

        int ShaderOptionDescriptor::GetSpecializationId() const
        {
            return m_specializationId;
        }

        ShaderVariantKey ShaderOptionDescriptor::GetBitMask() const
        {
            return m_bitMask;
        }

        ShaderVariantKey ShaderOptionDescriptor::GetBitMaskNot() const
        {
            return m_bitMaskNot;
        }

        HashValue64 ShaderOptionDescriptor::GetHash() const
        {
            return m_hash;
        }

        bool ShaderOptionDescriptor::Set(ShaderOptionGroup& group, const Name& valueName) const
        {
            auto valueIndex = FindValue(valueName);
            if (valueIndex.IsValid())
            {
                return Set(group, valueIndex);
            }
            else
            {
                AZ_Error(DebugCategory, false, "ShaderOption value '%s' does not exist", valueName.GetCStr());
                return false;
            }
        }

        bool ShaderOptionDescriptor::Set(ShaderOptionGroup& group, const ShaderOptionValue valueIndex) const
        {
            if (valueIndex.IsNull())
            {
                AZ_Error(DebugCategory, false, "Invalid ShaderOption value");
                return false;
            }
            
            if (m_type == ShaderOptionType::Unknown)
            {
                group.GetShaderVariantMask() &= m_bitMaskNot;
            }
            else
            {
                if (!(m_minValue.GetIndex() <= valueIndex.GetIndex() && valueIndex.GetIndex() <= m_maxValue.GetIndex()))
                {
                    AZ_Error(DebugCategory, false, "%s ShaderOption value [%d] is out of range [%d,%d].",
                        ToString(m_type), valueIndex.GetIndex(), m_minValue.GetIndex(), m_maxValue.GetIndex());
                    return false;
                }

                EncodeBits(group.GetShaderVariantKey(), valueIndex.GetIndex() - m_minValue.GetIndex());
                group.GetShaderVariantMask() |= m_bitMask;
            }

            return true;
        }

        bool ShaderOptionDescriptor::Set(ShaderVariantKey& key, const ShaderOptionValue valueIndex) const
        {
            if (valueIndex.IsNull())
            {
                AZ_Error(DebugCategory, false, "Invalid ShaderOption value");
                return false;
            }
            
            if (m_type != ShaderOptionType::Unknown)
            {
                if (!(m_minValue.GetIndex() <= valueIndex.GetIndex() && valueIndex.GetIndex() <= m_maxValue.GetIndex()))
                {
                    AZ_Error(DebugCategory, false, "%s ShaderOption value [%d] is out of range [%d,%d].",
                        ToString(m_type), valueIndex.GetIndex(), m_minValue.GetIndex(), m_maxValue.GetIndex());
                    return false;
                }

                EncodeBits(key, valueIndex.GetIndex() - m_minValue.GetIndex());
            }

            return true;
            
        }

        ShaderOptionValue ShaderOptionDescriptor::Get(const ShaderOptionGroup& group) const
        {
            if (group.GetShaderVariantMask().test(m_bitOffset))
            {
                return ShaderOptionValue(DecodeBits(group.GetShaderVariantKey()) + m_minValue.GetIndex());
            }

            return ShaderOptionValue();
        }

        void ShaderOptionDescriptor::Clear(ShaderOptionGroup& group) const
        {
            group.GetShaderVariantMask() &= m_bitMaskNot;
        }

        void ShaderOptionDescriptor::AddValue(const Name& valueName, const ShaderOptionValue valueIndex)
        {
            AZ_Assert(m_type != ShaderOptionType::IntegerRange || valueIndex.GetIndex() == ShaderOptionValue((uint32_t)AZStd::stoll(AZStd::string{valueName.GetCStr()})).GetIndex(), "By convention, IntegerRange's values' ids must be equal to their numerical value!");

            m_nameReflectionForValues.Insert(valueName, valueIndex);

            if (m_minValue.IsNull() || m_minValue.GetIndex() > valueIndex.GetIndex())
            {
                m_minValue = valueIndex;
            }
            if (m_maxValue.IsNull() || m_maxValue.GetIndex() < valueIndex.GetIndex())
            {
                m_maxValue = valueIndex;
            }
        }

        void ShaderOptionDescriptor::SetDefaultValue(const Name& valueName)
        {
            AZ_Assert(!valueName.IsEmpty(), "The default value cannot be empty!");
            auto valueIter = m_nameReflectionForValues.Find(valueName);
            if (valueIter.IsNull())
            {
                AZ_Assert(false, "ShaderOption [%s] has no member value [%s] so this cannot be the default!", m_name.GetCStr(), valueName.GetCStr());
                return;
            }

            m_defaultValue = valueName;
        }

        const Name& ShaderOptionDescriptor::GetDefaultValue() const
        {
            return m_defaultValue;
        }

        uint32_t ShaderOptionDescriptor::GetValuesCount() const
        {
            return static_cast<uint32_t>(m_maxValue.GetIndex() - m_minValue.GetIndex() + 1);
        }

        /// Sets the hint type for the shader option
        void ShaderOptionDescriptor::SetType(ShaderOptionType optionType)
        {
            m_type = optionType;
        }

        /// Gets the hint type for the shader option
        const ShaderOptionType& ShaderOptionDescriptor::GetType() const
        {
            return m_type;
        }

        ShaderOptionValue ShaderOptionDescriptor::GetMinValue() const
        {
            return m_minValue;
        }

        ShaderOptionValue ShaderOptionDescriptor::GetMaxValue() const
        {
            return m_maxValue;
        }

        ShaderOptionValue ShaderOptionDescriptor::FindValue(const Name& valueName) const
        {
            switch (m_type)
            {
                case ShaderOptionType::Boolean:
                    // This is better than hardcoding True, or On, or Enabled:
                    return m_nameReflectionForValues.Find(valueName);
                case ShaderOptionType::Enumeration:
                    return m_nameReflectionForValues.Find(valueName);

                case ShaderOptionType::IntegerRange:
                    {
                        int asInt;
                        if (AzFramework::StringFunc::LooksLikeInt(valueName.GetCStr(), &asInt))
                        {
                            if (aznumeric_cast<int64_t>(m_minValue.GetIndex()) <= asInt && asInt <= aznumeric_cast<int64_t>(m_maxValue.GetIndex()))
                            {
                                return ShaderOptionValue(asInt);
                            }
                        }
                    }
                    return ShaderOptionValue();

                default:
                    AZ_Assert(false, "Unhandled case for ShaderOptionType! We should not break here!");
            }

            // Unreachable code
            return ShaderOptionValue();
        }

        Name ShaderOptionDescriptor::GetValueName(ShaderOptionValue value) const
        {
            if (m_type == ShaderOptionType::IntegerRange)
            {
                // We can just return the value here, as IntegerRange's values' ids must be equal to their numerical value, this had been checked in AddValue()
                // We can't use m_nameReflectionForValues, since it only contains min and max value
                uint32_t value_uint = value.GetIndex();
                if (m_minValue.GetIndex() <= value_uint && value_uint <= m_maxValue.GetIndex())
                {
                    return Name(AZStd::to_string(value_uint));
                }
                else
                {
                    // mimic the behavior of RHI::NameIdReflectionMap's Find function
                    return {};
                }
            }
            auto name = m_nameReflectionForValues.Find(value);
            return name;
        }

        Name ShaderOptionDescriptor::GetValueName(uint32_t valueIndex) const
        {
            return GetValueName(ShaderOptionValue{ valueIndex });
        }

        void ShaderOptionDescriptor::EncodeBits(ShaderVariantKey& shaderVariantKey, uint32_t value) const
        {
            if (value < AZ_BIT(m_bitCount))
            {
                ShaderVariantKey valueBits = (value & AZ_BIT_MASK(m_bitCount));
                valueBits <<= m_bitOffset;
                shaderVariantKey &= m_bitMaskNot;
                shaderVariantKey |= valueBits;
            }
            else
            {
                AZ_Assert(false, "Exceeded maximum number of bits allocated for option.");
            }
        }

        uint32_t ShaderOptionDescriptor::DecodeBits(ShaderVariantKey shaderVariantKey) const
        {
            shaderVariantKey >>= m_bitOffset;
            shaderVariantKey &= AZ_BIT_MASK(m_bitCount);
            uint32_t value = static_cast<uint32_t>(shaderVariantKey.to_ulong());
            return value;
        }
        
        bool ShaderOptionDescriptor::operator==(const ShaderOptionDescriptor& rhs) const
        {
            return m_hash == rhs.m_hash;
        }

        bool ShaderOptionDescriptor::operator!=(const ShaderOptionDescriptor& rhs) const
        {
            return m_hash != rhs.m_hash;
        }

        bool ShaderOptionDescriptor::CompareOrder(const ShaderOptionDescriptor& first, const ShaderOptionDescriptor& second)
        {
            return first.GetOrder() < second.GetOrder();            
        }

        bool ShaderOptionDescriptor::SameOrder(const ShaderOptionDescriptor& first, const ShaderOptionDescriptor& second)
        {
            return first.GetOrder() == second.GetOrder();
        }

        void ShaderOptionGroupLayout::Reflect(AZ::ReflectContext* context)
        {
            ShaderOptionIndex::Reflect(context);
            NameReflectionMapForOptions::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderOptionGroupLayout>()
                    ->Version(3)
                    ->Field("m_bitMask", &ShaderOptionGroupLayout::m_bitMask)
                    ->Field("m_options", &ShaderOptionGroupLayout::m_options)
                    ->Field("m_nameReflectionForOptions", &ShaderOptionGroupLayout::m_nameReflectionForOptions)
                    ->Field("m_hash", &ShaderOptionGroupLayout::m_hash)
                    ->Field("m_isFullySpecialized", &ShaderOptionGroupLayout::m_isFullySpecialized)
                    ->Field("m_useSpecializationConstants", &ShaderOptionGroupLayout::m_useSpecializationConstants)
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ShaderOptionGroupLayout>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ->Method("GetShaderOptions", &ShaderOptionGroupLayout::GetShaderOptions)
                    ;
            }   
        }

        Ptr<ShaderOptionGroupLayout> ShaderOptionGroupLayout::Create()
        {
            return aznew ShaderOptionGroupLayout;
        }

        bool ShaderOptionGroupLayout::IsFinalized() const
        {
            return m_hash != HashValue64{ 0 };
        }

        HashValue64 ShaderOptionGroupLayout::GetHash() const
        {
            return m_hash;
        }

        bool ShaderOptionGroupLayout::IsFullySpecialized() const
        {
            return m_isFullySpecialized;
        }

        bool ShaderOptionGroupLayout::UseSpecializationConstants() const
        {
            return m_useSpecializationConstants;
        }

        void ShaderOptionGroupLayout::Clear()
        {
            m_options.clear();
            m_nameReflectionForOptions.Clear();
            m_bitMask = {};
            m_hash = HashValue64{ 0 };
        }

        void ShaderOptionGroupLayout::Finalize()
        {
            AZStd::sort(m_options.begin(), m_options.end(), ShaderOptionDescriptor::CompareOrder);

            // Start with a hash of the size so that m_hash!=0 will mean the group is finalized, even if the options list is empty.
            HashValue64 hash = TypeHash64(m_options.size());

            for (const ShaderOptionDescriptor& option : m_options)
            {
                hash = TypeHash64(option.GetHash(), hash);
            }
            m_hash = hash;
            m_isFullySpecialized = !AZStd::any_of(
                m_options.begin(),
                m_options.end(),
                [](const ShaderOptionDescriptor& elem)
                {
                    return elem.GetSpecializationId() < 0;
                });
            m_useSpecializationConstants = AZStd::any_of(
                m_options.begin(),
                m_options.end(),
                [](const ShaderOptionDescriptor& elem)
                {
                    return elem.GetSpecializationId() >= 0;
                });
        }

        bool ShaderOptionGroupLayout::ValidateIsFinalized() const
        {
            if (!IsFinalized())
            {
                AZ_Assert(false, "ShaderOptionGroupLayout is not finalized! This operation is only permitted on a finalized layout.");
                return false;
            }
            return true;
        }

        bool ShaderOptionGroupLayout::ValidateIsNotFinalized() const
        {
            if (IsFinalized())
            {
                AZ_Assert(false, "ShaderOptionGroupLayout is finalized! This operation is only permitted on a non-finalized layout.");
                return false;
            }
            return true;
        }

        bool ShaderOptionGroupLayout::AddShaderOption(const ShaderOptionDescriptor& option)
        {
            if (!ValidateIsNotFinalized())
            {
                return false;
            }

            const Name& optionName = option.GetName();
            const ShaderVariantKey bitMask = option.GetBitMask();

            if ((m_bitMask & bitMask).any())
            {
                AZ_Error(DebugCategory, false, "ShaderOptionBinding '%s': mask overlaps with previously added masks.", optionName.GetCStr());
                return false;
            }

            if (option.GetName().IsEmpty())
            {
                AZ_Error(DebugCategory, false, "ShaderOptionBinding added with empty name.");
                return false;
            }

            if (option.GetBitCount() == 0)
            {
                AZ_Error(DebugCategory, false, "ShaderOptionBinding '%s' has zero bits.", optionName.GetCStr());
                return false;
            }

            if (option.GetBitOffset() + option.GetBitCount() > bitMask.size())
            {
                AZ_Error(DebugCategory, false, "ShaderOptionBinding '%s' exceeds size of mask.", optionName.GetCStr());
                return false;
            }

            if (AZStd::any_of(m_options.begin(), m_options.end(),
                [&option](const ShaderOptionDescriptor& other) { return ShaderOptionDescriptor::SameOrder(option, other); }))
            {
                AZ_Error(DebugCategory, false, "ShaderOption '%s' has the same order (%d) as another shader option.", optionName.GetCStr(), option.GetOrder());
                return false;
            }

            if (!option.FindValue(option.GetDefaultValue()).IsValid())
            {
                AZ_Error(DebugCategory, false, "ShaderOption '%s' has invalid default value '%s'.", optionName.GetCStr(), option.GetDefaultValue().GetCStr());
                return false;
            }

            const ShaderOptionIndex optionIndex(m_options.size());
            if (!m_nameReflectionForOptions.Insert(optionName, optionIndex))
            {
                AZ_Error(DebugCategory, false, "ShaderOptionBinding '%s': name already exists.", optionName.GetCStr());
                return false;
            }

            m_bitMask |= bitMask;

            m_options.push_back(option);
            return true;
        }

        ShaderOptionIndex ShaderOptionGroupLayout::FindShaderOptionIndex(const Name& optionName) const
        {
            if (ValidateIsFinalized())
            {
                return m_nameReflectionForOptions.Find(optionName);
            }
            return {};
        }

        ShaderOptionValue ShaderOptionGroupLayout::FindValue(const Name& optionName, const Name& valueName) const
        {
            return FindValue(FindShaderOptionIndex(optionName), valueName);
        }

        ShaderOptionValue ShaderOptionGroupLayout::FindValue(const ShaderOptionIndex& optionIndex, const Name& valueName) const
        {
            if (optionIndex.IsValid() && optionIndex.GetIndex() < m_options.size())
            {
                return m_options[optionIndex.GetIndex()].FindValue(valueName);
            }
            else
            {
                return ShaderOptionValue{};
            }
        }

        uint32_t ShaderOptionGroupLayout::GetBitSize() const
        {
            if (m_options.empty())
            {
                return 0;
            }
            else
            {
                return m_options.back().GetBitOffset() + m_options.back().GetBitCount();
            }
        }

        const AZStd::vector<ShaderOptionDescriptor>& ShaderOptionGroupLayout::GetShaderOptions() const
        {
            return m_options;
        }

        const ShaderOptionDescriptor& ShaderOptionGroupLayout::GetShaderOption(ShaderOptionIndex optionIndex) const
        {
            return m_options[optionIndex.GetIndex()];
        }

        size_t ShaderOptionGroupLayout::GetShaderOptionCount() const
        {
            return m_options.size();
        }

        ShaderVariantKey ShaderOptionGroupLayout::GetBitMask() const
        {
            return m_bitMask;
        }

        bool ShaderOptionGroupLayout::IsValidShaderVariantKey(const ShaderVariantKey& shaderVariantKey) const
        {
            if (ValidateIsFinalized())
            {
                return (m_bitMask & shaderVariantKey) == shaderVariantKey;
            }
            return false;
        }
    } // namespace RPI
} // namespace AZ
