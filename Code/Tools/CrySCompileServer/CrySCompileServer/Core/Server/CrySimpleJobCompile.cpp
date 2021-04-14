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
// Original file Copyright Crytek GMBH or its affiliates, used under license.


#include "CrySimpleSock.hpp"
#include "CrySimpleJobCompile.hpp"
#include "CrySimpleFileGuard.hpp"
#include "CrySimpleServer.hpp"
#include "CrySimpleCache.hpp"
#include "ShaderList.hpp"

#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <tinyxml/tinyxml.h>
#include <Core/StdTypes.hpp>
#include <Core/WindowsAPIImplementation.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <zlib.h>

#include <iostream>
#include <fstream>
#include <sstream>

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define CRYSIMPLEJOBCOMPILE_CPP_SECTION_1 1
#endif

#define MAX_COMPILER_WAIT_TIME (60 * 1000)

AZStd::atomic_long CCrySimpleJobCompile::m_GlobalCompileTasks       = {0};
AZStd::atomic_long CCrySimpleJobCompile::m_GlobalCompileTasksMax    = {0};
volatile int32_t CCrySimpleJobCompile::m_RemoteServerID                 = 0;
volatile int64_t CCrySimpleJobCompile::m_GlobalCompileTime  =   0;

struct STimer
{
    int64_t m_freq;
    STimer()
    {
        QueryPerformanceFrequency((LARGE_INTEGER*)&m_freq);
    }
    int64_t GetTime() const
    {
        int64_t t;
        QueryPerformanceCounter((LARGE_INTEGER*)&t);
        return t;
    }

    double TimeToSeconds(int64_t t)
    {
        return ((double)t) / m_freq;
    }
};


STimer g_Timer;

// This function validates executables up to version 21
// because it's received within the compilation flags.
bool ValidateExecutableStringLegacy(const AZStd::string& executableString)
{
    AZStd::string::size_type endOfCommand = executableString.find(" ");

    // Game always sends some type of options after the command. If we don't
    // have a space then that implies that there are no options. Reject the
    // command as someone being malicious
    if (endOfCommand == AZStd::string::npos)
    {
        return false;
    }

    AZStd::string commandString = executableString.substr(0, endOfCommand);

    // The game never sends a parent directory in the compiler flags so lets
    // reject any commands that have .. in it
    if (commandString.find("..") != AZStd::string::npos)
    {
        return false;
    }

    // Though the code later down would fail gracefully reject any absolute paths here
    if (commandString.find("\\\\") != AZStd::string::npos ||
        commandString.find(":") != AZStd::string::npos)
    {
        return false;
    }

    // Only allow a subset of executables to be accepted...
    if (commandString.find("fxc.exe") == AZStd::string::npos &&
        commandString.find("FXC.exe") == AZStd::string::npos &&
        commandString.find("HLSLcc.exe") == AZStd::string::npos &&
        commandString.find("HLSLcc_dedicated.exe") == AZStd::string::npos &&
        commandString.find("DXProvoShaderCompiler.exe") == AZStd::string::npos &&
        commandString.find("dxcGL") == AZStd::string::npos &&
        commandString.find("dxcMetal") == AZStd::string::npos)
    {
        return false;
    }

    return true;
}

CCrySimpleJobCompile::CCrySimpleJobCompile(uint32_t requestIP, EProtocolVersion Version, std::vector<uint8_t>* pRVec)
    : CCrySimpleJobCache(requestIP)
    , m_Version(Version)
    , m_pRVec(pRVec)
{
    ++m_GlobalCompileTasks;
    if (m_GlobalCompileTasksMax < m_GlobalCompileTasks)
    {
        //Need this cast as the copy assignment operator is implicitly deleted
        m_GlobalCompileTasksMax = static_cast<long>(m_GlobalCompileTasks);
    }
}

CCrySimpleJobCompile::~CCrySimpleJobCompile()
{
    --m_GlobalCompileTasks;
}

