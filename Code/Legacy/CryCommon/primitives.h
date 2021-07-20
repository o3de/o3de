/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_PRIMITIVES_H
#define CRYINCLUDE_CRYCOMMON_PRIMITIVES_H
#pragma once



typedef int index_t;
enum
{
    PHYS_MAX_INDICES = 1 << 30
};

#include "stridedptr.h"

namespace primitives {
    ////////////////////////// primitives //////////////////////

    struct primitive
    {
    };

    struct box
        : primitive
    {
        enum entype
        {
            type = 0
        };
        Matrix33 Basis; // v_box = Basis*v_world; Basis = Rotation.T()
        int bOriented;
        Vec3 center;
        Vec3 size;
        AUTO_STRUCT_INFO
    };

    struct triangle
        : primitive
    {
        enum entype
        {
            type = 1
        };
        Vec3 pt[3];
        Vec3 n;
        AUTO_STRUCT_INFO
    };

    struct indexed_triangle
        : triangle
    {
        int idx;
        AUTO_STRUCT_INFO
    };

    typedef float (* getHeightCallback)(int ix, int iy);
    typedef unsigned char (* getSurfTypeCallback)(int ix, int iy);

    struct grid
        : primitive
    {
        Matrix33 Basis;
        int bOriented;
        Vec3 origin;
        vector2df step, stepr;
        vector2di size;
        vector2di stride;
        int bCyclic;
        grid() { bCyclic = 0; }
        int inrange(int ix, int iy) {   return bCyclic | -((ix - size.x & - 1 - ix & iy - size.y & - 1 - iy) >> 31); }
        int getcell_safe(int ix, int iy) { int mask = -inrange(ix, iy); return (iy & size.y - 1) * stride.y + (ix & size.x - 1) * stride.x & mask | size.x * size.y & ~mask; }
        int crop(int i, int icoord, int bAllowBorder = 1) { int brd = bAllowBorder + (1 << 30 & - bCyclic); return max(-brd, min(size[icoord] - 1 + brd, i)); }
        vector2di cropxy(const vector2di& ic, int bAllowBorder = 1) { int brd = bAllowBorder + (1 << 30 & - bCyclic); return vector2di(max(-brd, min(size.x - 1 + brd, ic.x)), max(-brd, min(size.y - 1 + brd, ic.y))); }
        int iscyclic() { return bCyclic; }
        AUTO_STRUCT_INFO
    };

    struct heightfield
        : grid
    {
        enum entype
        {
            type = 2
        };
        heightfield& operator=(const heightfield& src)
        {
            step = src.step;
            stepr = src.stepr;
            size = src.size;
            stride = src.stride;
            heightscale = src.heightscale;
            typemask = src.typemask;
            typehole = src.typehole;
            typepower = src.typepower;
            fpGetHeightCallback = src.fpGetHeightCallback;
            fpGetSurfTypeCallback = src.fpGetSurfTypeCallback;
            return *this;
        }

        ILINE float getheight(int ix, int iy) const
        {
            float result = (*fpGetHeightCallback)(ix, iy);
            return result * heightscale;
        }
        ILINE int gettype(int ix, int iy) const
        {
            int itype = (((*fpGetSurfTypeCallback)(ix, iy)) & typemask) >> typepower, idelta = itype - typehole;
            return itype | ((idelta - 1) >> 31 ^ idelta >> 31);
        }

        float heightscale;
        unsigned short typemask;
        int typehole;
        int typepower;
        getHeightCallback fpGetHeightCallback;
        getSurfTypeCallback fpGetSurfTypeCallback;
    };

    struct ray
        : primitive
    {
        enum entype
        {
            type = 3
        };
        Vec3 origin;
        Vec3 dir;
        AUTO_STRUCT_INFO
    };

    struct sphere
        : primitive
    {
        enum entype
        {
            type = 4
        };
        Vec3 center;
        float r;
        AUTO_STRUCT_INFO
    };

    struct cylinder
        : primitive
    {
        enum entype
        {
            type = 5
        };
        Vec3 center;
        Vec3 axis;
        float r, hh;
        AUTO_STRUCT_INFO
    };

    struct capsule
        : cylinder
    {
        enum entype
        {
            type = 6
        };
        AUTO_STRUCT_INFO
    };

    struct grid3d
        : primitive
    {
        Matrix33 Basis;
        int bOriented;
        Vec3 origin;
        Vec3 step, stepr;
        Vec3i size;
        Vec3i stride;
    };

    struct voxelgrid
        : grid3d
    {
        enum entype
        {
            type = 7
        };
        Matrix33 R;
        Vec3 offset;
        float scale, rscale;
        strided_pointer<Vec3> pVtx;
        index_t* pIndices;
        Vec3* pNormals;
        char* pIds;
        int* pCellTris;
        int* pTriBuf;
    };

    struct plane
        : primitive
    {
        enum entype
        {
            type = 8
        };
        Vec3 n;
        Vec3 origin;
        AUTO_STRUCT_INFO
    };

    struct coord_plane
        : plane
    {
        Vec3 axes[2];
        AUTO_STRUCT_INFO
    };
}

AUTO_TYPE_INFO(primitives::getHeightCallback)
AUTO_TYPE_INFO(primitives::getSurfTypeCallback)

struct prim_inters
{
    prim_inters() { minPtDist2 = 0.0f; ptbest.zero(); }
    Vec3 pt[2];
    Vec3 n;
    unsigned char iFeature[2][2];
    float minPtDist2;
    short id[2];
    int iNode[2];
    Vec3* ptborder;
    int nborderpt, nbordersz;
    Vec3 ptbest;
    int nBestPtVal;
};

struct contact
{
    real t, taux;
    Vec3 pt;
    Vec3 n;
    unsigned int iFeature[2];
};

const int NPRIMS = 8;   // since plane is currently not supported in collision checks

///////////////////// geometry contact structures ///////////////////

struct geom_contact_area
{
    enum entype
    {
        polygon, polyline
    };
    int type;
    int npt;
    int nmaxpt;
    float minedge;
    int* piPrim[2];
    int* piFeature[2];
    Vec3* pt;
    Vec3 n1; // normal of other object surface (or edge)
};

const int IFEAT_LOG2 = 23;
const int IDXMASK = ~(0xFF << IFEAT_LOG2);
const int TRIEND = 0x80 << IFEAT_LOG2;

struct geom_contact
{
    real t;
    Vec3 pt;
    Vec3 n;
    Vec3 dir; // unprojection direction
    int iUnprojMode;
    float vel; // original velocity along this direction, <0 if least squares normal was used
    int id[2]; // external ids for colliding geometry parts
    int iPrim[2];
    int iFeature[2];
    int iNode[2]; // BV-tree nodes of contacting primitives
    Vec3* ptborder; // intersection border
    int (*idxborder)[2]; // primitive index | primitive's feature's id << IFEAT_LOG2
    int nborderpt;
    int bClosed;
    Vec3 center;
    bool bBorderConsecutive;
    geom_contact_area* parea;
};

#endif // CRYINCLUDE_CRYCOMMON_PRIMITIVES_H
