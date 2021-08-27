/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class Aabb;
    class Vector3;
    class Vector4;
    class Quaternion;
    class Transform;

    AZStd::string Vector3ToString(const Vector3& v);
    AZStd::string Vector4ToString(const Vector4& v);
    AZStd::string QuaternionToString(const Quaternion& v);
    AZStd::string TransformToString(const Transform& t);

    float GetTransformEpsilon();

    template<typename t_Argument, typename t_Constructed>
    bool ConstructOnTypedArgument(t_Constructed& constructed, ScriptDataContext& dc, int index)
    {
        if (dc.IsClass<t_Argument>(index))
        {
            t_Argument argument;
            dc.ReadArg(index, argument);
            new (&constructed) t_Constructed(argument);
            return true;
        }
        else
        {
            return false;
        }
    }

    class UuidSerializer
        : public SerializeContext::IDataSerializer
    {
        //! Store the class data into a binary buffer.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override;

        //! Convert binary data to text.
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override;

        //! Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed.
        size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override;

        //! Load the class data from a stream.
        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override;

        bool CompareValueData(const void* lhs, const void* rhs) override;
    };

    class FloatArrayTextSerializer
    {
    public:
        static size_t DataToText(const float* floats, size_t numFloats, char* textBuffer, size_t textBufferSize, bool isDataBigEndian);
        static size_t TextToData(const char* text, float* floats, size_t numFloats, bool isDataBigEndian);
    };

    //! Custom template to cover all fundamental AZMATH classes.
    template <class T, T CreateFromFloats(const float*), void (T::* StoreToFloat)(float*) const, float GetEpsilon(), size_t NumFloats = sizeof(T) / sizeof(float)>
    class FloatBasedContainerSerializer
        : public SerializeContext::IDataSerializer
    {
        //! Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            float tempData[NumFloats];

            (reinterpret_cast<const T*>(classPtr)->*StoreToFloat)(tempData);
            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(tempData); ++i)
            {
                AZ_SERIALIZE_SWAP_ENDIAN(tempData[i], isDataBigEndian);
            }

            return static_cast<size_t>(stream.Write(sizeof(tempData), reinterpret_cast<void*>(&tempData)));
        }

        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            if (in.GetLength() < NumFloats * sizeof(float))
            {
                return 0;
            }

            float tempData[NumFloats];
            in.Read(NumFloats * sizeof(float), reinterpret_cast<void*>(&tempData));
            char textBuffer[256];
            FloatArrayTextSerializer::DataToText(tempData, NumFloats, textBuffer, sizeof(textBuffer), isDataBigEndian);
            AZStd::string outText = textBuffer;

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)textVersion;

            float floats[NumFloats];
            size_t numBytesRead = FloatArrayTextSerializer::TextToData(text, floats, NumFloats, isDataBigEndian);
            stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            return static_cast<size_t>(stream.Write(numBytesRead, reinterpret_cast<void*>(&floats)));
        }

        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian = false) override
        {
            AZ_UNUSED(version);
            float tempData[NumFloats];
            if (stream.GetLength() < sizeof(tempData))
            {
                return false;
            }

            stream.Read(sizeof(tempData), reinterpret_cast<void*>(tempData));

            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(tempData); ++i)
            {
                AZ_SERIALIZE_SWAP_ENDIAN(tempData[i], isDataBigEndian);
            }
            *reinterpret_cast<T*>(classPtr) = CreateFromFloats(tempData);
            return true;
        }

        bool CompareValueData(const void* lhs, const void* rhs) override
        {
            float tempDataLhs[NumFloats];
            float tempDataRhs[NumFloats];
            (reinterpret_cast<const T*>(lhs)->*StoreToFloat)(tempDataLhs);
            (reinterpret_cast<const T*>(rhs)->*StoreToFloat)(tempDataRhs);

            const float epsilon = GetEpsilon();

            bool match = true;
            for (size_t i = 0; i < NumFloats && match; ++i)
            {
                match &= AZ::IsClose(tempDataLhs[i], tempDataRhs[i], epsilon);
            }

            return match;
        }
    };
}