bool CCrySimpleJobCompile::Execute(const TiXmlElement* pElement)
{
    std::vector<uint8_t>& rVec = *m_pRVec;

    size_t Size = SizeOf(rVec);

    CheckHashID(rVec, Size);

    if (State() == ECSJS_CACHEHIT)
    {
        State(ECSJS_DONE);
        return true;
    }

    if (!SEnviropment::Instance().m_FallbackServer.empty() && m_GlobalCompileTasks > SEnviropment::Instance().m_FallbackTreshold)
    {
        tdEntryVec ServerVec;
        CSTLHelper::Tokenize(ServerVec, SEnviropment::Instance().m_FallbackServer, ";");
        uint32_t Idx = m_RemoteServerID++;
        uint32_t Count = (uint32_t)ServerVec.size();
        std::string Server  =   ServerVec[Idx % Count];
        printf("  Remote Compile on %s ...\n", Server.c_str());
        CCrySimpleSock Sock(Server, SEnviropment::Instance().m_port);
        if (Sock.Valid())
        {
            Sock.Forward(rVec);
            std::vector<uint8_t> Tmp;
            if (Sock.Backward(Tmp))
            {
                rVec    =   Tmp;
                if (Tmp.size() <= 4 || (m_Version == EPV_V002 && Tmp[4] != ECSJS_DONE))
                {
                    State(ECSJS_ERROR_COMPILE);
                    CrySimple_ERROR("failed to compile request");
                    return false;
                }
                State(ECSJS_DONE);
                //printf("done\n");
            }
            else
            {
                printf("failed, fallback to local\n");
            }
        }
        else
        {
            printf("failed, fallback to local\n");
        }
    }
    if (State() == ECSJS_NONE)
    {
        if (!Compile(pElement, rVec) || rVec.size() == 0)
        {
            State(ECSJS_ERROR_COMPILE);
            CrySimple_ERROR("failed to compile request");
            return false;
        }

        tdDataVector rDataRaw;
        rDataRaw.swap(rVec);
        if (!CSTLHelper::Compress(rDataRaw, rVec))
        {
            State(ECSJS_ERROR_COMPRESS);
            CrySimple_ERROR("failed to compress request");
            return false;
        }
        State(ECSJS_DONE);
    }

    // Cache compiled data
    const char* pCaching    =   pElement->Attribute("Caching");
    if (State() != ECSJS_ERROR && (!pCaching || std::string(pCaching) == "1"))
    {
        CCrySimpleCache::Instance().Add(HashID(), rVec);
    }

    return true;
}

