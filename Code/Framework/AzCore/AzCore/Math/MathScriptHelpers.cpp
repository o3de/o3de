/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/Locale.h>

namespace AZ
{
    AZStd::string Vector3ToString(const Vector3& v)
    {
        AZ::Locale::ScopedSerializationLocale locale; // Vector3 <---> string interpreted in the "C" Locale.

        return AZStd::string::format("(x=%.7f,y=%.7f,z=%.7f)", static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()));
    }


    AZStd::string Vector4ToString(const Vector4& v)
    {
        AZ::Locale::ScopedSerializationLocale locale; // Vector4 <---> string interpreted in the "C" Locale.

        return AZStd::string::format("(x=%.7f,y=%.7f,z=%.7f,w=%.7f)", static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()), static_cast<float>(v.GetW()));
    }


    AZStd::string QuaternionToString(const Quaternion& v)
    {
        AZ::Locale::ScopedSerializationLocale locale; // Quaternion <---> string should be interpreted in the "C" Locale.

        return AZStd::string::format("(x=%.7f,y=%.7f,z=%.7f,w=%.7f)", static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()), static_cast<float>(v.GetW()));
    }


    AZStd::string TransformToString(const Transform& t)
    {
        return AZStd::string::format("%s,%s,%s,%s",
            Vector3ToString(t.GetBasisX()).c_str(), Vector3ToString(t.GetBasisY()).c_str(),
            Vector3ToString(t.GetBasisZ()).c_str(), Vector3ToString(t.GetTranslation()).c_str());
    }


    float GetTransformEpsilon()
    {
        // For scale, rotation, and translation, use a larger epsilon to accommodate precision loss.
        // Note: This will likely become configurable through serialization attributes, but for now we'll
        // apply globally to vector and transform types.
        return 0.0001f;
    }


    size_t UuidSerializer::Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/)
    {
        AZ_UNUSED(isDataBigEndian);

        const Uuid* uuidPtr = reinterpret_cast<const Uuid*>(classPtr);

        return static_cast<size_t>(stream.Write(AZStd::ranges::size(*uuidPtr), AZStd::ranges::data(*uuidPtr)));
    }


    size_t UuidSerializer::DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian)
    {
        (void)isDataBigEndian;

        if (in.GetLength() < 16)
        {
            return 0;
        }

        Uuid value;
        in.Read(AZStd::ranges::size(value), AZStd::ranges::data(value));

        char str[128];
        value.ToString(str, 128);
        AZStd::string outText = str;

        return static_cast<size_t>(out.Write(outText.size(), outText.data()));
    }


    size_t UuidSerializer::TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian)
    {
        (void)textVersion;
        (void)isDataBigEndian;
        AZ_Assert(textVersion == 0, "Unknown char, short, int version");

        Uuid uuid = Uuid::CreateString(text);
        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        return static_cast<size_t>(stream.Write(AZStd::ranges::size(uuid), AZStd::ranges::data(uuid)));
    }


    bool UuidSerializer::Load(void* classPtr, IO::GenericStream& stream, unsigned int, bool isDataBigEndian)
    {
        const int32_t GuidSizeBytes = 16;

        (void)isDataBigEndian;
        if (stream.GetLength() < GuidSizeBytes)
        {
            return false;
        }

        Uuid* uuidPtr = reinterpret_cast<Uuid*>(classPtr);
        if (stream.Read(GuidSizeBytes, reinterpret_cast<void*>(AZStd::ranges::data(*uuidPtr))) == GuidSizeBytes)
        {
            return true;
        }

        return false;
    }


    bool UuidSerializer::CompareValueData(const void* lhs, const void* rhs)
    {
        return SerializeContext::EqualityCompareHelper<AZ::Uuid>::CompareValues(lhs, rhs);
    }


    size_t FloatArrayTextSerializer::DataToText(const float* floats, size_t numFloats, char* textBuffer, size_t textBufferSize, bool isDataBigEndian)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for printf-ing floats to be invariant

        size_t numWritten = 0;
        for (size_t i = 0; i < numFloats; ++i)
        {
            float value = floats[i];
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            int result;
            char* textData = &textBuffer[numWritten];
            size_t textDataSize = textBufferSize - numWritten;
            if (i != 0)
            {
                result = azsnprintf(textData, textDataSize, " %.7f", value);
            }
            else
            {
                result = azsnprintf(textData, textDataSize, "%.7f", value);
            }
            if (result == -1)
            {
                break;
            }
            numWritten += result;
        }
        return numWritten ? numWritten + 1 : 0;
    }


    size_t FloatArrayTextSerializer::TextToData(const char* text, float* floats, size_t numFloats, bool isDataBigEndian)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // for parsing floats via strtod to be invariant

        size_t nextNumberIndex = 0;
        while (text != nullptr && nextNumberIndex < numFloats)
        {
            char* end;
            floats[nextNumberIndex] = static_cast<float>(strtod(text, &end));
            AZ_SERIALIZE_SWAP_ENDIAN(floats[nextNumberIndex], isDataBigEndian);
            text = end;
            ++nextNumberIndex;
        }
        return nextNumberIndex * sizeof(float);
    }
}
