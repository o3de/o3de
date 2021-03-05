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

#include <AzCore/std/containers/stack.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class StackedString
    {
    public:
        enum class Format
        {
            ContextPath,    //!< String is formatted to mirror the SerializeContext such as "class.array.0.element".
            JsonPointer     //!< String is formatted as a JSON Pointer such as "/class/array/0/element".
        };

        explicit StackedString(Format format);

        //! Push a new string part onto the stack.
        void Push(AZStd::string_view value);
        //! Push an integer value to the stack. The value will be converted to a string.
        void Push(size_t value);
        void Pop();

        //! Resets the StackedString to it's default and frees all memory.
        void Reset();

        AZStd::string_view Get() const;
        operator AZStd::string_view() const;

    private:
        AZStd::stack<size_t> m_offsetStack;
        OSString m_string;
        Format m_format;
    };

    //! Scoped object that manages the life cycle of a single entry on the string stack.
    class ScopedStackedString
    {
    public:
        //! Pushes a new entry on the stack.
        //! @param string The string stack to push to.
        //! @param value The new entry to push.
        ScopedStackedString(StackedString& string, AZStd::string_view value);
        //! Pushes a new entry on the stack. This overload can be used to push an index to for instance an array.
        //! @param string The string stack to push to.
        //! @param value The new entry to push. The integer will be converted to string.
        ScopedStackedString(StackedString& string, size_t value);
        ~ScopedStackedString();

    private:
        StackedString& m_string;
    };
} // namespace AZ
