#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            enum class ColorChannel : AZ::u8
            {
                Red = 0,
                Green,
                Blue,
                Alpha
            };

            struct Color
            {
                union
                {
                    struct
                    {
                        float red;
                        float green;
                        float blue;
                        float alpha;
                    };
                    float m_channels[4];
                };

                Color() = default;
                Color(float r, float g, float b, float a)
                    : red(r), green(g), blue(b), alpha(a)
                {
                }

                float GetChannel(ColorChannel channel) const
                {
                    return m_channels[static_cast<AZ::u8>(channel)];
                }

                bool IsClose(const Color& c, float tolerance = AZ::Constants::Tolerance) const
                {
                    return (c - (*this)).Abs().IsLessEqualThan(tolerance);
                }

                bool IsLessEqualThan(float tolerance) const
                {
                    return AZStd::all_of(AZStd::begin(m_channels), AZStd::end(m_channels), [tolerance] (float channel) { return channel <= tolerance; } );
                }

                Color& Abs()
                {
                    for (float& channel : m_channels)
                    {
                        channel = std::abs(channel);
                    }
                    return *this;
                }

                Color operator-(const Color& rhs) const
                {
                    return Color(red - rhs.red, green - rhs.green, blue - rhs.blue, alpha - rhs.alpha);
                }
            };

            class IMeshVertexColorData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IMeshVertexColorData, "{27659F76-1245-4549-87A6-AF4E8B94CD51}", IGraphObject);

                virtual ~IMeshVertexColorData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual const AZ::Name& GetCustomName() const = 0;

                virtual size_t GetCount() const = 0;
                virtual const Color& GetColor(size_t index) const = 0;
            };
        }  // DataTypes
    }  // SceneAPI

    AZ_TYPE_INFO_SPECIALIZE(SceneAPI::DataTypes::Color, "{937E3BF8-5204-4D40-A8DA-C8F083C89F9F}");

}  // AZ

namespace AZStd
{
    template<>
    struct hash<AZ::SceneAPI::DataTypes::Color>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::SceneAPI::DataTypes::Color& value) const
        {
            result_type hash = 0;

            hash_combine(hash, value.red);
            hash_combine(hash, value.green);
            hash_combine(hash, value.blue);
            hash_combine(hash, value.alpha);

            return hash;
        }
    };
}
