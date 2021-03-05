
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

#include "Cry3DEngine_precompiled.h"
#include <Cry3DEngineBase.h>

#define MAX_ERROR_STRING MAX_WARNING_LENGTH

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::PrintComment(const char* szText, ...)
{
    if (!szText)
    {
        return;
    }

    va_list args;
    va_start(args, szText);
    GetLog()->LogV(IMiniLog::eComment, szText, args);
    va_end(args);
}

void Cry3DEngineBase::PrintMessage(const char* szText, ...)
{
    if (!szText)
    {
        return;
    }

    va_list args;
    va_start(args, szText);
    GetLog()->LogV(GetCVars()->e_3dEngineLogAlways ? IMiniLog::eAlways : IMiniLog::eMessage, szText, args);
    va_end(args);

    GetLog()->UpdateLoadingScreen(0);
}

void Cry3DEngineBase::PrintMessagePlus(const char* szText, ...)
{
    if (!szText)
    {
        return;
    }

    va_list arglist;
    char buf[MAX_ERROR_STRING];
    va_start(arglist, szText);
    int count = azvsnprintf(buf, sizeof(buf), szText, arglist);
    if (count == -1 || count >= sizeof(buf))
    {
        buf[sizeof(buf) - 1] = '\0';
    }
    va_end(arglist);
    GetLog()->LogPlus(buf);

    GetLog()->UpdateLoadingScreen(0);
}

float Cry3DEngineBase::GetCurTimeSec()
{
    return (gEnv->pTimer->GetCurrTime());
}

float Cry3DEngineBase::GetCurAsyncTimeSec()
{
    return (gEnv->pTimer->GetAsyncTime().GetSeconds());
}

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::Warning(const char* format, ...)
{
    if (!format)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    // Call to validating warning of system.
    m_pSystem->WarningV(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, 0, 0, format, args);
    va_end(args);

    GetLog()->UpdateLoadingScreen(0);
}

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::Error(const char* format, ...)
{
    //  assert(!"Cry3DEngineBase::Error");
    if (format)
    {
        va_list args;
        va_start(args, format);
        // Call to validating warning of system.
        m_pSystem->WarningV(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, 0, 0, format, args);
        va_end(args);
    }

    GetLog()->UpdateLoadingScreen(0);
}

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::FileWarning(int flags, const char* file, const char* format, ...)
{
    if (format)
    {
        va_list args;
        va_start(args, format);
        // Call to validating warning of system.
        m_pSystem->WarningV(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, flags | VALIDATOR_FLAG_FILE, file, format, args);
        va_end(args);
    }

    GetLog()->UpdateLoadingScreen(0);
}

_smart_ptr<IMaterial> Cry3DEngineBase::MakeSystemMaterialFromShader(const char* sShaderName, SInputShaderResources* Res)
{
    _smart_ptr<IMaterial> pMat = Get3DEngine()->GetMaterialManager()->CreateMaterial(sShaderName);
    //pMat->AddRef();
    SShaderItem si;

    si = GetRenderer()->EF_LoadShaderItem(sShaderName, true, 0, Res);

    pMat->AssignShaderItem(si);
    return pMat;
}

//////////////////////////////////////////////////////////////////////////
bool Cry3DEngineBase::IsValidFile(const char* sFilename)
{
    LOADING_TIME_PROFILE_SECTION;

    return gEnv->pCryPak->IsFileExist(sFilename);
}

//////////////////////////////////////////////////////////////////////////
bool Cry3DEngineBase::IsResourceLocked(const char* sFilename)
{
    auto pResList = GetPak()->GetResourceList(AZ::IO::IArchive::RFOM_NextLevel);
    if (pResList)
    {
        return pResList->IsExist(sFilename);
    }
    return false;
}

void Cry3DEngineBase::DrawBBoxLabeled(const AABB& aabb, const Matrix34& m34, const ColorB& col, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char szText[256];
    vsnprintf_s(szText, sizeof(szText), sizeof(szText) - 1, format, args);
    float fColor[4] = { col[0] / 255.f, col[1] / 255.f, col[2] / 255.f, col[3] / 255.f };
    GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
    GetRenderer()->DrawLabelEx(m34.TransformPoint(aabb.GetCenter()), 1.3f, fColor, true, true, szText);
    GetRenderer()->GetIRenderAuxGeom()->DrawAABB(aabb, m34, false, col, eBBD_Faceted);
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
void Cry3DEngineBase::DrawBBox(const Vec3& vMin, const Vec3& vMax, ColorB col)
{
    GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
    GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vMin, vMax), false, col, eBBD_Faceted);
}

void Cry3DEngineBase::DrawBBox(const AABB& box, ColorB col)
{
    GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
    GetRenderer()->GetIRenderAuxGeom()->DrawAABB(box, false, col, eBBD_Faceted);
}

void Cry3DEngineBase::DrawLine(const Vec3& vMin, const Vec3& vMax, ColorB col)
{
    GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
    GetRenderer()->GetIRenderAuxGeom()->DrawLine(vMin, col, vMax, col);
}

void Cry3DEngineBase::DrawSphere(const Vec3& vPos, float fRadius, ColorB color)
{
    GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
    GetRenderer()->GetIRenderAuxGeom()->DrawSphere(vPos, fRadius, color);
}

void Cry3DEngineBase::DrawQuad(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, ColorB color)
{
    GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
    GetRenderer()->GetIRenderAuxGeom()->DrawTriangle(v0, color, v2, color, v3, color);
    GetRenderer()->GetIRenderAuxGeom()->DrawTriangle(v0, color, v1, color, v2, color);
}

// Check if preloading is enabled.
bool Cry3DEngineBase::IsPreloadEnabled()
{
    bool bPreload = false;
    ICVar* pSysPreload = GetConsole()->GetCVar("sys_preload");
    if (pSysPreload && pSysPreload->GetIVal() != 0)
    {
        bPreload = true;
    }

    return bPreload;
}

//////////////////////////////////////////////////////////////////////////
bool Cry3DEngineBase::CheckMinSpec(uint32 nMinSpec)
{
    if ((int)nMinSpec != 0 && GetCVars()->e_ObjQuality != 0 && (int)nMinSpec > GetCVars()->e_ObjQuality)
    {
        return false;
    }

    return true;
}

bool Cry3DEngineBase::IsEscapePressed()
{
#ifdef WIN32
    if (Cry3DEngineBase::m_bEditor && (CryGetAsyncKeyState(0x03) & 1)) // Ctrl+Break
    {
        Get3DEngine()->PrintMessage("*** Ctrl-Break was pressed - operation aborted ***");
        return true;
    }
#endif
    return false;
}
