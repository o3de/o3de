/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/SelectorImplementations.h>

namespace GraphCanvas
{
    namespace Styling
    {
        namespace
        {
            AZStd::string BuildName(const AZStd::string& soFar, const Selector& selector)
            {
                return soFar + selector.ToString();
            }

            AZStd::string BuildNestedName(const AZStd::string& soFar, const Selector& selector)
            {
                return soFar + (soFar.empty() ? "" : " > ") + selector.ToString();
            }

            int SumComplexity(int accumulator, const Selector& selector)
            {
                return accumulator + selector.GetComplexity();
            }
        }

        /////////////////
        // NullSelector
        /////////////////

        void NullSelector::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<NullSelector, SelectorImplementation>()
                ->Version(1)
                ;
        }

        //////////////////
        // BasicSelector
        //////////////////

        class BasicSelectorEventHandler
            : public AZ::SerializeContext::IEventHandler
        {
        public:
            void OnWriteEnd(void* classPtr) override
            {
                BasicSelector* actual = reinterpret_cast<BasicSelector*>(classPtr);
                actual->m_hash = AZStd::hash<AZStd::string>()(actual->m_value);
            }
        };
        
        BasicSelector::BasicSelector(const AZStd::string& value /*AZStd::string*/)
            : m_value(value)
            , m_hash(AZStd::hash<AZStd::string>()(m_value))
        {
        }

        void BasicSelector::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<BasicSelector, SelectorImplementation>()
                ->Version(1)
                ->EventHandler<BasicSelectorEventHandler>()
                ->Field("Value", &BasicSelector::m_value)
                ;
        }

        bool BasicSelector::Matches(const AZ::EntityId& object) const
        {
            AZStd::vector<Selector> selectors;
            StyledEntityRequestBus::EventResult(selectors, object, &StyledEntityRequests::GetStyleSelectors);

            return AZStd::find_if(selectors.cbegin(), selectors.cend(), [this](const Selector& o) { return o == *this; }) != selectors.cend();
        }

        void DefaultSelector::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<DefaultSelector, SelectorImplementation>()
                ->Version(1)
                ->Field("Wrapped", &DefaultSelector::m_actual)
                ;
        }

        DefaultSelector::DefaultSelector(SelectorImplementation* actual)
            : m_actual(actual)
            , m_value("(" + actual->ToString() + ")")
        {
        }

        CompoundSelector::CompoundSelector(SelectorVector&& parts)
            : m_parts(std::move(parts))
            , m_complexity(std::accumulate(m_parts.cbegin(), m_parts.cend(), int{}, SumComplexity))
            , m_value(std::accumulate(m_parts.cbegin(), m_parts.cend(), AZStd::string(), BuildName))
        {
        }

        void CompoundSelector::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<CompoundSelector, SelectorImplementation>()
                ->Version(1)
                ->Field("Parts", &CompoundSelector::m_parts)
                ->Field("Complexity", &CompoundSelector::m_complexity)
                ->Field("Value", &CompoundSelector::m_value)
                ;
        }

        bool CompoundSelector::Matches(const AZ::EntityId& object) const
        {
            SelectorVector selectors;
            StyledEntityRequestBus::EventResult(selectors, object, &StyledEntityRequests::GetStyleSelectors);

            return std::all_of(m_parts.cbegin(), m_parts.cend(), [&](const Selector& s) {
                return std::find(selectors.cbegin(), selectors.cend(), s) != selectors.cend();
            });
        }

        NestedSelector::NestedSelector(SelectorVector&& parts)
            : m_parts(std::move(parts))
            , m_complexity(std::accumulate(m_parts.cbegin(), m_parts.cend(), int{}, SumComplexity))
            , m_value(std::accumulate(m_parts.cbegin(), m_parts.cend(), AZStd::string(), BuildNestedName))
        {
        }

        void NestedSelector::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<NestedSelector, SelectorImplementation>()
                ->Version(1)
                ->Field("Parts", &NestedSelector::m_parts)
                ->Field("Complexity", &NestedSelector::m_complexity)
                ->Field("Value", &NestedSelector::m_value)
                ;
        }

        bool NestedSelector::Matches(const AZ::EntityId& object) const
        {
            AZ::EntityId currentObject = object;

            auto selector = m_parts.crbegin();

            for (; selector != m_parts.crend() && currentObject.IsValid(); ++selector)
            {
                bool matches = selector->Matches(currentObject);
                if (!matches)
                {
                    return false;
                }

                StyledEntityRequestBus::EventResult(currentObject, currentObject, &StyledEntityRequests::GetStyleParent);
            }

            return true;
        }

    }
}

