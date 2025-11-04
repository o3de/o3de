/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/ColorSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonColorSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonColorSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Color>() == outputValueTypeId,
            "Unable to deserialize Color to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        Color* color = reinterpret_cast<Color*>(outputValue);
        AZ_Assert(color, "Output value for JsonColorSerializer can't be null.");

        if (IsExplicitDefault(inputValue))
        {
            *color = Color::CreateZero();
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Color value set to default of zero.");
        }

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadFloatArray(*color, inputValue, LoadAlpha::IfAvailable, context);
        case rapidjson::kObjectType:
            return LoadObject(*color, inputValue, context);
        
        case rapidjson::kStringType:
            [[fallthrough]];
        case rapidjson::kNumberType:
            [[fallthrough]];
        case rapidjson::kNullType:
            [[fallthrough]];
        case rapidjson::kFalseType:
            [[fallthrough]];
        case rapidjson::kTrueType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. Colors can only be read from arrays or objects.");
        
        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                "Unknown json type encountered in Color.");
        }
    }

    JsonSerializationResult::Result JsonColorSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Color>() == valueTypeId, "Unable to serialize Color to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const Color* color = reinterpret_cast<const Color*>(inputValue);
        AZ_Assert(color, "Input value for JsonColorSerializer can't be null.");
        const Color* defaultColor = reinterpret_cast<const Color*>(defaultValue);

        if (!context.ShouldKeepDefaults() && defaultColor && *color == *defaultColor)
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default Color used.");
        }

        outputValue.SetArray();
        outputValue.PushBack(color->GetR(), context.GetJsonAllocator());
        outputValue.PushBack(color->GetG(), context.GetJsonAllocator());
        outputValue.PushBack(color->GetB(), context.GetJsonAllocator());

        if (context.ShouldKeepDefaults() || !defaultColor || (defaultColor && color->GetA() != defaultColor->GetA()))
        {
            outputValue.PushBack(color->GetA(), context.GetJsonAllocator());
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Color successfully stored.");
        }
        else
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::PartialDefaults, "Color partially stored.");
        }
    }

    auto JsonColorSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadObject(Color& output, const rapidjson::Value& inputValue, 
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        if (IsExplicitDefault(inputValue))
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed,
                "Default value for color provided so no change was made.");
        }

        JSR::ResultCode result = JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported);
        auto it = inputValue.MemberEnd() - 1;
        while (true)
        {
            // Note that the order for checking is important because RGBA will be accepted for RGB if RGBA isn't tested first.
            if (azstricmp(it->name.GetString(), "RGBA8") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedContextPath subPath(context, "RGBA8");
                    result = LoadByteArray(output, it->value, true, context);
                }
            }
            else if (azstricmp(it->name.GetString(), "RGB8") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedContextPath subPath(context, "RGB8");
                    result = LoadByteArray(output, it->value, false, context);
                }
            }
            else if (azstricmp(it->name.GetString(), "RGBA") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedContextPath subPath(context, "RGBA");
                    result = LoadFloatArray(output, it->value, LoadAlpha::Yes, context);
                }
            }
            else if (azstricmp(it->name.GetString(), "RGB") == 0)
            {
                if (it->value.IsArray())
                {
                    ScopedContextPath subPath(context, "RGB");
                    result = LoadFloatArray(output, it->value, LoadAlpha::No, context);
                }
            }
            else if (azstricmp(it->name.GetString(), "HEXA") == 0)
            {
                if (it->value.IsString())
                {
                    ScopedContextPath subPath(context, "HEXA");
                    result = LoadHexString(output, it->value, true, context);
                }
            }
            else if (azstricmp(it->name.GetString(), "HEX") == 0)
            {
                if (it->value.IsString())
                {
                    ScopedContextPath subPath(context, "HEX");
                    result = LoadHexString(output, it->value, false, context);
                }
            }
            else if (it != inputValue.MemberBegin())
            {
                --it;
                continue;
            }

            return context.Report(result, result.GetProcessing() != JSR::Processing::Halted ?
                "Successfully read color." : "Problem encountered while reading color.");
        }
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadFloatArray(Color& output, const rapidjson::Value& inputValue,
        LoadAlpha loadAlpha, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        rapidjson::SizeType arraySize = inputValue.Size();
        rapidjson::SizeType minimumLength = loadAlpha == LoadAlpha::Yes ? 4 : 3;
        float channels[4];
        if (arraySize < minimumLength)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Not enough numbers in array to load color from.");
        }

        const rapidjson::Value& redValue = inputValue[0];
        if (redValue.IsDouble())
        {
            channels[0] = aznumeric_caster(redValue.GetDouble());
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Red channel couldn't be read because the value wasn't a decimal number.");
        }

        const rapidjson::Value& greenValue = inputValue[1];
        if (greenValue.IsDouble())
        {
            channels[1] = aznumeric_caster(greenValue.GetDouble());
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Green channel couldn't be read because the value wasn't a decimal number.");
        }

        const rapidjson::Value& blueValue = inputValue[2];
        if (blueValue.IsDouble())
        {
            channels[2] = aznumeric_caster(blueValue.GetDouble());
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Blue channel couldn't be read because the value wasn't a decimal number.");
        }

        if (loadAlpha == LoadAlpha::Yes || (loadAlpha == LoadAlpha::IfAvailable && arraySize >= 4))
        {
            const rapidjson::Value& alphaValue = inputValue[3];
            if (alphaValue.IsDouble())
            {
                channels[3] = aznumeric_caster(alphaValue.GetDouble());
                minimumLength = 4; // Set to 4 in case the alpha was optional but present. This is needed in order to report the correct result.
            }
            else
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Alpha channel couldn't be read because the value wasn't a decimal number.");
            }
        }
        else
        {
            channels[3] = output.GetA();
        }

        output = Color::CreateFromFloat4(channels);

        JSR::Outcomes outcome = (minimumLength == 3 && loadAlpha == LoadAlpha::IfAvailable) ? JSR::Outcomes::PartialDefaults : JSR::Outcomes::Success;
        return context.Report(JSR::Tasks::ReadField, outcome, "Successfully loaded color from decimal array.");
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadByteArray(Color& output, const rapidjson::Value& inputValue,
        bool loadAlpha, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        rapidjson::SizeType arraySize = inputValue.Size();
        rapidjson::SizeType expectedSize = loadAlpha ? 4 : 3;
        uint32_t color = 0; // Stored as AABBGGRR
        if (arraySize < expectedSize)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Not enough numbers in array to load color from.");
        }

        const rapidjson::Value& redValue = inputValue[0];
        if (redValue.IsInt64())
        {
            color = ClampToByteColorRange(redValue.GetInt64());
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Red channel couldn't be read because the value wasn't a byte number.");
        }

        const rapidjson::Value& greenValue = inputValue[1];
        if (greenValue.IsInt64())
        {
            color |= ClampToByteColorRange(greenValue.GetInt64()) << 8;
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Green channel couldn't be read because the value wasn't a byte number.");
        }

        const rapidjson::Value& blueValue = inputValue[2];
        if (blueValue.IsInt64())
        {
            color |= ClampToByteColorRange(blueValue.GetInt64()) <<  16;
        }
        else
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Blue channel couldn't be read because the value wasn't a byte number.");
        }

        if (loadAlpha)
        {
            const rapidjson::Value& alphaValue = inputValue[3];
            if (alphaValue.IsInt64())
            {
                color |= ClampToByteColorRange(alphaValue.GetInt64()) << 24;
            }
            else
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Alpha channel couldn't be read because the value wasn't a byte number.");
            }
            output.FromU32(color);
        }
        else
        {
            float alpha = output.GetA(); // Don't use the byte value here as it's a lot more imprecise than copying the float value.
            output.FromU32(color);
            output.SetA(alpha);
        }
        
        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully loaded color from byte array.");
    }

    JsonSerializationResult::Result JsonColorSerializer::LoadHexString(Color& output, const rapidjson::Value& inputValue,
        bool loadAlpha, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        size_t minimumLength = loadAlpha ? 8 : 6; // 8 for RRGGBBAA and 6 for RRGGBB
        
        AZStd::string_view text(inputValue.GetString(), inputValue.GetStringLength());
        if (text.length() < minimumLength)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "The hex text for color is too short.");
        }

        uint32_t color = 0;
        size_t hexDigitIndex;
        size_t length = AZStd::min(minimumLength, text.length());
        const char* front = text.begin();
        for (hexDigitIndex = 0; hexDigitIndex < length; ++hexDigitIndex)
        {
            color <<= 4;
            
            char character = *front;
            if (character >= '0' && character <= '9')
            {
                color |= character - '0';
            }
            else if (character >= 'a' && character <= 'f')
            {
                color |= character - 'a' + 10;
            }
            else if (character >= 'A' && character <= 'F')
            {
                color |= character - 'A' + 10;
            }
            else
            {
                break;
            }
            
            front++;
        }

        if (hexDigitIndex != minimumLength)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Failed to parse hex value. There might be an incorrect character in the string.");
        }

        if (loadAlpha)
        {
            output.SetR8((color >> 24) & 0xff);
            output.SetG8((color >> 16) & 0xff);
            output.SetB8((color >> 8) & 0xff);
            output.SetA8(color & 0xff);
        }
        else
        {
            output.SetR8((color >> 16) & 0xff);
            output.SetG8((color >> 8) & 0xff);
            output.SetB8(color & 0xff);
        }

        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully loaded color from hex string.");
    }

    uint32_t JsonColorSerializer::ClampToByteColorRange(int64_t value) const
    {
        return static_cast<uint32_t>(AZStd::clamp<int64_t>(value, 0, 255));
    }
}
