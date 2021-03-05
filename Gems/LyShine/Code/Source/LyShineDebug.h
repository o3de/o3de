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

#ifndef _RELEASE
#include <AzCore/std/containers/vector.h>

class ITexture;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Class for drawing test displays for testing the LyShine functionality
//
//! This is currently implemented as console variables and commands
class LyShineDebug
{
public: // static member functions

    //! Initialize debug vars
    static void Initialize();

    //! This is called when the game terminates
    static void Reset();

    //! Do the debug render
    static void RenderDebug();

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dFont, 0);

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dImage, 0);

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dLine, 0);

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dDefer, 0);

#ifndef _RELEASE
    //! Shared structures used for debug console commands

    struct DebugInfoRenderGraph
    {
        int m_numPrimitives;
        int m_numRenderNodes;
        int m_numTriangles;
        int m_numUniqueTextures;
        int m_numMasks;
        int m_numRTs;
        int m_numNodesDueToMask;
        int m_numNodesDueToRT;
        int m_numNodesDueToBlendMode;
        int m_numNodesDueToSrgb;
        int m_numNodesDueToMaxVerts;
        int m_numNodesDueToTextures;
        bool m_wasBuiltThisFrame;
        AZ::u64 m_timeGraphLastBuiltMs;
        bool m_isReusingRenderTargets;
    };

    struct DebugInfoTextureUsage
    {
        ITexture*   m_texture;
        bool        m_isClampTextureUsage;
        int         m_numCanvasesUsed;
        int         m_numDrawCallsUsed;
        int         m_numDrawCallsWhereExceedingMaxTextures;
        void*       m_lastContextUsed;
    };

    struct DebugInfoDrawCallReport
    {
        AZStd::vector<DebugInfoTextureUsage> m_textures;
    };
#endif
};