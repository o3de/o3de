/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Dom
{
    //! Represents the path to a direct descendant of a Value.
    //! PathEntry may be one of the following:
    //! - Index, a numerical index for indexing within Arrays and Nodes
    //! - Key, a name for indexing within Objects and Nodes
    //! - EndOfArray, a special-case indicator for representing the end of an array
    //!   used by the patching system to represent push / pop back operations.
    class PathEntry final
    {
    public:
        static constexpr size_t EndOfArrayIndex = size_t(-1);

        PathEntry() = default;
        PathEntry(const PathEntry&) = default;
        PathEntry(PathEntry&&) = default;
        explicit PathEntry(size_t value);
        explicit PathEntry(AZ::Name value);
        explicit PathEntry(AZStd::string_view value);

        PathEntry& operator=(const PathEntry&) = default;
        PathEntry& operator=(PathEntry&&) = default;
        PathEntry& operator=(size_t value);
        PathEntry& operator=(AZ::Name value);
        PathEntry& operator=(AZStd::string_view value);

        bool operator==(const PathEntry& other) const;
        bool operator==(size_t index) const;
        bool operator==(const AZ::Name& key) const;
        bool operator==(AZStd::string_view key) const;
        bool operator!=(const PathEntry& other) const;
        bool operator!=(size_t index) const;
        bool operator!=(const AZ::Name& key) const;
        bool operator!=(AZStd::string_view key) const;

        void SetEndOfArray();

        bool IsEndOfArray() const;
        bool IsIndex() const;
        bool IsKey() const;

        size_t GetIndex() const;
        const AZ::Name& GetKey() const;
        size_t GetHash() const;

    private:
        AZStd::variant<size_t, AZ::Name> m_value;
    };
} // namespace AZ::Dom

namespace AZ::Dom
{
    //! Represents a path, represented as a series of PathEntry values, to a position in a Value.
    class Path final
    {
    public:
        AZ_TYPE_INFO(AZ::Dom::Path, "{C0081C45-F15D-4F46-9680-19535D33C312}")

        using ContainerType = AZStd::vector<PathEntry>;
        static constexpr char PathSeparator = '/';
        static constexpr char EscapeCharacter = '~';
        static constexpr char TildeSequence = '0';
        static constexpr char ForwardSlashSequence = '1';
        static constexpr char EndOfArrayCharacter = '-';

        Path() = default;
        Path(const Path&) = default;
        Path(Path&&) = default;
        explicit Path(AZStd::initializer_list<PathEntry> init);
        //! Creates a Path from a path string, a path string is formatted per the JSON pointer specification
        //! and looks like "/path/to/value/0"
        explicit Path(AZStd::string_view pathString);

        template<class InputIterator>
        explicit Path(InputIterator first, InputIterator last)
            : m_entries(first, last)
        {
        }

        Path& operator=(const Path&) = default;
        Path& operator=(Path&&) = default;

        Path operator/(const PathEntry&) const;
        Path operator/(size_t) const;
        Path operator/(AZ::Name) const;
        Path operator/(AZStd::string_view) const;
        Path operator/(const Path&) const;

        Path& operator/=(const PathEntry&);
        Path& operator/=(size_t);
        Path& operator/=(AZ::Name);
        Path& operator/=(AZStd::string_view);
        Path& operator/=(const Path&);

        bool operator==(const Path&) const;
        bool operator!=(const Path& rhs) const
        {
            return !operator==(rhs);
        }

        const ContainerType& GetEntries() const;
        void Push(PathEntry entry);
        void Push(size_t entry);
        void Push(AZ::Name entry);
        void Push(AZStd::string_view key);
        void Pop();
        void Clear();
        PathEntry At(size_t index) const;
        PathEntry Back() const;
        size_t Size() const;
        bool IsEmpty() const;

        PathEntry& operator[](size_t index);
        const PathEntry& operator[](size_t index) const;

        ContainerType::iterator begin();
        ContainerType::iterator end();
        ContainerType::const_iterator begin() const;
        ContainerType::const_iterator end() const;
        ContainerType::const_iterator cbegin() const;
        ContainerType::const_iterator cend() const;
        size_t size() const;

        //! Gets the length this path would require, if string-formatted.
        //! The length includes the contents of the string but not a null terminator.
        size_t GetStringLength() const;
        //! Formats a JSON-pointer style path string into the target buffer.
        //! This operation will fail if bufferSize < GetStringLength() + 1
        //! \return The number of bytes written, excepting the null terminator.
        size_t FormatString(char* stringBuffer, size_t bufferSize) const;
        //! Returns a JSON-pointer style path string for this path.
        AZStd::string ToString() const;

        void AppendToString(AZStd::string& output) const;
        template<class T>
        void AppendToString(T& output) const
        {
            const size_t startIndex = output.length();
            output.resize_no_construct(startIndex + FormatString(output.data() + startIndex, output.capacity() - startIndex));
        }

        //! Reads a JSON-pointer style path from pathString and replaces this path's contents.
        //! Paths are accepted in the following forms:
        //! "/path/to/foo/0"
        //! "path/to/foo/0"
        void FromString(AZStd::string_view pathString);

        //! Returns true if this path contains any "EndOfArray" entries that require a target DOM to look up.
        bool ContainsNormalizedEntries() const;

    private:
        ContainerType m_entries;
    };
} // namespace AZ::Dom

namespace AZStd
{
    template<>
    struct hash<AZ::Dom::PathEntry>
    {
        size_t operator()(const AZ::Dom::PathEntry& entry) const;
    };

    template<>
    struct hash<AZ::Dom::Path>
    {
        size_t operator()(const AZ::Dom::Path& path) const
        {
            return AZStd::hash_range(path.begin(), path.end());
        }
    };
} // namespace AZStd
