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

// A simple profiler useful for collecting multiple call times per frame
// and displaying their different average statistics.
// For usage, see the bottom of the file

#pragma once

#include <AzCore/Debug/Profiler.h>

#define PP_CONCAT2(A, B) A##B
#define PP_CONCAT(A, B) PP_CONCAT2(A, B)
// set #if 0 here if you don't want profiling to be compiled in the code
//#if 0
#ifdef ENABLE_FRAME_PROFILER

// define PROFILE_RENDERER_DETAILED for additional render device events
// #define PROFILE_RENDERER_DETAILED
#ifdef PROFILE_RENDERER_DETAILED
#   define PROFILE_FRAME(id) FRAME_PROFILER_FAST("Renderer:" #id, iSystem, PROFILE_RENDERER, g_bProfilerEnabled)
#else
#   define PROFILE_FRAME(id)
#endif


#define PROFILE_PS_TIME_SCOPE(EXT) \
    PROFILE_PS_TIME_SCOPE_COND(EXT, true)

#define PROFILE_PS_TIME_SCOPE_COND(EXT, CONDITION)                                                                                                  \
    class CProfilePSTimeScope                                                                                                                       \
    {                                                                                                                                               \
        bool m_bCondition;                                                                                                                          \
        CTimeValue m_startTime;                                                                                                                     \
    public:                                                                                                                                         \
        CProfilePSTimeScope(bool bCondition)                                                                                                        \
        {                                                                                                                                           \
            m_bCondition = bCondition;                                                                                                              \
            if (bCondition) {                                                                                                                       \
                m_startTime = iTimer->GetAsyncTime(); }                                                                                             \
        }                                                                                                                                           \
        ~CProfilePSTimeScope()                                                                                                                      \
        {                                                                                                                                           \
            if (m_bCondition) {                                                                                                                     \
                gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_##EXT += iTimer->GetAsyncTime().GetDifferenceInSeconds(m_startTime); } \
        }                                                                                                                                           \
    } PP_CONCAT(profilePSTimeScope, __LINE__)(CONDITION);
#define PROFILE_DIPS_START                       \
    CTimeValue TimeDIP = iTimer->GetAsyncTime(); \

#define PROFILE_DIPS_END(id)                                                                                                              \
    gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fTimeDIPs[id] += iTimer->GetAsyncTime().GetDifferenceInSeconds(TimeDIP); \

// to get around a stupid compiler bug (Win32 debug with Edit and Continue enabled) where assert can't be used
#if defined(_DEBUG)
#define FP_CHECK_SHADER      if (!gRenDev->m_RP.m_pShader) {__debugbreak(); }
#else
#define FP_CHECK_SHADER
#endif

#define PROFILE_SHADER_SCOPE                                                                                                                                                                     \
    class CProfileShaderScope                                                                                                                                                                    \
    {                                                                                                                                                                                            \
        bool bProfile;                                                                                                                                                                           \
        bool bDoEnd;                                                                                                                                                                             \
        float time0;                                                                                                                                                                             \
        int nNumDips;                                                                                                                                                                            \
        int nNumPolys;                                                                                                                                                                           \
    public:                                                                                                                                                                                      \
        CProfileShaderScope()                                                                                                                                                                    \
        {                                                                                                                                                                                        \
            bDoEnd = true;                                                                                                                                                                       \
            time0 = 0;                                                                                                                                                                           \
            nNumDips = 0;                                                                                                                                                                        \
            nNumPolys = 0;                                                                                                                                                                       \
            if (CRenderer::CV_r_profileshaders == 1 || (CRenderer::CV_r_profileshaders == 2 && gRenDev->m_RP.m_pCurObject && (gRenDev->m_RP.m_pCurObject->m_ObjFlags & FOB_SELECTED)))     \
            {                                                                                                                                                                                    \
                bProfile = true;                                                                                                                                                                 \
                time0 = iTimer->GetAsyncCurTime();                                                                                                                                               \
                gRenDev->m_RP.m_fProfileTime = time0;                                                                                                                                         \
                nNumPolys = gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_nPolygons[gRenDev->m_RP.m_nPassGroupDIP];                                                            \
                nNumDips = gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_nDIPs[gRenDev->m_RP.m_nPassGroupDIP];                                                                 \
            }                                                                                                                                                                                    \
            else                                                                                                                                                                                 \
            {                                                                                                                                                                                    \
                bProfile = false;                                                                                                                                                                \
            }                                                                                                                                                                                    \
        }                                                                                                                                                                                        \
        ~CProfileShaderScope()                                                                                                                                                                   \
        {   End(); }                                                                                                                                                                             \
        void End()                                                                                                                                                                               \
        {                                                                                                                                                                                        \
            if (!bDoEnd) {                                                                                                                                                                       \
                return; }                                                                                                                                                                        \
            bDoEnd = false;                                                                                                                                                                      \
            float fTime = 0;                                                                                                                                                                     \
            if (bProfile)                                                                                                                                                                        \
            {                                                                                                                                                                                    \
                float time1 = iTimer->GetAsyncCurTime();                                                                                                                                         \
                fTime = time1 - time0;                                                                                                                                                           \
            }                                                                                                                                                                                    \
                                                                                                                                                                                                 \
            if (gRenDev->m_RP.m_pShader && gRenDev->m_RP.m_pCurTechnique)                                                                                                                  \
            {                                                                                                                                                                                    \
                if (CRenderer::CV_r_profileshaders == 1 || (CRenderer::CV_r_profileshaders == 2 && gRenDev->m_RP.m_pCurObject && (gRenDev->m_RP.m_pCurObject->m_ObjFlags & FOB_SELECTED))) \
                {                                                                                                                                                                                \
                    if (time0 == gRenDev->m_RP.m_fProfileTime)                                                                                                                                \
                    {                                                                                                                                                                            \
                        SProfInfo pi;                                                                                                                                                            \
                        pi.Time = fTime;                                                                                                                                                         \
                        pi.NumPolys = gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_nPolygons[gRenDev->m_RP.m_nPassGroupDIP] - nNumPolys;                                      \
                        pi.NumDips = gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_nDIPs[gRenDev->m_RP.m_nPassGroupDIP] - nNumDips;                                            \
                        FP_CHECK_SHADER;                                                                                                                                                         \
                        pi.pShader = gRenDev->m_RP.m_pShader;                                                                                                                                 \
                        pi.pTechnique = gRenDev->m_RP.m_pCurTechnique;                                                                                                                        \
                        pi.m_nItems = 0;                                                                                                                                                         \
                        gRenDev->m_RP.m_Profile.AddElem(pi);                                                                                                                                  \
                    }                                                                                                                                                                            \
                }                                                                                                                                                                                \
            }                                                                                                                                                                                    \
        }                                                                                                                                                                                        \
    } profileShaderScope;

#define PROFILE_SHADER_START \
    PROFILE_SHADER_SCOPE

#define PROFILE_SHADER_END \
    profileShaderScope.End();

#else
#define PROFILE_FRAME(id)
#define PROFILE_SHADER_SCOPE
#define PROFILE_SHADER_START
#define PROFILE_SHADER_END
#define PROFILE_PS_TIME_SCOPE(EXT)
#define PROFILE_PS_TIME_SCOPE_COND(EXT, CONDITION)
#define PROFILE_DIPS_START
#define PROFILE_DIPS_END(id)
#endif

#if defined(ENABLE_FRAME_PROFILER_LABELS)

// define these to instrument push/pop marker for GPU, also added to the internal profiler and CPU Markers
    #define PROFILE_LABEL(X)
    #define PROFILE_LABEL_PUSH(X)
    #define PROFILE_LABEL_POP(X)

// scope util class for GPU profiling Marker
    #define PROFILE_LABEL_SCOPE(X)            \
    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Renderer, X); \
    class CProfileLabelScope                  \
    {                                         \
        const char* m_label;                  \
        AZ::Debug::EventTrace::ScopedSlice m_slice; \
    public:                                   \
        CProfileLabelScope(const char* label) \
            : m_label(label)                  \
            , m_slice(label, "Renderer") \
        { PROFILE_LABEL_PUSH(label); }        \
        ~CProfileLabelScope()                 \
        { PROFILE_LABEL_POP(m_label); }       \
    } PP_CONCAT(profileLabelScope, __LINE__)(X);

