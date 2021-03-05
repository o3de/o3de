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
#include "ConvertContext.h"
#include "IConfig.h"
#include "FileUtil.h"
#include "UpToDateFileHelpers.h"

#include "LuaCompiler.h"

extern "C"
{
#include <Lua/lua.h>
#include <Lua/lauxlib.h>

#include <Lua/ldo.h>
#include <Lua/lfunc.h>
#include <Lua/lmem.h>
#include <Lua/lobject.h>
#include <Lua/lopcodes.h>
#include <Lua/lstring.h>
#include <Lua/lundump.h>
}

extern "C"
{
float script_frand0_1()
{
    return rand() / static_cast<float>(RAND_MAX);
}

void script_randseed(uint seed)
{
    srand(seed);
}
}

// Shamelessly stolen from luac.c and modified a bit to support rc usage

#define toproto(L, i) (clvalue(L->top + (i))->l.p)

static const Proto* combine(lua_State* L)
{
    return toproto(L, -1);
}

static int writer(lua_State* L, const void* p, size_t size, void* u)
{
    UNUSED(L);
    return (fwrite(p, size, 1, (FILE*)u) != 1) && (size != 0);
}

static int pmain(lua_State* L)
{
    LuaCompiler* pCompiler = reinterpret_cast<LuaCompiler*>(lua_touserdata(L, 1));
    const Proto* f;
    const char* filename = pCompiler->GetInFilename();
    if (luaL_loadfile(L, filename) != 0)
    {
        RCLogError(lua_tostring(L, -1));
        return 1;
    }

    f = combine(L);
    if (pCompiler->IsDumping())
    {
        const char* outputFilename = pCompiler->GetOutFilename();

        FILE* D = nullptr; 
        azfopen(&D, outputFilename, "wb");
        if (D == NULL)
        {
            RCLogError("Cannot open %s", outputFilename);
            return 1;
        }

        lua_lock(L);
        luaU_dump(L, f, writer, D, (int)pCompiler->IsStripping());
        lua_unlock(L);

        if (ferror(D))
        {
            RCLogError("Cannot write to %s", outputFilename);
            return 1;
        }

        if (fclose(D))
        {
            RCLogError("Cannot close %s", outputFilename);
            return 1;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
LuaCompiler::LuaCompiler()
    : m_bIsDumping(true)
    , m_bIsStripping(true)
    , m_bIsBigEndian(false)
{
    m_refCount = 1;
}

//////////////////////////////////////////////////////////////////////////
LuaCompiler::~LuaCompiler()
{
}

////////////////////////////////////////////////////////////
string LuaCompiler::GetOutputFileNameOnly() const
{
    return PathHelpers::RemoveExtension(m_CC.m_sourceFileNameOnly) + ".lua";
}

////////////////////////////////////////////////////////////
string LuaCompiler::GetOutputPath() const
{
    return PathHelpers::Join(m_CC.GetOutputFolder(), GetOutputFileNameOnly());
}

//////////////////////////////////////////////////////////////////////////
void LuaCompiler::Release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
ICompiler* LuaCompiler::CreateCompiler()
{
    // Only ever return one compiler, since we don't support multithreading. Since
    // the compiler is just this object, we can tell whether we have already returned
    // a compiler by checking the ref count.
    if (m_refCount >= 2)
    {
        return 0;
    }

    // Until we support multithreading for this convertor, the compiler and the
    // convertor may as well just be the same object.
    ++m_refCount;
    return this;
}

//////////////////////////////////////////////////////////////////////////
bool LuaCompiler::Process()
{
    string sourceFile = m_CC.GetSourcePath();
    string outputFile = GetOutputPath();
    std::replace(sourceFile.begin(), sourceFile.end(), '/', '\\');
    std::replace(outputFile.begin(), outputFile.end(), '/', '\\');

    if (!m_CC.m_bForceRecompiling && UpToDateFileHelpers::FileExistsAndUpToDate(GetOutputPath(), m_CC.GetSourcePath()))
    {
        // The file is up-to-date
        m_CC.m_pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());
        return true;
    }

    bool ok = false;

    const bool isPlatformBigEndian = m_CC.m_pRC->GetPlatformInfo(m_CC.m_platform)->bBigEndian;

    m_bIsDumping = true;
    m_bIsStripping = true;
    m_bIsBigEndian = isPlatformBigEndian;
    m_sInFilename = sourceFile;
    m_sOutFilename = outputFile;

    lua_State* L = luaL_newstate();

    if (L)
    {
        lua_pushcfunction(L, pmain);
        lua_pushlightuserdata(L, this);

        if (lua_pcall(L, 1, 0, 0) == 0)
        {
            ok = true;
        }
        else
        {
            RCLogError(lua_tostring(L, -1));
        }

        lua_close(L);
    }
    else
    {
        RCLogError("Not enough memory for lua state");
    }

    if (ok)
    {
        if (!UpToDateFileHelpers::SetMatchingFileTime(GetOutputPath(), m_CC.GetSourcePath()))
        {
            return false;
        }
        m_CC.m_pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());
    }

    return ok;
}
