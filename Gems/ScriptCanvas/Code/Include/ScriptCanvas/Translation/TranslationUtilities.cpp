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

#include "precompiled.h"
#include "TranslationUtilities.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <stdarg.h> 

namespace TranslationUtilitiesCPP
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Translation;
    
    const char* k_namespaceNameNative = "AutoNative";
    const char* k_fileDirectoryPathNative = "@engroot@/Gems/ScriptCanvas/Include/ScriptCanvas/AutoNative";
    const char* k_fileDirectoryPathLua = "@engroot@/Gems/ScriptCanvas/Scripts/AutoLua";
    const char* k_space = " ";
    
    const size_t k_maxTabs = 20;

    AZ_INLINE const char* GetTabs(size_t tabs)
    {
        AZ_Assert(tabs >= 0 && tabs <= k_maxTabs, "invalid argument to GetTabs");
        
        static const char* const k_tabs[] =
        {
            "", // 0
            "\t",
            "\t\t",
            "\t\t\t",
            "\t\t\t\t",
            "\t\t\t\t\t", // 5
            "\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t", // 10
            "\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", // 15
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t",
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", // 20
        };
        
        return k_tabs[tabs];
    }

    AZ_INLINE void WriteTabs(AZStd::string& text, size_t indent)
    {
        // in the interest of compilation max speed, min readability, tab addition should be able to be able be compiled out
        text.append(GetTabs(indent));
    }

    class FileEventHandler
        : public AZ::IO::FileIOEventBus::Handler
    {
    public:
        int m_errorCode = 0;
        AZStd::string m_fileName;

        FileEventHandler()
        {
            BusConnect();
        }

        void OnError(const AZ::IO::SystemFile* file, const char* fileName, int errorCode) override
        {
            m_errorCode = errorCode;
            
            if (fileName) 
            {
                m_fileName = fileName;
            }
        }
    };

    AZ::Outcome<void, AZStd::string> SaveFile(const Grammar::Source& source, AZStd::string_view text, AZStd::string_view extension)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        if (!fileIO)
        {
            return AZ::Failure(AZStd::string("FileIOBase unavailable"));
        }

        if (!fileIO->GetAlias("@engroot@"))
        {
             const char* engineRoot = nullptr;
             AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
 
             if (engineRoot == nullptr)
             {
                 return AZ::Failure(AZStd::string("no engine root"));
             }
 
             fileIO->SetAlias("@engroot@", engineRoot);
        }
        
        // create an auto-gen directory layout that matches the layout of where the .scriptcanvas assets are saved
        const AZStd::string filePath = AZStd::string::format("%s/%s%s.%s", TranslationUtilitiesCPP::k_fileDirectoryPathNative, source.m_path.c_str(), source.m_name.data(), extension.data());

        FileEventHandler eventHandler;

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        const AZ::IO::Result fileOpenResult = fileIO->Open(filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle);

        if (fileOpenResult != AZ::IO::ResultCode::Success)
        {
            return AZ::Failure(AZStd::string::format("Failed to open file: %s, error code: %d", filePath.c_str(), eventHandler.m_errorCode));
        }

        const AZ::IO::Result fileWriteResult = fileIO->Write(fileHandle, text.begin(), text.size());
        if (fileWriteResult != AZ::IO::ResultCode::Success)
        {
            return AZ::Failure(AZStd::string::format("Failed to write file: %s, error code: %d", filePath.c_str(), eventHandler.m_errorCode));
        }

        const AZ::IO::Result fileCloseResult = fileIO->Close(fileHandle);
        if (fileCloseResult != AZ::IO::ResultCode::Success)
        {
            return AZ::Failure(AZStd::string::format("Failed to close file: %s, error code: %d", filePath.c_str(), eventHandler.m_errorCode));
        }

        return AZ::Success();
    } // AZ::Outcome<void, AZStd::string> SaveFile(const Grammar::Source& source, AZStd::string_view text, AZStd::string_view extension)

} // namespace TranslationUtilitiesCPP

namespace ScriptCanvas
{
    namespace Translation
    {
        const char* GetAmazonCopyright()
        {
            return
                "* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or\n"
                "* its licensors.\n"
                "*\n"
                "* For complete copyright and license terms please see the LICENSE at the root of this\n"
                "* distribution (the \"License\"). All use of this software is governed by the License,\n"
                "* or, if provided, by the license below or the license accompanying this file. Do not\n"
                "* remove or modify any license notices. This file is distributed on an \"AS IS\" BASIS,\n"
                "* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
                "*"
                ;
        }

        const char* GetDoNotModifyCommentText()
        {
            return 
                "DO NOT MODIFY THIS FILE, IT IS AUTO-GENERATED FROM A SCRIPT CANVAS GRAPH!"
                ;
        }

        const char* GetAutoNativeNamespace()
        {
            return TranslationUtilitiesCPP::k_namespaceNameNative;
        }
                
        AZ::Outcome<void, AZStd::string> SaveDotCPP(const Grammar::Source& source, AZStd::string_view dotCPP)
        {
            return TranslationUtilitiesCPP::SaveFile(source, dotCPP, "cpp");
        }

        AZ::Outcome<void, AZStd::string> SaveDotH(const Grammar::Source& source, AZStd::string_view dotH)
        {
            return TranslationUtilitiesCPP::SaveFile(source, dotH, "h");
        }

        AZ::Outcome<void, AZStd::string> SaveDotLua(const Grammar::Source& source, AZStd::string_view dotLua)
        {
            return TranslationUtilitiesCPP::SaveFile(source, dotLua, "lua");
        }
      
        Writer::Writer()
            : m_indent(0)
        {}
        
        void Writer::Indent(size_t tabs)
        {
            m_indent = AZStd::clamp(m_indent + tabs, size_t(0), TranslationUtilitiesCPP::k_maxTabs);
        }
        
        const AZStd::string& Writer::GetOutput() const
        {
            return m_output;
        }

        size_t Writer::GetIndent() const
        {
            return m_indent;
        }

        AZStd::string&& Writer::MoveOutput()
        {
            return AZStd::move(m_output);
        }

        void Writer::Outdent(size_t tabs)
        {
            m_indent = AZStd::clamp(m_indent - tabs, size_t(0), TranslationUtilitiesCPP::k_maxTabs);
        }

        void Writer::SetIndent(size_t tabs)
        {
            m_indent = AZStd::clamp(tabs, size_t(0), TranslationUtilitiesCPP::k_maxTabs);
        }

        void Writer::Write(const AZStd::string_view& stringView)
        {
            m_output.append(stringView);
        }

        void Writer::Write(const char* format, ...)
        {
            va_list vargs;
            va_start(vargs, format);
            m_output.append(AZStd::string::format_arg(format, vargs));
            va_end(vargs);
        }

        void Writer::WriteIndent()
        {
            TranslationUtilitiesCPP::WriteTabs(m_output, m_indent);
        }
        
        void Writer::WriteLine(const AZStd::string_view& stringView)
        {
            m_output.append(stringView);
            WriteNewLine();
        }

        void Writer::WriteLine(const char* format, ...)
        {
            va_list vargs;
            va_start(vargs, format);
            m_output.append(AZStd::string::format_arg(format, vargs));
            WriteNewLine();
            va_end(vargs);
        }

        void Writer::WriteNewLine()
        {
            m_output.append("\n");
        }

        void Writer::WriteSpace()
        {
            m_output.append(TranslationUtilitiesCPP::k_space);
        }

    } // namespace Translation

} // namespace ScriptCanvas