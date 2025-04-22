/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Console/ConsoleTypeHelpers.h>
#include <AzCore/DOM/DomPath.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ::Dom
{
    PathEntry::PathEntry(size_t value)
        : m_value(value)
    {
    }

    PathEntry::PathEntry(AZ::Name value)
        : m_value(AZStd::move(value))
    {
    }

    PathEntry::PathEntry(AZStd::string_view value)
        : m_value(AZ::Name(value))
    {
    }

    PathEntry& PathEntry::operator=(size_t value)
    {
        m_value = value;
        return *this;
    }

    PathEntry& PathEntry::operator=(AZ::Name value)
    {
        m_value = AZStd::move(value);
        return *this;
    }

    PathEntry& PathEntry::operator=(AZStd::string_view value)
    {
        m_value = AZ::Name(value);
        return *this;
    }

    bool PathEntry::operator==(const PathEntry& other) const
    {
        return m_value == other.m_value;
    }

    bool PathEntry::operator==(size_t value) const
    {
        const size_t* internalValue = AZStd::get_if<size_t>(&m_value);
        return internalValue != nullptr && *internalValue == value;
    }

    bool PathEntry::operator==(const AZ::Name& key) const
    {
        const AZ::Name* internalValue = AZStd::get_if<AZ::Name>(&m_value);
        return internalValue != nullptr && *internalValue == key;
    }

    bool PathEntry::operator==(AZStd::string_view key) const
    {
        const AZ::Name* internalValue = AZStd::get_if<AZ::Name>(&m_value);
        return internalValue != nullptr && *internalValue == AZ::Name(key);
    }

    bool PathEntry::operator!=(const PathEntry& other) const
    {
        return m_value != other.m_value;
    }

    bool PathEntry::operator!=(size_t value) const
    {
        return !operator==(value);
    }

    bool PathEntry::operator!=(const AZ::Name& key) const
    {
        return !operator==(key);
    }

    bool PathEntry::operator!=(AZStd::string_view key) const
    {
        return !operator==(key);
    }

    void PathEntry::SetEndOfArray()
    {
        m_value = EndOfArrayIndex;
    }

    bool PathEntry::IsEndOfArray() const
    {
        const size_t* result = AZStd::get_if<size_t>(&m_value);
        return result == nullptr ? false : ((*result) == EndOfArrayIndex);
    }

    bool PathEntry::IsIndex() const
    {
        const size_t* result = AZStd::get_if<size_t>(&m_value);
        return result == nullptr ? false : ((*result) != EndOfArrayIndex);
    }

    bool PathEntry::IsKey() const
    {
        return AZStd::holds_alternative<AZ::Name>(m_value);
    }

    size_t PathEntry::GetIndex() const
    {
        AZ_Assert(!IsEndOfArray() && IsIndex(), "GetIndex called on PathEntry that is not an index");
        return AZStd::get<size_t>(m_value);
    }

    size_t PathEntry::GetHash() const
    {
        return AZStd::visit(
            [&](auto&& value) -> size_t
            {
                using CurrentType = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<CurrentType, size_t>)
                {
                    AZStd::hash<size_t> hasher;
                    return hasher(value);
                }
                else if constexpr (AZStd::is_same_v<CurrentType, AZ::Name>)
                {
                    return value.GetHash();
                }
            },
            m_value);
    }
} // namespace AZ::Dom

namespace AZStd
{
    size_t AZStd::hash<AZ::Dom::PathEntry>::operator()(const AZ::Dom::PathEntry& entry) const
    {
        return entry.GetHash();
    }
} // namespace AZStd

namespace AZ::Dom
{
    const AZ::Name& PathEntry::GetKey() const
    {
        AZ_Assert(IsKey(), "Key called on PathEntry that is not a key");
        return AZStd::get<AZ::Name>(m_value);
    }

    Path::Path(AZStd::initializer_list<PathEntry> init)
        : m_entries(init)
    {
    }

    Path::Path(AZStd::string_view pathString)
    {
        FromString(pathString);
    }

    Path Path::operator/(const PathEntry& entry) const
    {
        Path newPath(*this);
        newPath /= entry;
        return newPath;
    }

