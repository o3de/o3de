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

#pragma once

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/CompressionMarshal.h>

namespace EMotionFX
{
    namespace Integration
    {
        namespace Network
        {
            /**
             * \brief A general storage for an anim graph parameter.
             */
            class AnimParameter
            {
            public:
                /**
                 * \brief String type is not supported because one should not be syncing strings over the network.
                 */
                enum class Type : AZ::u8
                {
                    Unsupported,
                    Float,
                    Bool,
                    Vector2,
                    Vector3,
                    Quaternion,
                };

                /**
                 * \brief A storage for all possible supported types in @AnimGraphNetSyncComponent
                 */
                union Value
                {
                    Value()
                    {
                        q = AZ::Quaternion::CreateZero();
                    }

                    float f;
                    bool b = false;
                    AZ::Vector2 v2;
                    AZ::Vector3 v3;
                    AZ::Quaternion q;
                };

                AnimParameter() : m_type(Type::Unsupported) {}

                Type m_type;
                Value m_value;

                AnimParameter(const AnimParameter& other)
                {
                    m_type = other.m_type;
                    CopyValue(other);
                }

                AnimParameter& operator=(const AnimParameter& other)
                {
                    m_type = other.m_type;
                    CopyValue(other);

                    return *this;
                }

                friend bool operator==(const AnimParameter& lhs, const AnimParameter& rhs)
                {
                    if (lhs.m_type != rhs.m_type)
                    {
                        return false;
                    }

                    switch (lhs.m_type)
                    {
                    case Type::Float:
                        return lhs.m_value.f == rhs.m_value.f;
                    case Type::Bool:
                        return lhs.m_value.b == rhs.m_value.b;
                    case Type::Vector2:
                        return lhs.m_value.v2 == rhs.m_value.v2;
                    case Type::Vector3:
                        return lhs.m_value.v3 == rhs.m_value.v3;
                    case Type::Quaternion:
                        return lhs.m_value.q == rhs.m_value.q;
                    default:
                        return true;
                    }
                }

            private:
                void CopyValue(const AnimParameter& other)
                {
                    switch (m_type)
                    {
                    case Type::Float:
                        m_value.f = other.m_value.f;
                        break;
                    case Type::Bool:
                        m_value.b = other.m_value.b;
                        break;
                    case Type::Vector2:
                        m_value.v2 = other.m_value.v2;
                        break;
                    case Type::Vector3:
                        m_value.v3 = other.m_value.v3;
                        break;
                    case Type::Quaternion:
                        m_value.q = other.m_value.q;
                        break;
                    default:
                        break;
                    }
                }
            };

            /**
             * \brief Custom GridMate throttler. See GridMate:: @BasicThrottle
             */
            class AnimParameterThrottler
            {
            public:
                bool WithinThreshold(const AnimParameter& newValue) const
                {
                    return m_baseline == newValue;
                }

                void UpdateBaseline(const AnimParameter& baseline)
                {
                    m_baseline = baseline;
                }

            private:
                AnimParameter m_baseline;
            };

            /**
             * \brief A custom GridMate marshaler.
             * 1 byte is spend on the type. And a variable number of bytes afterwards for the value.
             */
            class AnimParameterMarshaler
            {
            public:
                void Marshal(GridMate::WriteBuffer& wb, const AnimParameter& parameter)
                {
                    wb.Write(AZ::u8(parameter.m_type));

                    switch (parameter.m_type)
                    {
                    case AnimParameter::Type::Float:
                        wb.Write(parameter.m_value.f);
                        break;
                    case AnimParameter::Type::Bool:
                        wb.Write(parameter.m_value.b);
                        break;
                    case AnimParameter::Type::Vector2:
                        wb.Write(parameter.m_value.v2);
                        break;
                    case AnimParameter::Type::Vector3:
                        wb.Write(parameter.m_value.v3);
                        break;
                    case AnimParameter::Type::Quaternion:
                        wb.Write(parameter.m_value.q);
                        break;
                    default:
                        // other types are not supported
                        break;
                    }
                }

                void Unmarshal(AnimParameter& parameter, GridMate::ReadBuffer& rb)
                {
                    AZ::u8 type;
                    rb.Read(type);
                    parameter.m_type = static_cast<AnimParameter::Type>(type);

                    switch (parameter.m_type)
                    {
                    case AnimParameter::Type::Float:
                        rb.Read(parameter.m_value.f);
                        break;
                    case AnimParameter::Type::Bool:
                        rb.Read(parameter.m_value.b);
                        break;
                    case AnimParameter::Type::Vector2:
                        rb.Read(parameter.m_value.v2);
                        break;
                    case AnimParameter::Type::Vector3:
                        rb.Read(parameter.m_value.v3);
                        break;
                    case AnimParameter::Type::Quaternion:
                        rb.Read(parameter.m_value.q);
                        break;
                    default:
                        // other types are not supported
                        break;
                    }
                }
            };
            
            /**
             * \brief Custom marshaler for Animation node index that is used by Activate Nodes list
             */
            struct NodeIndexContainerMarshaler
            {
                void Marshal(GridMate::WriteBuffer& wb, const NodeIndexContainer& source) const
                {
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    m64.Marshal(wb, source.size()); // 1 byte most of the time (if the size is less than 127)
                    for (AZ::u32 item : source)
                    {
                        m32.Marshal(wb, item); // 1 byte most of the time (if the value is less than 127)
                    }
                }

                void Unmarshal(NodeIndexContainer& target, GridMate::ReadBuffer& rb) const
                {
                    target.clear();
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    AZ::u64 arraySize;
                    m64.Unmarshal(arraySize, rb);
                    target.resize(arraySize);

                    for (AZ::u64 i = 0; i < arraySize; ++i)
                    {
                        m32.Unmarshal(target[i], rb);
                    }
                }
            };

            /**
             * \brief Custom marshaler for Animation motion node information that is used by motion node playtime list
             */
            struct MotionNodePlaytimeContainerMarshaler
            {
                void Marshal(GridMate::WriteBuffer& wb, const MotionNodePlaytimeContainer& source) const
                {
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    m64.Marshal(wb, source.size());
                    for (const AZStd::pair<AZ::u32, float>& item : source)
                    {
                        m32.Marshal(wb, item.first);    // average of 1 byte
                        wb.Write(item.second);          // 4 bytes
                    }
                }

                void Unmarshal(MotionNodePlaytimeContainer& target, GridMate::ReadBuffer& rb) const
                {
                    target.clear();
                    GridMate::VlqU64Marshaler m64;
                    GridMate::VlqU32Marshaler m32;

                    AZ::u64 arraySize;
                    m64.Unmarshal(arraySize, rb);
                    target.resize(arraySize);

                    for (AZ::u64 i = 0; i < arraySize; ++i)
                    {
                        m32.Unmarshal(target[i].first, rb);
                        rb.Read(target[i].second);
                    }
                }
            };
        }
    } // namespace Integration
} // namespace EMotionFXAnimation
