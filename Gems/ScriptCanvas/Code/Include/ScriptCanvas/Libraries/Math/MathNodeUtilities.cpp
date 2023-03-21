/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MathNodeUtilities.h"

#include <AzCore/Module/Environment.h>
#include <AzCore/std/algorithm.h>
#include <random>

namespace MathNodeUtilitiesCPP
{
    class RandomEngineInternal
    {
    public:
        AZ_TYPE_INFO(RandomEngineInternal, "{94DF8BDF-FF9F-434B-BF0B-FC215EA44069}");
        AZ_CLASS_ALLOCATOR(RandomEngineInternal, AZ::SystemAllocator);

        std::mt19937 m_randomEngine;

        RandomEngineInternal()
            : m_randomEngine(std::random_device()())
        {}
    };

    static const char* k_randomEngineName = "ScriptCanvasRandomEngine";
    static AZ::EnvironmentVariable<RandomEngineInternal> s_randomEngine;
}

namespace ScriptCanvas
{
    namespace MathNodeUtilities
    {
        void RandomEngineInit()
        {
            AZ_Assert(!MathNodeUtilitiesCPP::s_randomEngine.IsConstructed(), "random engine is already initialized");
            MathNodeUtilitiesCPP::s_randomEngine = AZ::Environment::CreateVariable<MathNodeUtilitiesCPP::RandomEngineInternal>(MathNodeUtilitiesCPP::k_randomEngineName);
        }

        void RandomEngineReset()
        {
            MathNodeUtilitiesCPP::s_randomEngine.Reset();
        }

        Data::NumberType GetRandom(Data::NumberType lhs, Data::NumberType rhs)
        {
            if (AZ::IsClose(lhs, rhs, std::numeric_limits<Data::NumberType>::epsilon()))
            {
                return lhs;
            }

            const auto minAndMax = AZStd::minmax(lhs, rhs);
            std::uniform_real_distribution<Data::NumberType> dis(minAndMax.first, minAndMax.second);
            auto randomEngineVar = AZ::Environment::FindVariable<MathNodeUtilitiesCPP::RandomEngineInternal>(MathNodeUtilitiesCPP::k_randomEngineName);
            AZ_Assert(randomEngineVar.IsConstructed(), "random engine is not initialized");
            return dis(randomEngineVar->m_randomEngine);
        }

        AZ::s64 GetRandom(AZ::s64 lhs, AZ::s64 rhs)
        {
            if (lhs == rhs)
            {
                return lhs;
            }

            const auto minAndMax = AZStd::minmax(lhs, rhs);
            std::uniform_int_distribution<AZ::s64> dis(minAndMax.first, minAndMax.second);
            auto randomEngineVar = AZ::Environment::FindVariable<MathNodeUtilitiesCPP::RandomEngineInternal>(MathNodeUtilitiesCPP::k_randomEngineName);
            AZ_Assert(randomEngineVar.IsConstructed(), "random engine is not initialized");
            return dis(randomEngineVar->m_randomEngine);
        }

    }
}