    Path Path::operator/(size_t index) const
    {
        return *this / PathEntry(index);
    }

    Path Path::operator/(AZ::Name key) const
    {
        return *this / PathEntry(key);
    }

    Path Path::operator/(AZStd::string_view key) const
    {
        return *this / PathEntry(key);
    }

    Path Path::operator/(const Path& other) const
    {
        Path newPath(*this);
        newPath /= other;
        return newPath;
    }

    Path& Path::operator/=(const PathEntry& entry)
    {
        Push(entry);
        return *this;
    }

    Path& Path::operator/=(size_t index)
    {
        return *this /= PathEntry(index);
    }

    Path& Path::operator/=(AZ::Name key)
    {
        return *this /= PathEntry(key);
    }

    Path& Path::operator/=(AZStd::string_view key)
    {
        return *this /= PathEntry(key);
    }

    Path& Path::operator/=(const Path& other)
    {
        for (const PathEntry& entry : other)
        {
            Push(entry);
        }
        return *this;
    }

    bool Path::operator==(const Path& other) const
    {
        return m_entries == other.m_entries;
    }

    const Path::ContainerType& Path::GetEntries() const
    {
        return m_entries;
    }

    void Path::Push(PathEntry entry)
    {
        m_entries.push_back(AZStd::move(entry));
    }

    void Path::Push(size_t entry)
    {
        Push(PathEntry(entry));
    }

    void Path::Push(AZ::Name entry)
    {
        Push(PathEntry(AZStd::move(entry)));
    }

    void Path::Push(AZStd::string_view entry)
    {
        Push(AZ::Name(entry));
    }

    void Path::Pop()
    {
        m_entries.pop_back();
    }

    void Path::Clear()
    {
        m_entries.clear();
    }

    PathEntry Path::At(size_t index) const
    {
        if (index < m_entries.size())
        {
            return m_entries[index];
        }
        return {};
    }

    PathEntry Path::Back() const
    {
        return m_entries.back();
    }

    size_t Path::Size() const
    {
        return m_entries.size();
    }

    bool Path::IsEmpty() const
    {
        return m_entries.empty();
    }

    PathEntry& Path::operator[](size_t index)
    {
        return m_entries[index];
    }

    const PathEntry& Path::operator[](size_t index) const
    {
        return m_entries[index];
    }

    Path::ContainerType::iterator Path::begin()
    {
        return m_entries.begin();
    }

    Path::ContainerType::iterator Path::end()
    {
        return m_entries.end();
    }

    Path::ContainerType::const_iterator Path::begin() const
    {
        return m_entries.cbegin();
    }

    Path::ContainerType::const_iterator Path::end() const
    {
        return m_entries.cend();
    }

    Path::ContainerType::const_iterator Path::cbegin() const
    {
        return m_entries.cbegin();
    }

    Path::ContainerType::const_iterator Path::cend() const
    {
        return m_entries.cend();
    }

    size_t Path::size() const
    {
        return m_entries.size();
    }

    size_t Path::GetStringLength() const
    {
        size_t size = 0;
        for (const PathEntry& entry : m_entries)
        {
            ++size;
            if (entry.IsEndOfArray())
            {
                size += 1;
            }
            else if (entry.IsIndex())
            {
                const size_t index = entry.GetIndex();
                const double digitCount = index > 0 ? log10(aznumeric_cast<double>(index + 1)) : 1.0;
                size += aznumeric_cast<size_t>(ceil(digitCount));
            }
            else
            {
                const char* nameBuffer = entry.GetKey().GetCStr();
                for (size_t i = 0; nameBuffer[i]; ++i)
                {
                    if (nameBuffer[i] == EscapeCharacter || nameBuffer[i] == PathSeparator)
                    {
                        ++size;
                    }
                    ++size;
                }
            }
        }
        return size;
    }

