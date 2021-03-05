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
#include <Atom/RHI.Reflect/Scissor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    namespace RHI
    {
        void Scissor::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Scissor>()
                    ->Version(1)
                    ->Field("m_minX", &Scissor::m_minX)
                    ->Field("m_minY", &Scissor::m_minY)
                    ->Field("m_maxX", &Scissor::m_maxX)
                    ->Field("m_maxY", &Scissor::m_maxY);
            }
        }

        Scissor::Scissor(
            int32_t minX,
            int32_t minY,
            int32_t maxX,
            int32_t maxY)
            : m_minX(minX)
            , m_minY(minY)
            , m_maxX(maxX)
            , m_maxY(maxY)
        {}

        Scissor Scissor::GetScaled(
            float normalizedMinX,
            float normalizedMinY,
            float normalizedMaxX,
            float normalizedMaxY) const
        {
            Scissor scissor;
            scissor.m_minX = static_cast<int32_t>(Lerp(static_cast<float>(m_minX), static_cast<float>(m_maxX), normalizedMinX));
            scissor.m_maxX = static_cast<int32_t>(Lerp(static_cast<float>(m_minX), static_cast<float>(m_maxX), normalizedMaxX));
            scissor.m_minY = static_cast<int32_t>(Lerp(static_cast<float>(m_minY), static_cast<float>(m_maxY), normalizedMinY));
            scissor.m_maxY = static_cast<int32_t>(Lerp(static_cast<float>(m_minY), static_cast<float>(m_maxY), normalizedMaxY));
            return scissor;
        }

        Scissor Scissor::CreateNull()
        {
            return Scissor{0, 0, -1, -1};
        }

        bool Scissor::IsNull() const
        {
            return ((m_minX > m_maxX) || (m_minY > m_maxY));
        }

    } // namespace RHI
} // namespace AZ