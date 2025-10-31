/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <stdarg.h> 

namespace ScriptCanvas
{
    class Graph;
    class Node;
    class Slot;

    namespace Grammar
    {
        struct Source;
    }

    namespace Translation
    {
        class Writer;

        struct Configuration;

        AZStd::string EntityIdValueToString(const AZ::EntityId& entityId, const Configuration& config);

        AZStd::string_view GetCopyright();

        AZStd::string_view GetAutoNativeNamespace();

        AZStd::string_view GetDoNotModifyCommentText();

        AZ::Outcome<void, AZStd::string> SaveDotCPP(const Grammar::Source& source, AZStd::string_view dotCPP);

        AZ::Outcome<void, AZStd::string> SaveDotH(const Grammar::Source& source, AZStd::string_view dotH);

        AZ::Outcome<void, AZStd::string> SaveDotLua(const Grammar::Source& source, AZStd::string_view dotLua);

        class Writer
        {
            friend class ScopedIndent;

        public:
            Writer();

            void Indent(size_t tabs = 1);

            const AZStd::string& GetOutput() const;

            size_t GetIndent() const;

            AZStd::string&& MoveOutput();

            void Outdent(size_t tabs = 1);

            void SetIndent(size_t tabs);

            // in general, don't include newlines, as it will violate the tab policy
            void Write(const AZStd::string_view& stringView);

            // in general, don't include newlines, as it will violate the tab policy
            void Write(const char* format, ...);

            void WriteIndent();

            // in general, don't include newlines, as it will violate the tab policy
            inline void WriteIndented(const AZStd::string_view& stringView)
            {
                WriteIndent();
                Write(stringView);
            }

            // in general, don't include newlines, as it will violate the tab policy
            inline void WriteIndented(const char* format, ...)
            {
                va_list vargs;
                va_start(vargs, format);
                WriteIndent();
                m_output.append(AZStd::string::format_arg(format, vargs));
                va_end(vargs);
            }

            // in general, don't include newlines, as it will violate the tab policy
            void WriteLine(const AZStd::string_view& stringView);

            // in general, don't include newlines, as it will violate the tab policy
            void WriteLine(const char* format, ...);

            // in general, don't include newlines, as it will violate the tab policy
            inline void WriteLineIndented(const AZStd::string_view& stringView)
            {
                WriteIndent();
                WriteLine(stringView);
            }

            // in general, don't include newlines, as it will violate the tab policy
            inline void WriteLineIndented(const char* format, ...)
            {
                va_list vargs;
                va_start(vargs, format);
                WriteIndent();
                m_output.append(AZStd::string::format_arg(format, vargs));
                WriteNewLine();
                va_end(vargs);
            }

            // in general, don't include newlines, as it will violate the tab policy
            void WriteNewLine();

            void WriteSpace();

        private:
            static const size_t k_initialReservationSize = 2048;

            AZStd::string m_output;
            size_t m_indent = 0;
        };
         
        class ScopedIndent
        {
        public:
            ScopedIndent(Writer& writer)
                : m_writer(writer)
            {
                m_writer.Indent();
            }

            ~ScopedIndent()
            {
                m_writer.Outdent();
            }
        
        private:
            Writer& m_writer;
        };

    } 

} 
