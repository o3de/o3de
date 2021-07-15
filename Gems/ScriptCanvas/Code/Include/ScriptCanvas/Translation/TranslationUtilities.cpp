/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationUtilities.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

#include "Configuration.h"

namespace TranslationUtilitiesCPP
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Translation;
    
    const char* k_namespaceNameNative = "AutoNative";
    const char* k_fileDirectoryPathLua = "@usercache@/DebugScriptCanvas2LuaOutput/";
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

    AZStd::string GetDebugLuaFilePath(const Grammar::Source& source, AZStd::string_view extension)
    {
        return AZStd::string::format("%s%s_VM.%s", TranslationUtilitiesCPP::k_fileDirectoryPathLua, source.m_name.data(), extension.data());
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

        ~FileEventHandler()
        {
            BusDisconnect();
        }

        void OnError(const AZ::IO::SystemFile* /*file*/, const char* fileName, int errorCode) override
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

        // \todo get a (debug) file path based on the extension
        const AZStd::string filePath = TranslationUtilitiesCPP::GetDebugLuaFilePath(source, extension);

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
    }

}

namespace ScriptCanvas
{
    namespace Translation
    {
        AZStd::string EntityIdValueToString(const AZ::EntityId& entityId, const Configuration& config)
        {
            if (entityId == GraphOwnerId)
            {
                return config.m_executionStateEntityIdRef;
            }
            else if (entityId == UniqueId)
            {
                return config.m_executionStateScriptCanvasIdRef;
            }
            else
            {
                return EntityIdToU64String(entityId);
            }
        }

        AZStd::string EntityIdToU64String(const AZ::EntityId& entityId)
        {
            return AZStd::string::format("%llu", AZ::u64(entityId));
        }

        AZStd::string_view GetAutoNativeNamespace()
        {
            return TranslationUtilitiesCPP::k_namespaceNameNative;
        }

        AZStd::string_view GetCopyright()
        {
            return
                "* Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.\n"
                "*\n"
                "* SPDX-License-Identifier: Apache-2.0 OR MIT\n"
                "*"
                ;
        }

        AZStd::string_view GetDoNotModifyCommentText()
        {
            return 
                "DO NOT MODIFY THIS FILE, IT IS AUTO-GENERATED FROM A SCRIPT CANVAS GRAPH!"
                ;
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

    } 

} 