    size_t Path::FormatString(char* stringBuffer, size_t bufferSize) const
    {
        size_t bufferIndex = 0;

        auto putChar = [&](char c)
        {
            if (bufferIndex >= bufferSize)
            {
                return;
            }
            stringBuffer[bufferIndex++] = c;
        };

        auto writeToBuffer = [&](const char* key)
        {
            for (size_t keyIndex = 0; key[keyIndex]; ++keyIndex)
            {
                const char c = key[keyIndex];
                if (c == EscapeCharacter)
                {
                    putChar(EscapeCharacter);
                    putChar(TildeSequence);
                }
                else if (c == PathSeparator)
                {
                    putChar(EscapeCharacter);
                    putChar(ForwardSlashSequence);
                }
                else
                {
                    putChar(c);
                }
            }
        };

        for (const PathEntry& entry : m_entries)
        {
            if (bufferIndex >= bufferSize)
            {
                return bufferIndex;
            }

            putChar(PathSeparator);
            if (entry.IsEndOfArray())
            {
                putChar(EndOfArrayCharacter);
            }
            else if (entry.IsIndex())
            {
                bufferIndex += azsnprintf(&stringBuffer[bufferIndex], bufferSize - bufferIndex, "%zu", entry.GetIndex());
            }
            else
            {
                writeToBuffer(entry.GetKey().GetCStr());
            }
        }

        size_t bytesWritten = bufferIndex;
        putChar('\0');

        return bytesWritten;
    }

    AZStd::string Path::ToString() const
    {
        AZStd::string formattedString;
        const size_t size = GetStringLength();
        formattedString.resize_no_construct(size);
        FormatString(formattedString.data(), size + 1);
        return formattedString;
    }

    void Path::AppendToString(AZStd::string& output) const
    {
        const size_t startIndex = output.length();
        const size_t stringLength = GetStringLength();
        output.resize_no_construct(startIndex + stringLength);
        FormatString(output.data() + startIndex, stringLength + 1);
    }

    bool Path::ContainsNormalizedEntries() const
    {
        for (const PathEntry& entry : m_entries)
        {
            if (entry.IsEndOfArray())
            {
                return true;
            }
        }
        return false;
    }

    void Path::FromString(AZStd::string_view pathString)
    {
        m_entries.clear();
        if (pathString.empty())
        {
            return;
        }

        size_t pathEntryCount = 0;
        for (size_t i = 1; i < pathString.size(); ++i)
        {
            if (pathString[i] == PathSeparator)
            {
                ++pathEntryCount;
            }
        }
        m_entries.reserve(pathEntryCount);

        // Ignore a preceeding path separator and start processing after it
        size_t pathIndex = pathString[0] == PathSeparator ? 1 : 0;
        bool isNumber = true;
        AZStd::string convertedSection;
        for (size_t i = pathIndex; i <= pathString.size(); ++i)
        {
            if (i == pathString.size() || pathString[i] == PathSeparator)
            {
                AZStd::string_view section = pathString.substr(pathIndex, i - pathIndex);
                if (section.size() == 1 && section[0] == EndOfArrayCharacter)
                {
                    PathEntry entry;
                    entry.SetEndOfArray();
                    m_entries.push_back(AZStd::move(entry));
                }
                else if (isNumber && !section.empty())
                {
                    size_t index = 0;
                    ConsoleTypeHelpers::StringToValue(index, section);
                    m_entries.push_back(PathEntry{ index });
                }
                else
                {
                    convertedSection.clear();
                    size_t lastPos = 0;
                    size_t posToEscape = section.find(EscapeCharacter);
                    while (posToEscape != AZStd::string_view::npos)
                    {
                        if (convertedSection.empty())
                        {
                            convertedSection.reserve(section.size() - 1);
                        }
                        convertedSection += section.substr(lastPos, posToEscape - lastPos);
                        if (section[posToEscape + 1] == ForwardSlashSequence)
                        {
                            convertedSection += '/';
                        }
                        else
                        {
                            convertedSection += '~';
                        }

                        lastPos = posToEscape + 2;
                        posToEscape = section.find(EscapeCharacter, posToEscape + 2);
                    }

                    if (!convertedSection.empty())
                    {
                        convertedSection += section.substr(lastPos);
                        m_entries.emplace_back(convertedSection);
                    }
                    else
                    {
                        m_entries.emplace_back(section);
                    }
                }
                pathIndex = i + 1;
                isNumber = true;
                continue;
            }

            const char c = pathString[i];
            isNumber = isNumber && c >= '0' && c <= '9';
        }
    }
} // namespace AZ::Dom
