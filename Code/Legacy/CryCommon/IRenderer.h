/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "VertexFormats.h"

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/intrusive_slist.h>

 // Object states
#define OS_ALPHA_BLEND             0x1
#define OS_ADD_BLEND               0x2
#define OS_MULTIPLY_BLEND          0x4
#define OS_TRANSPARENT            (OS_ALPHA_BLEND | OS_ADD_BLEND | OS_MULTIPLY_BLEND)
#define OS_NODEPTH_TEST            0x8
#define OS_NODEPTH_WRITE           0x10
#define OS_ANIM_BLEND              0x20
#define OS_ENVIRONMENT_CUBEMAP     0x40

// Render State flags
#define GS_BLSRC_MASK              0xf
#define GS_BLSRC_ZERO              0x1
#define GS_BLSRC_ONE               0x2
#define GS_BLSRC_DSTCOL            0x3
#define GS_BLSRC_ONEMINUSDSTCOL    0x4
#define GS_BLSRC_SRCALPHA          0x5
#define GS_BLSRC_ONEMINUSSRCALPHA  0x6
#define GS_BLSRC_DSTALPHA          0x7
#define GS_BLSRC_ONEMINUSDSTALPHA  0x8
#define GS_BLSRC_ALPHASATURATE     0x9
#define GS_BLSRC_SRCALPHA_A_ZERO   0xa // separate alpha blend state
#define GS_BLSRC_SRC1ALPHA         0xb // dual source blending


#define GS_BLDST_MASK              0xf0
#define GS_BLDST_ZERO              0x10
#define GS_BLDST_ONE               0x20
#define GS_BLDST_SRCCOL            0x30
#define GS_BLDST_ONEMINUSSRCCOL    0x40
#define GS_BLDST_SRCALPHA          0x50
#define GS_BLDST_ONEMINUSSRCALPHA  0x60
#define GS_BLDST_DSTALPHA          0x70
#define GS_BLDST_ONEMINUSDSTALPHA  0x80
#define GS_BLDST_ONE_A_ZERO        0x90 // separate alpha blend state
#define GS_BLDST_ONEMINUSSRC1ALPHA 0xa0 // dual source blending

#define GS_DEPTHWRITE              0x00000100

#define GS_COLMASK_RT1             0x00000200
#define GS_COLMASK_RT2             0x00000400
#define GS_COLMASK_RT3             0x00000800

#define GS_NOCOLMASK_R             0x00001000
#define GS_NOCOLMASK_G             0x00002000
#define GS_NOCOLMASK_B             0x00004000
#define GS_NOCOLMASK_A             0x00008000
#define GS_COLMASK_RGB             (GS_NOCOLMASK_A)
#define GS_COLMASK_A               (GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_B)
#define GS_COLMASK_NONE            (GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_B | GS_NOCOLMASK_A)
#define GS_COLMASK_MASK            GS_COLMASK_NONE
#define GS_COLMASK_SHIFT           12

#define GS_WIREFRAME               0x00010000
#define GS_NODEPTHTEST             0x00040000

#define GS_BLEND_MASK              0x0f0000ff

#define GS_DEPTHFUNC_LEQUAL        0x00000000
#define GS_DEPTHFUNC_EQUAL         0x00100000
#define GS_DEPTHFUNC_GREAT         0x00200000
#define GS_DEPTHFUNC_LESS          0x00300000
#define GS_DEPTHFUNC_GEQUAL        0x00400000
#define GS_DEPTHFUNC_NOTEQUAL      0x00500000
#define GS_DEPTHFUNC_HIZEQUAL      0x00600000 // keep hi-z test, always pass fine depth. Useful for debug display
#define GS_DEPTHFUNC_ALWAYS        0x00700000
#define GS_DEPTHFUNC_MASK          0x00700000

#define GS_STENCIL                 0x00800000

#define GS_BLEND_OP_MASK           0x03000000
#define GS_BLOP_MAX                0x01000000
#define GS_BLOP_MIN                0x02000000

// Separate alpha blend mode
#define GS_BLALPHA_MASK            0x0c000000
#define GS_BLALPHA_MIN             0x04000000
#define GS_BLALPHA_MAX             0x08000000

#define GS_ALPHATEST_MASK          0xf0000000
#define GS_ALPHATEST_GREATER       0x10000000
#define GS_ALPHATEST_LESS          0x20000000
#define GS_ALPHATEST_GEQUAL        0x40000000
#define GS_ALPHATEST_LEQUAL        0x80000000
//////////////////////////////////////////////////////////////////////
#define R_SOLID_MODE    0
#define R_WIREFRAME_MODE 1

#define MAX_NUM_VIEWPORTS 7


constexpr float UIDRAW_TEXTSIZEFACTOR = 12.0f;
constexpr float MIN_RESOLUTION_SCALE = 0.25f;
constexpr float MAX_RESOLUTION_SCALE = 4.0f;

//////////////////////////////////////////////////////////////////////
// All possible primitive types

enum class PublicRenderPrimitiveType
{
    prtTriangleList,
    prtTriangleStrip,
    prtLineList,
    prtLineStrip
};

// Summary:
//   Flags used in DrawText function.
// See also:
//   SDrawTextInfo
// Remarks:
//   Text must be fixed pixel size.
enum EDrawTextFlags
{
    eDrawText_Left = 0,        // default left alignment if neither Center or Right are specified
    eDrawText_Center = BIT(0),   // centered alignment, otherwise right or left
    eDrawText_Right = BIT(1),   // right alignment, otherwise center or left
    eDrawText_CenterV = BIT(2),   // center vertically, otherwise top
    eDrawText_Bottom = BIT(3),   // bottom alignment

    eDrawText_2D = BIT(4),   // 3 component vector is used for xy screen position, otherwise it's 3d world space position

    eDrawText_FixedSize = BIT(5),   // font size is defined in the actual pixel resolution, otherwise it's in the virtual 800x600
    eDrawText_800x600 = BIT(6),   // position are specified in the virtual 800x600 resolution, otherwise coordinates are in pixels

    eDrawText_Monospace = BIT(7),   // non proportional font rendering (Font width is same for all characters)

    eDrawText_Framed = BIT(8),   // draw a transparent, rectangular frame behind the text to ease readability independent from the background

    eDrawText_DepthTest = BIT(9),   // text should be occluded by world geometry using the depth buffer
    eDrawText_IgnoreOverscan = BIT(10),  // ignore the overscan borders, text should be drawn at the location specified
    eDrawText_UseTransform = BIT(11),  // use a transform for the text
};


