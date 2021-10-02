/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/functional.h>

namespace AZ::IO
{
    template <typename PathType>
    struct PathSerializer
        : public SerializeContext::IDataSerializer
    {
    public:
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool) override
        {
            PathType outPath;
            outPath.Native().resize_no_construct(in.GetLength());
            in.Read(outPath.Native().size(), outPath.Native().data());

            return static_cast<size_t>(out.Write(outPath.Native().size(), outPath.Native().c_str()));
        }

        size_t TextToData(const char* text, unsigned int, IO::GenericStream& stream, bool) override
        {
            return static_cast<size_t>(stream.Write(strlen(text), reinterpret_cast<const void*>(text)));
        }

        size_t Save(const void* classPtr, IO::GenericStream& stream, bool) override
        {
            /// Save paths out using the PosixPathSeparator
            PathType path(reinterpret_cast<const PathType*>(classPtr)->Native(), AZ::IO::PosixPathSeparator);
            path.MakePreferred();

            return static_cast<size_t>(stream.Write(path.Native().size(), path.c_str()));
        }

        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int, bool) override
        {
            // Normalize the path load
            auto path = reinterpret_cast<PathType*>(classPtr);

            path->Native().resize_no_construct(stream.GetLength());
            stream.Read(path->Native().size(), path->Native().data());
            *path = path->LexicallyNormal();

            return true;
        }

        bool CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<Path>::CompareValues(lhs, rhs);
        }
    };

    void PathReflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<Path>()
                ->Serializer(AZ::SerializeContext::IDataSerializerPtr{ new PathSerializer<Path>{},
                AZ::SerializeContext::IDataSerializer::CreateDefaultDeleteDeleter() })
                ;

            serializeContext->Class<FixedMaxPath>()
                ->Serializer(AZ::SerializeContext::IDataSerializerPtr{ new PathSerializer<FixedMaxPath>{},
                AZ::SerializeContext::IDataSerializer::CreateDefaultDeleteDeleter() })
                ;
        }
    }
}
