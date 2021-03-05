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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace ScriptCanvas
{
    class Graph;

    namespace Grammar
    {
        struct Source;
    }

    namespace Translation
    {
        class Writer;

        const char* GetAmazonCopyright();
        
        const char* GetDoNotModifyCommentText();

        const char* GetAutoNativeNamespace();

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
            void WriteLine(const AZStd::string_view& stringView);

            // in general, don't include newlines, as it will violate the tab policy
            void WriteLine(const char* format, ...);

            void WriteNewLine();

            void WriteSpace();

        private:
            static const size_t k_initialReservationSize = 2048;

            AZStd::string m_output;
            size_t m_indent = 0;
        }; // class Writer
         
    } // namespace Translation

} // namespace ScriptCanvas