// scope util class for GPU profiling Marker with a dynamic string name
#define PROFILE_LABEL_SCOPE_DYNAMIC(X)            \
    AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::Renderer, "%s", X); \
    class CProfileLabelScope                  \
    {                                         \
        const char* m_label;                  \
        AZ::Debug::EventTrace::ScopedSlice m_slice; \
    public:                                   \
        CProfileLabelScope(const char* label) \
            : m_label(label)                  \
            , m_slice(label, "Renderer") \
        { PROFILE_LABEL_PUSH(label); }        \
        ~CProfileLabelScope()                 \
        { PROFILE_LABEL_POP(m_label); }       \
    } PP_CONCAT(profileLabelScope, __LINE__)(X);

#else
    #define PROFILE_LABEL(X)
    #define PROFILE_LABEL_PUSH(X)
    #define PROFILE_LABEL_POP(X)
    #define PROFILE_LABEL_SCOPE(X)
#endif

#define PROFILE_LABEL_SHADER(X) PROFILE_LABEL(X)


#if defined(ENABLE_FRAME_PROFILER) && !defined(_RELEASE)
    #define FUNCTION_PROFILER_RENDER_FLAT \
        FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_RENDERER) \
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::RendererDetailed)
#else
    #define FUNCTION_PROFILER_RENDER_FLAT
#endif
