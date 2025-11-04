/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantId::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantId>()
                    ->Version(0)
                    ->Field("Key", &ShaderVariantId::m_key)
                    ->Field("Mask", &ShaderVariantId::m_mask)
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ShaderVariantId>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ->Method("Equal", &ShaderVariantId::operator==)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ->Method("IsEmpty", &ShaderVariantId::IsEmpty)
                    ;
            }
        }

        ShaderVariantId& ShaderVariantId::Reset()
        {
            m_key.reset();
            m_mask.reset();
            return *this;
        }

        bool ShaderVariantId::operator==(const ShaderVariantId& other) const
        {
            return ShaderVariantIdComparator::Compare(*this, other) == 0;
        }

        bool ShaderVariantId::operator!=(const ShaderVariantId& other) const
        {
            return ShaderVariantIdComparator::Compare(*this, other) != 0;
        }

        bool ShaderVariantId::operator<(const ShaderVariantId& other) const
        {
            return ShaderVariantIdComparator::Compare(*this, other) == -1;
        }

        bool ShaderVariantId::operator>(const ShaderVariantId& other) const
        {
            return ShaderVariantIdComparator::Compare(*this, other) == 1;
        }

        bool ShaderVariantId::operator<=(const ShaderVariantId& other) const
        {
            return ShaderVariantIdComparator::Compare(*this, other) != 1;
        }

        bool ShaderVariantId::operator>=(const ShaderVariantId& other) const
        {
            return ShaderVariantIdComparator::Compare(*this, other) != -1;
        }

        bool ShaderVariantId::IsEmpty() const
        {
            return m_mask.none();
        }

        ShaderVariantSearchResult::ShaderVariantSearchResult(ShaderVariantStableId stableId, uint32_t dynamicOptionCount)
            : m_shaderVariantStableId (stableId)
            , m_dynamicOptionCount(dynamicOptionCount)
        {

        }

        ShaderVariantStableId ShaderVariantSearchResult::GetStableId() const
        {
            return m_shaderVariantStableId;
        }

        bool ShaderVariantSearchResult::IsRoot() const
        {
            return m_shaderVariantStableId == RootShaderVariantStableId;
        }

        bool ShaderVariantSearchResult::IsFullyBaked() const
        {
            return (m_dynamicOptionCount == 0);
        }

        uint32_t ShaderVariantSearchResult::GetDynamicOptionCount() const
        {
            return m_dynamicOptionCount;
        }


        int ShaderVariantKeyComparator::Compare(const ShaderVariantKey& lhs, const ShaderVariantKey& rhs)
        {
            if constexpr (ShaderVariantKeyBitCount <= 64)
            {
                return CompareSmallKey(lhs, rhs);
            }
            else
            {
                return CompareLargeKey(lhs, rhs);
            }
        }
        int ShaderVariantKeyComparator::CompareSmallKey(const ShaderVariantKey& lhs, const ShaderVariantKey& rhs)
        {
            // Works for keys smaller than 64-bits. Asserts that bits higher than 64 aren't set.
            return lhs.to_ullong() < rhs.to_ullong() ?
                -1 : lhs.to_ullong() == rhs.to_ullong() ? 0 : 1;
        }
        int ShaderVariantKeyComparator::CompareLargeKey(const ShaderVariantKey& lhs, const ShaderVariantKey& rhs)
        {
            // Walks the from most to least significant and compares words. Matching upper words
            // requires searching the next lower word.
            for (size_t i = lhs.num_words() - 1; i != static_cast<size_t>(-1); --i)
            {
                const uint32_t lhsWord = lhs.data()[i];
                const uint32_t rhsWord = rhs.data()[i];
                if (lhsWord != rhsWord)
                {
                    return lhsWord < rhsWord ? -1 : 1;
                }
            }
            return 0;
        }
        bool ShaderVariantKeyComparator::operator () (const ShaderVariantKey& lhs, const ShaderVariantKey& rhs) const
        {
            return ShaderVariantKeyComparator::Compare(lhs, rhs) < 0;
        }
        int ShaderVariantIdComparator::Compare(const ShaderVariantId& lhs, const ShaderVariantId& rhs)
        {
            auto compareMask = ShaderVariantKeyComparator::Compare(lhs.m_mask, rhs.m_mask);
            if (compareMask != 0)
            {
                return compareMask;
            }
            return ShaderVariantKeyComparator::Compare(lhs.m_key & lhs.m_mask, rhs.m_key & rhs.m_mask);
        }
        bool ShaderVariantIdComparator::operator () (const ShaderVariantId& lhs, const ShaderVariantId& rhs) const
        {
            return ShaderVariantIdComparator::Compare(lhs, rhs) < 0;
        }
    } // namespace RPI
} // namespace AZ
