/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/PathSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/functional.h>
#include <AzCore/StringFunc/StringFunc.h>

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
            auto posixPathString{ reinterpret_cast<const PathType*>(classPtr)->AsPosix() };
            return static_cast<size_t>(stream.Write(posixPathString.size(), posixPathString.c_str()));
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

    void PathViewBehaviorReflect(AZ::BehaviorContext* behaviorContext)
    {
        // Replicates the python 3 pathlib API: https://docs.python.org/3/library/pathlib.html
        behaviorContext->Class<PathView>("PurePathView")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, "pathlib")
            ->Attribute(AZ::Script::Attributes::Category, "FilesystemPath")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
            // string_view constructor
            ->template Constructor<AZStd::string_view>()
            // string_view constructor with preferred path separator
            ->template Constructor<AZStd::string_view, const char>()
            // const char* constructor
            ->template Constructor<const typename PathView::value_type*>()
            // const char* constructor with preferred path separator
            ->template Constructor<const typename PathView::value_type*, const char>()
            // preferred path separator only constructor
            ->template Constructor<const char>()
            ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
            ->Method("__truediv__", [](const PathView* thisPtr, const AZ::IO::PathView* src) -> PathView
            {
                    return FixedMaxPath(*thisPtr) / *src;
            })
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)
            ->Method("__hash__", [](const PathView* thisPtr) -> AZ::u64 { return static_cast<AZ::u64>(AZ::IO::hash_value(*thisPtr)); })
            ->Method("__str__", &PathView::String)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
            ->Method("__eq__", [](const PathView* left, const PathView* right) -> bool { return *left == *right; })
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
            ->Method("__lt__", [](const PathView* left, const PathView* right) -> bool { return *left < *right; })
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)
            ->Method("__ne__", [](const PathView* left, const PathView* right) -> bool { return *left != *right; })
            ->Method("__gt__", [](const PathView* left, const PathView* right) -> bool { return *left > *right; })
            ->Method("__le__", [](const PathView* left, const PathView* right) -> bool { return *left <= *right; })
            ->Method("__ge__", [](const PathView* left, const PathView* right) -> bool { return *left >= *right; })
            // No setters for the path class properties
            ->Property("parts", [](const PathView* thisPtr) -> AZStd::vector<PathView>
            {
                // Initialize the parts with the RootPath value which includes any drive letters and root directory
                AZStd::vector<PathView> parts{ thisPtr->RootPath()};
                // Append each relative path part using the Path iterator code
                PathView postRootNamePath = thisPtr->RelativePath();
                parts.insert(parts.end(), postRootNamePath.begin(), postRootNamePath.end());
                return parts;
            }, nullptr)
            ->Property("drive", [](const PathView* thisPtr) -> AZStd::string { return thisPtr->RootName().String(); }, nullptr)
            ->Property("root", [](const PathView* thisPtr) -> AZStd::string { return thisPtr->RootDirectory().String(); }, nullptr)
            ->Property("anchor", [](const PathView* thisPtr) -> AZStd::string { return thisPtr->RootPath().String(); }, nullptr)
            ->Property("parents", [](const PathView* thisPtr) -> AZStd::vector<PathView>
            {
                AZStd::vector<PathView> parentPaths;
                AZ::IO::PathView currentPath(*thisPtr);
                for(AZ::IO::PathView parentPath = currentPath.ParentPath(); parentPath != currentPath; parentPath = currentPath.ParentPath())
                {
                    parentPaths.emplace_back(parentPath);
                    currentPath = parentPath;
                }
                return parentPaths;
            }, nullptr)
            ->Property("parent", [](const PathView* thisPtr) -> PathView { return thisPtr->ParentPath(); }, nullptr)
            ->Property("name", [](const PathView* thisPtr) -> AZStd::string { return thisPtr->Filename().String(); }, nullptr)
            ->Property("suffixes", [](const PathView* thisPtr) -> AZStd::vector<AZStd::string>
            {
                AZStd::vector<AZStd::string> suffixes;
                auto GetSuffixes = [&suffixes](AZStd::string_view token)
                {
                    // suffixes are always output with leading <dot> '.'
                    suffixes.emplace_back(AZStd::string::format(".%.*s", AZ_STRING_ARG(token)));
                };
                // Skip past the shortest stem.
                // Ex. filename.material.materialtype -> material.materialtype
                AZ::IO::PathView postShortestStem = thisPtr->Filename();
                AZ::StringFunc::TokenizeNext(postShortestStem.Native(), '.');
                // Only suffixes of the filename exist at this point so split on the "." character
                AZ::StringFunc::TokenizeVisitor(postShortestStem.Native(), GetSuffixes, '.');
                return suffixes;
            }, nullptr)
            ->Property("suffix", [](const PathView* thisPtr) -> AZStd::string { return thisPtr->Extension().String(); }, nullptr)
            ->Property("stem", [](const PathView* thisPtr) -> AZStd::string { return thisPtr->Stem().String(); }, nullptr)
            ->Method("as_posix", &PathView::StringAsPosix)
            ->Method("as_uri", &PathView::StringAsUri)
            ->Method("match", &PathView::Match)
            ->Method("is_absolute", &PathView::IsAbsolute)
            ->Method("is_relative_to", &PathView::IsRelativeTo)
            ->Method("relative_to", &PathView::IsAbsolute)
            ->Method("with_name", [](const PathView* thisPtr, AZStd::string_view name) -> FixedMaxPath
            {
                return thisPtr->LexicallyNormal().ReplaceFilename(name);
            })
            ->Method("with_stem", [](const PathView* thisPtr, AZStd::string_view stem) -> PathView
            {
                auto stemReplacedFilename = FixedMaxPathString(stem);
                stemReplacedFilename += thisPtr->Extension().Native();
                return thisPtr->LexicallyNormal().ReplaceFilename(AZ::IO::PathView(stemReplacedFilename));
            })
            ->Method("with_suffix", [](const PathView* thisPtr, AZStd::string_view suffix) -> PathView
            {
                return thisPtr->LexicallyNormal().ReplaceExtension(suffix);
            })
        ;
    }

    template <class PathType>
    void PathBehaviorReflect(AZ::BehaviorContext* behaviorContext)
    {
        const char* className = "";
        if constexpr (AZStd::is_same_v<PathType, Path>)
        {
            className = "PurePath";
        }
        else if constexpr (AZStd::is_same_v<PathType, FixedMaxPath>)
        {
            className = "PureFixedMaxPath";
        }

        // Replicates the python 3 pathlib API: https://docs.python.org/3/library/pathlib.html
        behaviorContext->Class<PathType>(className)
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, "pathlib")
            ->Attribute(AZ::Script::Attributes::Category, "FilesystemPath")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
            // PathView constructor
            ->template Constructor<AZ::IO::PathView>()
            // <string type> constructor
            ->template Constructor<const typename PathType::string_type&>()
            // <string type> constructor with preferred path separator
            ->template Constructor<const typename PathType::string_type&, const char>()
            // string_view constructor
            ->template Constructor<AZStd::string_view>()
            // string_view constructor with preferred path separator
            ->template Constructor<AZStd::string_view, const char>()
            // const char* constructor
            ->template Constructor<const typename PathType::value_type*>()
            // const char* constructor with preferred path separator
            ->template Constructor<const typename PathType::value_type*, const char>()
            // preferred path separator only constructor
            ->template Constructor<const char>()
            ->template WrappingMember<const char*>(&PathType::c_str)
            ->Method("c_str", &PathType::c_str)
            // Conversion function to allow interoperability with PathView class
            ->Method("ToPurePathView", &PathType::operator PathView)
            ->Method("__truediv__", [](const PathType* thisPtr, const AZ::IO::PathView& src) -> PathType { return *thisPtr / src; })
            ->Method("__div", [](const PathType* thisPtr, const AZ::IO::PathView& src) -> PathType { return *thisPtr / src; }) // Lua operator /
            ->Method("__hash__", [](const PathType* thisPtr) -> AZ::u64 { return static_cast<AZ::u64>(AZ::IO::hash_value(*thisPtr)); })
            ->Method("__str__", &PathType::String)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
            ->Method("__eq__", [](const PathType& left, const PathView& right) -> bool { return left == right; })
            ->Method("__eq", [](const PathType& left, const PathView& right) -> bool { return left == right; }) // Lua operator==
            ->Method("__lt__", [](const PathType& left, const PathView& right) -> bool { return left < right; })
            ->Method("__lt", [](const PathType& left, const PathView& right) -> bool { return left < right; }) // Lua operator <
            ->Method("__ne__", [](const PathType& left, const PathView& right) -> bool { return left != right; })
            ->Method("__gt__", [](const PathType& left, const PathView& right) -> bool { return left > right; })
            ->Method("__le__", [](const PathType& left, const PathView& right) -> bool { return left <= right; })
            ->Method("__ge__", [](const PathType& left, const PathView& right) -> bool { return left >= right; })
            // No setters for the path class properties
            ->Property("parts", [](const PathType* thisPtr) -> AZStd::vector<PathType>
            {
                // Initialize the parts with the RootPath value which includes any drive letters and root directory
                AZStd::vector<PathType> parts{ thisPtr->RootPath() };
                // Append each relative path part using the Path iterator code
                PathView postRootNamePath = thisPtr->RelativePath();
                parts.insert(parts.end(), postRootNamePath.begin(), postRootNamePath.end());
                return parts;
            }, nullptr)
            ->Property("drive", [](const PathType* thisPtr) -> AZStd::string { return thisPtr->RootName().String(); }, nullptr)
            ->Property("root", [](const PathType* thisPtr) -> AZStd::string { return thisPtr->RootDirectory().String(); }, nullptr)
            ->Property("anchor", [](const PathType* thisPtr) -> AZStd::string { return thisPtr->RootPath().String(); }, nullptr)
            ->Property("parents", [](const PathType* thisPtr) -> AZStd::vector<PathType>
            {
                AZStd::vector<PathType> parentPaths;
                AZ::IO::PathView currentPath(*thisPtr);
                for(AZ::IO::PathView parentPath = currentPath.ParentPath(); parentPath != currentPath; parentPath = currentPath.ParentPath())
                {
                    parentPaths.emplace_back(parentPath);
                    currentPath = parentPath;
                }
                return parentPaths;
            }, nullptr)
            ->Property("parent", [](const PathType* thisPtr) -> PathType { return thisPtr->ParentPath(); }, nullptr)
            ->Property("name", [](const PathType* thisPtr) -> AZStd::string { return thisPtr->Filename().String(); }, nullptr)
            ->Property("suffixes", [](const PathType* thisPtr) -> AZStd::vector<AZStd::string>
            {
                AZStd::vector<AZStd::string> suffixes;
                auto GetSuffixes = [&suffixes](AZStd::string_view token)
                {
                    // suffixes are always output with leading <dot> '.'
                    suffixes.emplace_back(AZStd::string::format(".%.*s", AZ_STRING_ARG(token)));
                };
                // Skip past the shortest stem.
                // Ex. filename.material.materialtype -> material.materialtype
                AZ::IO::PathView postShortestStem = thisPtr->Filename();
                AZ::StringFunc::TokenizeNext(postShortestStem.Native(), '.');
                // Only suffixes of the filename exist at this point so split on the "." character
                AZ::StringFunc::TokenizeVisitor(postShortestStem.Native(), GetSuffixes, '.');
                return suffixes;
            }, nullptr)
            ->Property("suffix", [](const PathType* thisPtr) -> AZStd::string { return thisPtr->Extension().String(); }, nullptr)
            ->Property("stem", [](const PathType* thisPtr) -> AZStd::string { return thisPtr->Stem().String(); }, nullptr)
            ->Method("as_posix", &PathType::StringAsPosix)
            ->Method("as_uri", &PathType::StringAsUri)
            ->Method("match", &PathType::Match)
            ->Method("is_absolute", &PathType::IsAbsolute)
            ->Method("is_relative_to", &PathType::IsRelativeTo)
            ->Method("relative_to", &PathType::IsAbsolute)
            ->Method("with_name", [](const PathType* thisPtr, AZStd::string_view name) -> PathType
            {
                return PathType(*thisPtr).ReplaceFilename(name);
            })
            ->Method("with_stem", [](const PathType* thisPtr, AZStd::string_view stem) -> PathType
            {
                auto stemReplacedFilename = typename PathType::string_type(stem);
                stemReplacedFilename += thisPtr->Extension().Native();
                return PathType(*thisPtr).ReplaceFilename(AZ::IO::PathView(stemReplacedFilename));
            })
            ->Method("with_suffix", [](const PathType* thisPtr, AZStd::string_view suffix) -> PathType
            {
                return PathType(*thisPtr).ReplaceExtension(suffix);
            })
        ;
    }

    void PathReflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<Path>()
                ->Serializer(AZ::Serialize::IDataSerializerPtr{ new PathSerializer<Path>{},
                AZ::SerializeContext::IDataSerializer::CreateDefaultDeleteDeleter() })
                ;

            serializeContext->Class<FixedMaxPath>()
                ->Serializer(AZ::Serialize::IDataSerializerPtr{ new PathSerializer<FixedMaxPath>{},
                AZ::SerializeContext::IDataSerializer::CreateDefaultDeleteDeleter() })
                ;
        }
        else if (auto jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonPathSerializer>()
                ->HandlesType<Path>()
                ->HandlesType<FixedMaxPath>();
        }
        else if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context); behaviorContext != nullptr)
        {
            PathViewBehaviorReflect(behaviorContext);
            PathBehaviorReflect<Path>(behaviorContext);
            PathBehaviorReflect<FixedMaxPath>(behaviorContext);
        }
    }
}
