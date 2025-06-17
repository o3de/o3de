/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        ShaderOptionGroup::ShaderOptionGroup(const ShaderOptionGroup& rhs)
            : m_layout(rhs.m_layout)
            , m_id{rhs.m_id}
        {
        }

        ShaderOptionGroup::ShaderOptionGroup(const ConstPtr<ShaderOptionGroupLayout>& shaderOptionGroupLayout)
            : m_layout{shaderOptionGroupLayout}
        {
            AZ_Assert(m_layout, "ShaderOptionGroup created with null layout!");

            Clear();
        }

        ShaderOptionGroup::ShaderOptionGroup(const ConstPtr<ShaderOptionGroupLayout>& shaderOptionGroupLayout, const ShaderVariantId& id)
            : m_layout{shaderOptionGroupLayout}
            , m_id{id}
        {
            AZ_Assert(m_layout, "ShaderOptionGroup created with null layout!");
        }

        void ShaderOptionGroup::Reflect(AZ::ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ShaderOptionGroup>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ->Method("GetValueByOptionName", static_cast<ShaderOptionValue (ShaderOptionGroup::*)(const Name&) const>(&ShaderOptionGroup::GetValue))
                    ->Method("GetShaderOptionDescriptors", &ShaderOptionGroup::GetShaderOptionDescriptors)
                    ->Method("GetShaderVariantId", &ShaderOptionGroup::GetShaderVariantId)
                    ->Method("ClearValue", static_cast<bool (ShaderOptionGroup::*)(const Name&)>(&ShaderOptionGroup::ClearValue))
                    ;
            }
        }

        void ShaderOptionGroup::Clear()
        {
            m_id.Reset();
        }

        ShaderOptionIndex ShaderOptionGroup::FindShaderOptionIndex(const Name& optionName) const
        {
            return m_layout->FindShaderOptionIndex(optionName);
        }

        bool ShaderOptionGroup::GetShaderOptionIndex(const Name& optionName, ShaderOptionIndex& optionIndex) const
        {
            optionIndex = FindShaderOptionIndex(optionName);

            if (!optionIndex.IsValid())
            {
                AZ_Error(DebugCategory, false, "ShaderOption '%s' does not exist", optionName.GetCStr());
                return false;
            }

            return true;
        }

        bool ShaderOptionGroup::ValidateIndex(const ShaderOptionIndex& optionIndex) const
        {
            if (!optionIndex.IsValid())
            {
                AZ_Error(DebugCategory, false, "Invalid ShaderOptionIndex");
                return false;
            }

            return true;
        }

        bool ShaderOptionGroup::SetValue(const Name& optionName, const Name& valueName)
        {
            ShaderOptionIndex index;

            if (GetShaderOptionIndex(optionName, index))
            {
                return SetValue(index, valueName);
            }
            else
            {
                return false;
            }
        }

        bool ShaderOptionGroup::SetValue(const Name& optionName, ShaderOptionValue valueIndex)
        {
            ShaderOptionIndex index;

            if (GetShaderOptionIndex(optionName, index))
            {
                return SetValue(index, valueIndex);
            }
            else
            {
                return false;
            }
        }

        ShaderOptionValue ShaderOptionGroup::GetValue(const Name& optionName) const
        {
            ShaderOptionIndex index;

            if (GetShaderOptionIndex(optionName, index))
            {
                return GetValue(index);
            }
            else
            {
                return ShaderOptionValue{};
            }
        }

        bool ShaderOptionGroup::SetValue(ShaderOptionIndex optionIndex, const Name& valueName)
        {
            if (ValidateIndex(optionIndex))
            {
                const ShaderOptionDescriptor& option = m_layout->GetShaderOption(optionIndex);
                if (!option.Set(*this, valueName))
                {
                    return false;
                }

                return true;
            }
            else
            {
                return false;
            }
        }

        bool ShaderOptionGroup::SetValue(ShaderOptionIndex optionIndex, ShaderOptionValue valueIndex)
        {
            if (ValidateIndex(optionIndex))
            {
                const ShaderOptionDescriptor& option = m_layout->GetShaderOption(optionIndex);
                if (!option.Set(*this, valueIndex))
                {
                    return false;
                }

                return true;
            }
            else
            {
                return false;
            }
        }

        ShaderOptionValue ShaderOptionGroup::GetValue(ShaderOptionIndex optionIndex) const
        {
            if (ValidateIndex(optionIndex))
            {
                const ShaderOptionDescriptor& option = m_layout->GetShaderOption(optionIndex);
                return option.Get(*this);
            }
            else
            {
                return ShaderOptionValue{};
            }
        }

        bool ShaderOptionGroup::ClearValue(const Name& optionName)
        {
            ShaderOptionIndex index;

            if (GetShaderOptionIndex(optionName, index))
            {
                return ClearValue(index);
            }
            else
            {
                return false;
            }
        }

        bool ShaderOptionGroup::ClearValue(ShaderOptionIndex optionIndex)
        {
            if (ValidateIndex(optionIndex))
            {
                const ShaderOptionDescriptor& option = m_layout->GetShaderOption(optionIndex);
                option.Clear(*this);
                return true;
            }

            return false;
        }

        void ShaderOptionGroup::SetAllToDefaultValues()
        {
            for (auto& option : m_layout->GetShaderOptions())
            {
                option.Set(*this, option.GetDefaultValue());                    
            }
        }

        void ShaderOptionGroup::SetUnspecifiedToDefaultValues()
        {
            for (auto& option : m_layout->GetShaderOptions())
            {
                if (!(m_id.m_mask & option.GetBitMask()).any())
                {
                    option.Set(*this, option.GetDefaultValue());                    
                }
            }
        }

        bool ShaderOptionGroup::IsFullySpecified() const
        {
            for (auto& option : m_layout->GetShaderOptions())
            {
                if (!(m_id.m_mask & option.GetBitMask()).any())
                {
                    return false;
                }
            }

            return true;
        }
        
        bool ShaderOptionGroup::IsEmpty() const
        {
            return m_id.IsEmpty();
        }

        ShaderVariantKey ShaderOptionGroup::GetShaderVariantKeyFallbackValue() const
        {
            // By default the fallback value is the search key
            auto fallbackValueKey = m_id.m_key;

            // However, we have to make sure that all options are set, opting for default values where missing
            for (auto& option : m_layout->GetShaderOptions())
            {
                if (!(m_id.m_mask & option.GetBitMask()).any())
                {
                    const auto value = option.FindValue(option.GetDefaultValue());

                    // This is an assert, not error, because the build system should have detected this situation earlier.
                    AZ_Assert(value.IsValid(), "Default value for shader option '%s' is invalid.", option.GetName().GetCStr());

                    option.Set(fallbackValueKey, value);
                }
            }

            return fallbackValueKey;
        }

        const ShaderVariantKey& ShaderOptionGroup::GetShaderVariantKey() const
        {
            return m_id.m_key;
        }

        const ShaderVariantKey& ShaderOptionGroup::GetShaderVariantMask() const
        {
            return m_id.m_mask;
        }

        const ShaderVariantId& ShaderOptionGroup::GetShaderVariantId() const
        {
            return m_id;
        }
        
        ShaderVariantKey& ShaderOptionGroup::GetShaderVariantKey()
        {
            return m_id.m_key;
        }

        ShaderVariantKey& ShaderOptionGroup::GetShaderVariantMask()
        {
            return m_id.m_mask;
        }

        const ShaderOptionGroupLayout* ShaderOptionGroup::GetShaderOptionLayout() const
        {
            return m_layout.get();
        }

        AZStd::string ShaderOptionGroup::ToString() const
        {
            AZStd::string s;
            for (int i = 0; i < GetShaderOptionLayout()->GetShaderOptionCount(); ++i)
            {
                ShaderOptionIndex index{i};
                const ShaderOptionDescriptor& option = GetShaderOptionLayout()->GetShaderOption(index);
                const ShaderOptionValue& value = GetValue(index);
                if (value.IsNull())
                {
                    s += AZStd::string::format("%s=?, ", option.GetName().GetCStr());
                }
                else
                {
                    //[GFX TODO][ATOM-3481] Report the names of enum options instead of numeric values. This depends on storing Names in NameIdReflectionMap.
                    s += AZStd::string::format("%s=%d, ", option.GetName().GetCStr(), value.GetIndex());
                }
            }

            // Remove the trailing ", " from the last shader option in the string
            static const size_t separateLength = 2;
            if (s.size() >= separateLength)
            {
                s.resize(s.size() - separateLength);
            }
            return s;
        }

        const AZStd::vector<AZ::RPI::ShaderOptionDescriptor>& ShaderOptionGroup::GetShaderOptionDescriptors() const
        {
            return m_layout->GetShaderOptions();
        }

    } // namespace RPI
} // namespace AZ
