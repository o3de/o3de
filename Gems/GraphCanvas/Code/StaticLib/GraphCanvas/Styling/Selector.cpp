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
#include <QDebug>

#include <AzCore/Component/EntityUtils.h>

#include <GraphCanvas/Styling/Selector.h>

#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/SelectorImplementations.h>

namespace GraphCanvas
{
    namespace Styling
    {
        void Selector::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<Selector>()
                ->Version(1)
                ->Field("Implementation", &Selector::m_actual)
                ;
        }

        const Selector Selector::Get(const AZStd::string& selector)
        {
            if (!selector.empty())
            {
                return aznew BasicSelector(selector);
            }
            return{};
        }

        Selector::Selector(SelectorImplementation * actual)
            : m_actual(actual)
        {
            if (m_actual == nullptr)
            {
                MakeNull();
            }
        }

        Selector::Selector(const Selector& other)
            : m_actual(other.m_actual ? other.m_actual->Clone() : nullptr)
        {
            if (m_actual == nullptr)
            {
                MakeNull();
            }
        }

        Selector& Selector::operator=(const Selector& other)
        {
            m_actual.reset(other.m_actual ? other.m_actual->Clone() : nullptr);

            if (m_actual == nullptr)
            {
                MakeNull();
            }

            return *this;
        }

        Selector & Selector::operator=(Selector && other)
        {
            m_actual = AZStd::move(other.m_actual);

            if (m_actual == nullptr)
            {
                MakeNull();
            }

            return *this;
        }

        void Selector::MakeDefault()
        {
            m_actual.reset(aznew DefaultSelector(m_actual.release()));
        }

        void Selector::MakeNull()
        {
            m_actual.reset(aznew NullSelector());
        }

        void SelectorImplementation::Reflect(AZ::ReflectContext * context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<SelectorImplementation>()
                ->Version(1)
                ;
        }

        AZStd::string SelectorToStringAccumulator(const AZStd::string& soFar, const Selector& selector)
        {
            return (soFar.empty() ? "" : soFar + ", ") + selector.ToString();
        }

        AZStd::string SelectorsToString(const AZStd::vector<Selector>& selectors)
        {
            return std::accumulate(selectors.cbegin(), selectors.cend(), AZStd::string(), SelectorToStringAccumulator);
        }

    }
}

