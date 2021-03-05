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

#include "ResourceCompilerPC_precompiled.h"
#include "CryAssert_impl.h"
#include "StatCGFCompiler.h"
#include "ChunkCompiler.h"
#include "LuaCompiler.h"
#include "CGF/AssetWriter.h"
#include "../../CryXML/ICryXML.h"
#include <AzCore/Module/Environment.h>
#include <AzCore/Utils/Utils.h>
#include <CryLibrary.h>
#include <ResourceCompiler/ResourceCompiler.h>

#if defined(AZ_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>    // HANDLE
#endif

#include "ResourceCompilerPC.h"

static HMODULE g_hInst;

static AssetWriter g_AssetWriter;

#if defined(AZ_PLATFORM_WINDOWS) && !defined(AZ_MONOLITHIC_BUILD)
BOOL APIENTRY DllMain(
    HANDLE hModule,
    DWORD  ul_reason_for_call,
    [[maybe_unused]] LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = (HMODULE)hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif

extern "C" DLL_EXPORT void __stdcall RegisterConvertors(IResourceCompiler* const pRC)
{
    PREVENT_MODULE_AND_ENVIRONMENT_SYMBOL_STRIPPING

    SetRCLog(pRC->GetIRCLog());

    pRC->RegisterConvertor("StatCGFCompiler", new CStatCGFCompiler());

    pRC->RegisterConvertor("ChunkCompiler", new CChunkCompiler());

    pRC->RegisterConvertor("LuaCompiler", new LuaCompiler());

    pRC->SetAssetWriter(&g_AssetWriter);

    ICryXML* const pCryXML = LoadICryXML();

    if (pCryXML == 0)
    {
        RCLogError("Loading xml library failed - not registering collada converter.");
    }

    pRC->RegisterKey("createmtl", "[DAE] 0=don't create .mtl files (default), 1=create .mtl files");

    pRC->RegisterKey("file", "animation file for processing");
    pRC->RegisterKey("dest", "destination folder for the results\n"
        "OBSOLETE. Use 'targetroot' pointing to folder with .cba file instead.");
    pRC->RegisterKey("report", "report mode");
    pRC->RegisterKey("dcc", "the name of the dcc that called the rc.");
    pRC->RegisterKey("dccv", "the version of the dcc that called the rc.");
    pRC->RegisterKey("SkipDba", "skip build dba");
    pRC->RegisterKey("animConfigFolder", "Path to a folder that contains SkeletonList.xml and DBATable.json");
    pRC->RegisterKey("cbaUpdate", "Check for CBA-update only. Do not recompile CAF-s when CBA is up to date");
    pRC->RegisterKey("checkloco",
        "should be used with report mode.\n"
        "Compare locomotion_locator motion with recalculated root motion");
    pRC->RegisterKey("debugcompression", "[I_CAF] show per-bone compression values during CAF-compression");
    pRC->RegisterKey("ignorepresets", "[I_CAF] do not apply compression presets");
    pRC->RegisterKey("animSettingsFile", "File to use instead of the default animation settings file");
    pRC->RegisterKey("cafAlignTracks", "[I_CAF] Apply padding to animation tracks to make the CAF suitable for in-place streaming");
    pRC->RegisterKey("dbaStreamPrepare", "[DBA] Prepare DBAs so they can be streamed in-place");

    pRC->RegisterKey("qtangents", "0=use vectors to represent tangent space(default), 1=use quaternions");

    pRC->RegisterKey("vertexPositionFormat",
        "[CGF] Format of mesh vertex positions:\n"
        "f32 = 32-bit floating point (default)\n"
        "f16 = 16-bit floating point\n"
        "exporter = format specified in exporter\n");

    pRC->RegisterKey("vertexIndexFormat",
        "[CGF] Format of mesh vertex indices:\n"
        "u32 = 32-bit unsigned integer (default)\n"
        "u16 = 16-bit unsigned integer\n");

    pRC->RegisterKey("debugdump", "[CGF] dump contents of source .cgf file instead of compiling it");
    pRC->RegisterKey("debugvalidate", "[CGF, CHR] validate source file instead of compiling it");

    pRC->RegisterKey("targetversion", "[chunk] Convert chunk file to the specified version\n"
        "0x745 = chunk data contain chunk headers\n"
        "0x746 = chunk data has no chunk headers (default)\n");

    pRC->RegisterKey("StripMesh", "[CGF/CHR] Strip mesh chunks from output files\n"
        "0 = No stripping\n"
        "1 = Only strip mesh\n"
        "3 = [CHR] Treat input as a skin file, stripping all unnecessary chunks (including mesh)\n"
        "4 = [CHR] Treat input as a skel file, stripping all unnecessary chunks (including mesh)");
    pRC->RegisterKey("StripNonMesh", "[CGF/CHR] Strip non mesh chunks from the output files");
    pRC->RegisterKey("CompactVertexStreams",
        "[CGF] Optimise vertex streams for streaming, by removing those that are unneeded for streaming,\n"
        "and packing those streams that are left into the format used internally by the engine.");
    // Confetti: Nicholas Baldwin
    //    add option for outputting triangle strip mesh primitives for processors that prefer them.
    pRC->RegisterKey("OptimizedPrimitiveType", "[CGF/CHR] Choose the preferred optimized mesh primitive type\n"
        "0 = Forsyth Indexed Triangle Lists Algorithm (default)\n"
        "1 = PowerVR Indexed Triangle Strips Lists Algorithm");
    pRC->RegisterKey("ComputeSubsetTexelDensity", "[CGF] Compute per-subset texel density");
    pRC->RegisterKey("SplitLODs", "[CGF] Auto split LODs into the separate files");

    pRC->RegisterKey("maxWeightsPerVertex", "[CHR] Maximum number of weights per vertex (default is 4)");

    pRC->RegisterKey("DegenerateFacesAreErrors", "If the meshcompiler finds a degenerate face, it is a suboptimal mesh. Should the we treat this as a warning or error.");
}