bool CCrySimpleJobCompile::Compile(const TiXmlElement* pElement, std::vector<uint8_t>& rVec)
{
    AZStd::string platform;
    AZStd::string compiler;
    AZStd::string language;
    AZStd::string shaderPath;
    
    if (m_Version >= EPV_V0023)
    {
        // NOTE: These attributes were alredy validated.
        platform = pElement->Attribute("Platform");
        compiler = pElement->Attribute("Compiler");
        language = pElement->Attribute("Language");
        
        shaderPath = AZStd::string::format("%s%s-%s-%s/", SEnviropment::Instance().m_ShaderPath.c_str(), platform.c_str(), compiler.c_str(), language.c_str());
    }
    else
    {
        // In previous versions Platform attribute is the language
        platform = "N/A";
        language = pElement->Attribute("Platform");
        
        // Map shader language to shader compiler key
        const AZStd::unordered_map<AZStd::string, AZStd::string> languageToCompilerMap
        {
            {
                "GL4", SEnviropment::m_GLSL_HLSLcc
            },{
                "GLES3_0", SEnviropment::m_GLSL_HLSLcc
            },{
                "GLES3_1", SEnviropment::m_GLSL_HLSLcc
            },{
                "DX11", SEnviropment::m_D3D11_FXC
            },{
                "METAL", SEnviropment::m_METAL_HLSLcc
            },{
                "ORBIS", SEnviropment::m_Orbis_DXC
            },{
                "JASPER", SEnviropment::m_Jasper_FXC
            }
        };
        
        auto foundShaderLanguage = languageToCompilerMap.find(language);
        if (foundShaderLanguage == languageToCompilerMap.end())
        {
            State(ECSJS_ERROR_INVALID_LANGUAGE);
            CrySimple_ERROR("Trying to compile with invalid shader language");
            return false;
        }
        
        if (m_Version < EPV_V0022)
        {
            compiler = "N/A"; // Compiler exe will be specified inside 'compile flags', this variable won't be used
        }
        else
        {
            compiler = foundShaderLanguage->second;
            
            if (!SEnviropment::Instance().IsShaderCompilerValid(compiler))
            {
                State(ECSJS_ERROR_INVALID_COMPILER);
                CrySimple_ERROR("Trying to compile with invalid shader compiler");
                return false;
            }
        }
        
        shaderPath = AZStd::string::format("%s%s/", SEnviropment::Instance().m_ShaderPath.c_str(), language.c_str());
    }
    
    shaderPath = AZ::IO::PathView(shaderPath).LexicallyNormal().Native();
    if (!IsPathValid(shaderPath))
    {
        State(ECSJS_ERROR);
        CrySimple_ERROR("Shaders output path is invalid");
        return false;
    }

    // Create shaders directory
    AZ::IO::SystemFile::CreateDir( shaderPath.c_str() );

    const char* pProfile           = pElement->Attribute("Profile");
    const char* pProgram           = pElement->Attribute("Program");
    const char* pEntry             = pElement->Attribute("Entry");
    const char* pCompileFlags      = pElement->Attribute("CompileFlags");
    const char* pShaderRequestLine = pElement->Attribute("ShaderRequest");

    if (!pProfile)
    {
        State(ECSJS_ERROR_INVALID_PROFILE);
        CrySimple_ERROR("failed to extract Profile of the request");
        return false;
    }
    if (!pProgram)
    {
        State(ECSJS_ERROR_INVALID_PROGRAM);
        CrySimple_ERROR("failed to extract Program of the request");
        return false;
    }
    if (!pEntry)
    {
        State(ECSJS_ERROR_INVALID_ENTRY);
        CrySimple_ERROR("failed to extract Entry of the request");
        return false;
    }
    if (!pShaderRequestLine)
    {
        State(ECSJS_ERROR_INVALID_SHADERREQUESTLINE);
        CrySimple_ERROR("failed to extract ShaderRequest of the request");
        return false;
    }
    if (!pCompileFlags)
    {
        State(ECSJS_ERROR_INVALID_COMPILEFLAGS);
        CrySimple_ERROR("failed to extract CompileFlags of the request");
        return false;
    }

    // Validate that the shader request line has a set of open/close parens as
    // the code below this expects at least the open paren to be in the string.
    // Without the open paren the code below will crash the compiler
    std::string strippedShaderRequestLine(pShaderRequestLine);
    const size_t locationOfOpenParen = strippedShaderRequestLine.find("(");
    const size_t locationOfCloseParen = strippedShaderRequestLine.find(")");
    if (locationOfOpenParen == std::string::npos ||
        locationOfCloseParen == std::string::npos || locationOfCloseParen < locationOfOpenParen)
    {
        State(ECSJS_ERROR_INVALID_SHADERREQUESTLINE);
        CrySimple_ERROR("invalid ShaderRequest attribute");
        return false;
    }

    static AZStd::atomic_long nTmpCounter = { 0 };
    ++nTmpCounter;

    const auto tmpIndex = AZStd::string::format("%ld", static_cast<long>(nTmpCounter));
    const AZ::IO::Path TmpIn = SEnviropment::Instance().m_TempPath / (tmpIndex + ".In");
    const AZ::IO::Path TmpOut = SEnviropment::Instance().m_TempPath / (tmpIndex + ".Out");
    CCrySimpleFileGuard FGTmpIn(TmpIn.c_str());
    CCrySimpleFileGuard FGTmpOut(TmpOut.c_str());
    CSTLHelper::ToFile(TmpIn.c_str(), std::vector<uint8_t>(pProgram, &pProgram[strlen(pProgram)]));

    AZ::IO::Path compilerPath = SEnviropment::Instance().m_CompilerPath;
    AZStd::string command;
    if (m_Version >= EPV_V0022)
    {
        AZStd::string compilerExecutable;
        bool validCompiler = SEnviropment::Instance().GetShaderCompilerExecutable(compiler, compilerExecutable);
        if (!validCompiler)
        {
            State(ECSJS_ERROR_INVALID_COMPILER);
            CrySimple_ERROR("Trying to compile with unknown compiler");
            return false;
        }

        AZStd::string commandStringToFormat = (compilerPath / compilerExecutable).Native();
        
#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)
        // Surrounding compiler path+executable with quotes to support spaces in the path.
        // NOTE: Executable has a space at the end on purpose, inserting quote before.
        commandStringToFormat.insert(0, "\"");
        commandStringToFormat.insert(commandStringToFormat.length()-1, "\"");
#endif
        
        commandStringToFormat.append(pCompileFlags);

        if (strstr(pCompileFlags, "-fxc") != nullptr)
        {
            AZStd::string fxcCompilerExecutable;
            bool validFXCCompiler = SEnviropment::Instance().GetShaderCompilerExecutable(SEnviropment::m_D3D11_FXC, fxcCompilerExecutable);
            if (!validFXCCompiler)
            {
                State(ECSJS_ERROR_INVALID_COMPILER);
                CrySimple_ERROR("FXC compiler executable cannot be found");
                return false;
            }
        
            AZ::IO::Path fxcLocation = compilerPath / fxcCompilerExecutable;
            
            // Handle an extra string parameter to specify the base directory where the fxc compiler is located
            command = AZStd::move(AZStd::string::format(commandStringToFormat.c_str(), fxcLocation.c_str(), pEntry, pProfile, TmpOut.c_str(), TmpIn.c_str()));
        }
        else
        {
            command = AZStd::move(AZStd::string::format(commandStringToFormat.c_str(), pEntry, pProfile, TmpOut.c_str(), TmpIn.c_str()));
        }
    }
    else
    {
        if (!ValidateExecutableStringLegacy(pCompileFlags))
        {
            State(ECSJS_ERROR_INVALID_COMPILEFLAGS);
            CrySimple_ERROR("CompileFlags failed validation");
            return false;
        }

        if (strstr(pCompileFlags, "-fxc=\"%s") != nullptr)
        {
            // Check that the string after the %s is a valid shader compiler
            // executable
            AZStd::string tempString(pCompileFlags);
            const AZStd::string::size_type fxcOffset = tempString.find("%s") + 2;
            const AZStd::string::size_type endOfFxcString = tempString.find(" ", fxcOffset);
            tempString = tempString.substr(fxcOffset, endOfFxcString);
            if (!ValidateExecutableStringLegacy(tempString))
            {
                State(ECSJS_ERROR_INVALID_COMPILEFLAGS);
                CrySimple_ERROR("CompileFlags failed validation");
                return false;
            }

            // Handle an extra string parameter to specify the base directory where the fxc compiler is located
            command = AZStd::move(AZStd::string::format(pCompileFlags, compilerPath.c_str(), pEntry, pProfile, TmpOut.c_str(), TmpIn.c_str()));

            // Need to add the string for escaped quotes around the path to the compiler. This is in case the path has spaces.
            // Adding just quotes (escaped) doesn't work because this cmd line is used to execute another process.
            AZStd::string insertPattern = "\\\"";

            // Search for the next space until that path exists. Then we assume that's the path to the executable.
            size_t startPos = command.find(compilerPath.Native());
            for (size_t pos = command.find(" ", startPos); pos != AZStd::string::npos; pos = command.find(" ", pos + 1))
            {
                if (AZ::IO::SystemFile::Exists(command.substr(startPos, pos - startPos).c_str()))
                {
                    command.insert(pos, insertPattern);
                    command.insert(startPos, insertPattern);
                }
            }
        }
        else
        {
            command = AZStd::move(AZStd::string::format(pCompileFlags, pEntry, pProfile, TmpOut.c_str(), TmpIn.c_str()));
        }
        
        command = compilerPath.Native() + command;
    }

    AZStd::string hardwareTarget;

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
    #if defined(TOOLS_SUPPORT_JASPER)
        #define AZ_RESTRICTED_SECTION CRYSIMPLEJOBCOMPILE_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleJobCompile_cpp, jasper)
    #endif
    #if defined(TOOLS_SUPPORT_PROVO)
        #define AZ_RESTRICTED_SECTION CRYSIMPLEJOBCOMPILE_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleJobCompile_cpp, provo)
    #endif
    #if defined(TOOLS_SUPPORT_SALEM)
        #define AZ_RESTRICTED_SECTION CRYSIMPLEJOBCOMPILE_CPP_SECTION_1
#include AZ_RESTRICTED_FILE_EXPLICIT(CrySimpleJobCompile_cpp, salem)
    #endif
#endif

    int64_t t0 = g_Timer.GetTime();

    std::string outError;

    std::string shaderName;
    std::stringstream crcStringStream;


    // Dump source shader
    if (SEnviropment::Instance().m_DumpShaders)
    {
        unsigned long crc = crc32(0l, Z_NULL, 0);
        // shader permutations start with '('
        size_t position = strippedShaderRequestLine.find('(');
        // split the string into shader name
        shaderName = strippedShaderRequestLine.substr(0, position);
        // split the string into permutation
        std::string permutation = strippedShaderRequestLine.substr(position, strippedShaderRequestLine.length() - position);
        // replace illegal filename characters with valid ones
        AZStd::replace(shaderName.begin(), shaderName.end(), '<', '(');
        AZStd::replace(shaderName.begin(), shaderName.end(), '>', ')');
        AZStd::replace(shaderName.begin(), shaderName.end(), '/', '_');
        AZStd::replace(shaderName.begin(), shaderName.end(), '|', '+');
        AZStd::replace(shaderName.begin(), shaderName.end(), '*', '^');
        AZStd::replace(shaderName.begin(), shaderName.end(), ':', ';');
        AZStd::replace(shaderName.begin(), shaderName.end(), '?', '!');
        AZStd::replace(shaderName.begin(), shaderName.end(), '%', '$');

        crc = crc32(crc, reinterpret_cast<const unsigned char*>(permutation.c_str()), static_cast<unsigned int>(permutation.length()));
        crcStringStream << crc;
        const std::string HlslDump = shaderPath.c_str() + shaderName + "_" + crcStringStream.str() + ".hlsl";
        CSTLHelper::ToFile(HlslDump, std::vector<uint8_t>(pProgram, &pProgram[strlen(pProgram)]));
        std::ofstream crcFile;

        std::string crcFileName = shaderPath.c_str() + shaderName + "_" + crcStringStream.str() + ".txt";

        crcFile.open(crcFileName, std::ios_base::trunc);

        if (!crcFile.fail())
        {
            // store permutation
            crcFile << permutation;
        }
        else
        {
            std::cout << "Error opening file " + crcFileName << std::endl;
        }

        crcFile.close();
    }

    if (SEnviropment::Instance().m_PrintCommands)
    {
        AZ_Printf(0, "Compiler Command:\n%s\n\n", command.c_str());
    }

    if (!ExecuteCommand(command.c_str(), outError))
    {
        unsigned char* nIP = (unsigned char*) &RequestIP();
        char sIP[128];
        azsprintf(sIP, "%d.%d.%d.%d", nIP[0], nIP[1], nIP[2], nIP[3]);

        const char* pProject = pElement->Attribute("Project");
        const char* pTags = pElement->Attribute("Tags");
        const char* pEmailCCs = pElement->Attribute("EmailCCs");

        std::string project = pProject ? pProject : "Unk/";
        std::string ccs = pEmailCCs ? pEmailCCs : "";
        std::string tags = pTags ? pTags : "";

        std::string filteredError;
        AZ::IO::Path patchFilePath = TmpIn;
        patchFilePath.ReplaceFilename(AZ::IO::PathView{ AZStd::string{ TmpIn.Filename().Native() } + ".patched" });
        CSTLHelper::Replace(filteredError, outError, patchFilePath.c_str(), "%filename%"); // DXPS does its own patching
        CSTLHelper::Replace(filteredError, filteredError, TmpIn.c_str(), "%filename%");
        // replace any that don't have the full path
        CSTLHelper::Replace(filteredError, filteredError, (tmpIndex + ".In.patched").c_str(), "%filename%"); // DXPS does its own patching
        CSTLHelper::Replace(filteredError, filteredError, (tmpIndex + ".In").c_str(), "%filename%");

        CSTLHelper::Replace(filteredError, filteredError, "\r\n", "\n");

        State(ECSJS_ERROR_COMPILE);
        throw new CCompilerError(pEntry, filteredError, ccs, sIP, pShaderRequestLine, pProgram, project, platform.c_str(), compiler.c_str(), language.c_str(), tags, pProfile);
    }

    if (!CSTLHelper::FromFile(TmpOut.c_str(), rVec))
    {
        State(ECSJS_ERROR_FILEIO);
        std::string errorString("Could not read: ");
        errorString += std::string(TmpOut.c_str(), TmpOut.Native().size());
        CrySimple_ERROR(errorString.c_str());
        return false;
    }

    // Dump cross-compiled shader
    if (SEnviropment::Instance().m_DumpShaders)
    {
        AZStd::string fileExtension = language;
        AZStd::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), tolower);
        
        std::string shaderDump = shaderPath.c_str() + shaderName + "_" + crcStringStream.str() + "." + fileExtension.c_str();
        CSTLHelper::ToFile(shaderDump, rVec);
    }


    int64_t t1 = g_Timer.GetTime();
    int64_t dt = t1 - t0;
    m_GlobalCompileTime += dt;

    int millis = (int)(g_Timer.TimeToSeconds(dt) * 1000.0);
    int secondsTotal = (int)g_Timer.TimeToSeconds(m_GlobalCompileTime);
    logmessage("Compiled [%5dms|%8ds] (%s - %s - %s - %s) %s\n", millis, secondsTotal, platform.c_str(), compiler.c_str(), language.c_str(), pProfile, pEntry);

    if (hardwareTarget.empty())
    {
        logmessage("Compiled [%5dms|%8ds] (% 5s %s) %s\n", millis, secondsTotal, platform.c_str(), pProfile, pEntry);
    }
    else
    {
        logmessage("Compiled [%5dms|%8ds] (% 5s %s) %s %s\n", millis, secondsTotal, platform.c_str(), pProfile, pEntry, hardwareTarget.c_str());
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
inline bool SortByLinenum(const std::pair<int, std::string>& f1, const std::pair<int, std::string>& f2)
{
    return f1.first < f2.first;
}

CCompilerError::CCompilerError(const std::string& entry, const std::string& errortext, const std::string& ccs, const std::string& IP,
    const std::string& requestLine, const std::string& program, const std::string& project,
    const std::string& platform, const std::string& compiler, const std::string& language, const std::string& tags, const std::string& profile)
    : ICryError(COMPILE_ERROR)
    , m_entry(entry)
    , m_errortext(errortext)
    , m_IP(IP)
    , m_program(program)
    , m_project(project)
    , m_platform(platform)
    , m_compiler(compiler)
    , m_language(language)
    , m_tags(tags)
    , m_profile(profile)
    , m_uniqueID(0)
{
    m_requests.push_back(requestLine);
    Init();

    CSTLHelper::Tokenize(m_CCs, ccs, ";");
}

void CCompilerError::Init()
{
    while (!m_errortext.empty() && (m_errortext.back() == '\r' || m_errortext.back() == '\n'))
    {
        m_errortext.pop_back();
    }

    if (m_requests[0].size())
    {
        m_shader = m_requests[0];
        size_t offs = m_shader.find('>');
        if (offs != std::string::npos)
        {
            m_shader.erase(0, m_shader.find('>') + 1); // remove <2> version
        }
        offs = m_shader.find('@');
        if (offs != std::string::npos)
        {
            m_shader.erase(m_shader.find('@')); // remove everything after @
        }
        offs = m_shader.find('/');
        if (offs != std::string::npos)
        {
            m_shader.erase(m_shader.find('/')); // remove everything after / (used on xenon)
        }
    }
    else
    {
        // default to entry function
        m_shader = m_entry;
        size_t len = m_shader.length();

        // if it ends in ?S then trim those two characters
        if (m_shader[len - 1] == 'S')
        {
            m_shader.pop_back();
            m_shader.pop_back();
        }
    }

    std::vector<std::string> lines;
    CSTLHelper::Tokenize(lines, m_errortext, "\n");

    for (uint32_t i = 0; i < lines.size(); i++)
    {
        std::string& line = lines[i];

        if (line.substr(0, 5) == "error")
        {
            m_errors.push_back(std::pair<int, std::string>(-1, line));
            m_hasherrors += line;

            continue;
        }

        if (line.find(": error") == std::string::npos)
        {
            continue;
        }

        if (line.substr(0, 10) != "%filename%")
        {
            continue;
        }

        if (line[10] != '(')
        {
            continue;
        }

        uint32_t c = 11;

        int linenum = 0;
        {
            bool ln = true;
            while (c < line.length() &&
                   ((line[c] >= '0' && line[c] <= '9') || line[c] == ',' || line[c] == '-')
                   )
            {
                if (line[c] == ',')
                {
                    ln = false;        // reached column, don't save the value - just keep reading to the end
                }
                if (ln)
                {
                    linenum *= 10;
                    linenum += line[c] - '0';
                }
                c++;
            }

            if (c >= line.length())
            {
                continue;
            }

            if (line[c] != ')')
            {
                continue;
            }

            c++;
        }

        while (c < line.length() && (line[c] == ' ' || line[c] == ':'))
        {
            c++;
        }

        if (line.substr(c, 5) != "error")
        {
            continue;
        }

        m_errors.push_back(std::pair<int, std::string>(linenum, line));
        m_hasherrors += line.substr(c);
    }

    AZStd::sort(m_errors.begin(), m_errors.end(), SortByLinenum);
}

std::string CCompilerError::GetErrorLines() const
{
    std::string ret = "";

    for (uint32_t i = 0; i < m_errors.size(); i++)
    {
        if (m_errors[i].first < 0)
        {
            ret += m_errors[i].second + "\n";
        }
        else if (i > 0 && m_errors[i - 1].first < 0)
        {
            ret += "\n" + GetContext(m_errors[i].first) + "\n" + m_errors[i].second + "\n\n";
        }
        else if (i > 0 && m_errors[i - 1].first == m_errors[i].first)
        {
            ret.pop_back(); // pop extra newline
            ret += m_errors[i].second + "\n\n";
        }
        else
        {
            ret += GetContext(m_errors[i].first) + "\n" + m_errors[i].second + "\n\n";
        }
    }

    return ret;
}

std::string CCompilerError::GetContext(int linenum, int context, std::string prefix) const
{
    std::vector<std::string> lines;
    CSTLHelper::Tokenize(lines, m_program, "\n");

    std::string ret = "";

    linenum--; // line numbers start at one

    char sLineNum[16];

    for (uint32_t i = AZStd::GetMax(0U, (uint32_t)(linenum - context)); i <= AZStd::GetMin((uint32_t)lines.size() - 1U, (uint32_t)(linenum + context)); i++)
    {
        azsprintf(sLineNum, "% 3d", i + 1);

        ret += sLineNum;
        ret += " ";

        if (prefix.size())
        {
            if (i == linenum)
            {
                ret += "*";
            }
            else
            {
                ret += " ";
            }

            ret += prefix;

            ret += " ";
        }

        ret += lines[i] + "\n";
    }

    return ret;
}

void CCompilerError::AddDuplicate(ICryError* err)
{
    ICryError::AddDuplicate(err);

    if (err->GetType() == COMPILE_ERROR)
    {
        CCompilerError* comperr = (CCompilerError*)err;
        m_requests.insert(m_requests.end(), comperr->m_requests.begin(), comperr->m_requests.end());
    }
}

bool CCompilerError::Compare(const ICryError* err) const
{
    if (GetType() != err->GetType())
    {
        return GetType() < err->GetType();
    }

    CCompilerError* e = (CCompilerError*)err;

    if (m_platform != e->m_platform)
    {
        return m_platform < e->m_platform;
    }

    if (m_compiler != e->m_compiler)
    {
        return m_compiler < e->m_compiler;
    }

    if (m_language != e->m_language)
    {
        return m_language < e->m_language;
    }

    if (m_shader != e->m_shader)
    {
        return m_shader < e->m_shader;
    }

    if (m_entry != e->m_entry)
    {
        return m_entry < e->m_entry;
    }

    return Hash() < err->Hash();
}

bool CCompilerError::CanMerge(const ICryError* err) const
{
    if (GetType() != err->GetType()) // don't merge with non compile errors
    {
        return false;
    }

    CCompilerError* e = (CCompilerError*)err;

    if (m_platform != e->m_platform || m_compiler != e->m_compiler || m_language != e->m_language || m_shader != e->m_shader)
    {
        return false;
    }

    if (m_CCs.size() != e->m_CCs.size())
    {
        return false;
    }

    for (size_t a = 0, S = m_CCs.size(); a < S; a++)
    {
        if (m_CCs[a] != e->m_CCs[a])
        {
            return false;
        }
    }

    return true;
}

void CCompilerError::AddCCs(std::set<std::string>& ccs) const
{
    for (size_t a = 0, S = m_CCs.size(); a < S; a++)
    {
        ccs.insert(m_CCs[a]);
    }
}

std::string CCompilerError::GetErrorName() const
{
    return std::string("[") + m_tags + "] Shader Compile Errors in " + m_shader + " on " + m_language + " for " + m_platform + " " + m_compiler;
}

std::string CCompilerError::GetErrorDetails(EOutputFormatType outputType) const
{
    std::string errorString("");

    char sUniqueID[16], sNumDuplicates[16];
    azsprintf(sUniqueID, "%d", m_uniqueID);
    azsprintf(sNumDuplicates, "%d", NumDuplicates());

    std::string errorOutput;
    CSTLHelper::Replace(errorOutput, GetErrorLines(), "%filename%", std::string(sUniqueID) + "-" + GetFilename());

    std::string fullOutput;
    CSTLHelper::Replace(fullOutput, m_errortext, "%filename%", std::string(sUniqueID) + "-" + GetFilename());

    if (outputType == OUTPUT_HASH)
    {
        errorString = GetFilename() + m_IP + m_platform + m_compiler + m_language + m_project + m_entry + m_tags + m_profile + m_hasherrors /*+ m_requestline*/;
    }
    else if (outputType == OUTPUT_EMAIL)
    {
        errorString = std::string("=== Shader compile error in ") + m_entry + " (" + sNumDuplicates + " duplicates)\n\n";

        /////
        errorString += std::string("* From:                  ") + m_IP + " on " + m_language + " for " + m_platform + " " + m_compiler + " " + m_project;
        if (m_tags != "")
        {
            errorString += std::string(" (Tags: ") + m_tags + ")";
        }
        errorString += "\n";

        /////
        errorString += std::string("* Target profile:        ") + m_profile + "\n";

        /////
        bool hasrequests = false;
        for (uint32_t i = 0; i < m_requests.size(); i++)
        {
            if (m_requests[i].size())
            {
                errorString += std::string("* Shader request line:   ") + m_requests[i] + "\n";
                hasrequests = true;
            }
        }

        errorString += "\n";

        if (hasrequests)
        {
            errorString += "* Shader source from first listed request\n";
        }

        errorString += std::string("* Reported error(s) from ") + sUniqueID + "-" + GetFilename() + "\n\n";
        errorString += errorOutput + "\n\n";

        errorString += std::string("* Full compiler output:\n\n");
        errorString += fullOutput + "\n";
    }
    else if (outputType == OUTPUT_TTY)
    {
        errorString = std::string("===  Shader compile error in ") + m_entry + " { " + m_requests[0] + " }\n";
        // errors only
        errorString += std::string("* Reported error(s):\n\n");
        errorString += errorOutput;
        errorString += m_errortext;
    }

    return errorString;
}
