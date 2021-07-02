/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
/* BEGIN CONTENTS OF README.TXT -------------------------------------
The ConvexDecomposition library was written by John W. Ratcliff mailto:jratcliffscarab@gmail.com

What is Convex Decomposition?

Convex Decomposition is when you take an arbitrarily complex triangle mesh and sub-divide it into
a collection of discrete compound pieces (each represented as a convex hull) to approximate
the original shape of the objet.

This is required since few physics engines can treat aribtrary triangle mesh objects as dynamic
objects.  Even those engines which can handle this use case incurr a huge performance and memory
penalty to do so.

By breaking a complex triangle mesh up into a discrete number of convex components you can greatly
improve performance for dynamic simulations.

--------------------------------------------------------------------------------

This code is released under the MIT license.

The code is functional but could use the following improvements:

(1) The convex hull generator, originally written by Stan Melax, could use some major code cleanup.

(2) The code to remove T-junctions appears to have a bug in it.  This code was working fine before,
but I haven't had time to debug why it stopped working.

(3) Island generation once the mesh has been split is currently disabled due to the fact that the
Remove Tjunctions functionality has a bug in it.

(4) The code to perform a raycast against a triangle mesh does not currently use any acceleration
data structures.

(5) When a split is performed, the surface that got split is not 'capped'.  This causes a problem
if you use a high recursion depth on your convex decomposition.  It will cause the object to
be modelled as if it had a hollow interior.  A lot of work was done to solve this problem, but
it hasn't been integrated into this code drop yet.


*/// ---------- END CONTENTS OF README.TXT ----------------------------


// a set of routines that let you do common 3d math
// operations without any vector, matrix, or quaternion
// classes or templates.
//
// a vector (or point) is a 'NxF32 *' to 3 floating point numbers.
// a matrix is a 'NxF32 *' to an array of 16 floating point numbers representing a 4x4 transformation matrix compatible with D3D or OGL
// a quaternion is a 'NxF32 *' to 4 floats representing a quaternion x,y,z,w
//
//
/*!
**
** Copyright (c) 2009 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
** Portions of this source has been released with the PhysXViewer application, as well as
** Rocket, CreateDynamics, ODF, and as a number of sample code snippets.
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** DeMolay is a youth group for young men between the ages of 12 and 21.
** It teaches strong moral principles, as well as leadership skills and
** public speaking.  The donations page uses the 'pay for pixels' paradigm
** where, in this case, a pixel is only a single penny.  Donations can be
** made for as small as $4 or as high as a $100 block.  Each person who donates
** will get a link to their own site as well as acknowledgement on the
** donations blog located here http://www.amillionpixels.blogspot.com/
**
** If you wish to contact me you can use the following methods:
**
** Skype ID: jratcliff63367
** Yahoo: jratcliff63367
** AOL: jratcliff1961
** email: jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#pragma warning(disable:4996)

class TVec
{
public:
    TVec(NxF64 _x, NxF64 _y, NxF64 _z) { x = _x; y = _y; z = _z; };
    TVec(void) { };

    NxF64 x;
    NxF64 y;
    NxF64 z;
};


class CTriangulator
{
public:
    ///     Default constructor
    CTriangulator();

    ///     Default destructor
    virtual ~CTriangulator();

    ///     Returns the given point in the triangulator array
    inline TVec get(const TU32 id) { return mPoints[id]; }

    virtual void reset(void)
    {
        mInputPoints.clear();
        mPoints.clear();
        mIndices.clear();
    }

    virtual void addPoint(NxF64 x, NxF64 y, NxF64 z)
    {
        TVec v(x, y, z);
        // update bounding box...
        if (mInputPoints.empty())
        {
            mMin = v;
            mMax = v;
        }
        else
        {
            if (x < mMin.x)
            {
                mMin.x = x;
            }
            if (y < mMin.y)
            {
                mMin.y = y;
            }
            if (z < mMin.z)
            {
                mMin.z = z;
            }

            if (x > mMax.x)
            {
                mMax.x = x;
            }
            if (y > mMax.y)
            {
                mMax.y = y;
            }
            if (z > mMax.z)
            {
                mMax.z = z;
            }
        }
        mInputPoints.push_back(v);
    }

    // Triangulation happens in 2d.  We could inverse transform the polygon around the normal direction, or we just use the two most signficant axes
    // Here we find the two longest axes and use them to triangulate.  Inverse transforming them would introduce more doubleing point error and isn't worth it.
    virtual NxU32* triangulate(NxU32& tcount, NxF64 epsilon)
    {
        NxU32* ret = 0;
        tcount = 0;
        mEpsilon = epsilon;

        if (!mInputPoints.empty())
        {
            mPoints.clear();

            NxF64 dx = mMax.x - mMin.x; // locate the first, second and third longest edges and store them in i1, i2, i3
            NxF64 dy = mMax.y - mMin.y;
            NxF64 dz = mMax.z - mMin.z;

            NxU32 i1, i2, i3;

            if (dx > dy && dx > dz)
            {
                i1 = 0;
                if (dy > dz)
                {
                    i2 = 1;
                    i3 = 2;
                }
                else
                {
                    i2 = 2;
                    i3 = 1;
                }
            }
            else if (dy > dx && dy > dz)
            {
                i1 = 1;
                if (dx > dz)
                {
                    i2 = 0;
                    i3 = 2;
                }
                else
                {
                    i2 = 2;
                    i3 = 0;
                }
            }
            else
            {
                i1 = 2;
                if (dx > dy)
                {
                    i2 = 0;
                    i3 = 1;
                }
                else
                {
                    i2 = 1;
                    i3 = 0;
                }
            }

            NxU32 pcount = (NxU32)mInputPoints.size();
            const NxF64* points = &mInputPoints[0].x;
            for (NxU32 i = 0; i < pcount; i++)
            {
                TVec v(points[i1], points[i2], points[i3]);
                mPoints.push_back(v);
                points += 3;
            }

            mIndices.clear();
            triangulate(mIndices);
            tcount = (NxU32)mIndices.size() / 3;
            if (tcount)
            {
                ret = &mIndices[0];
            }
        }
        return ret;
    }

    virtual const NxF64* getPoint(NxU32 index)
    {
        return &mInputPoints[index].x;
    }


private:
    NxF64                  mEpsilon;
    TVec                   mMin;
    TVec                   mMax;
    TVecVector             mInputPoints;
    TVecVector             mPoints;
    TU32Vector             mIndices;

    ///     Tests if a point is inside the given triangle
    bool _insideTriangle(const TVec& A, const TVec& B, const TVec& C, const TVec& P);

    ///     Returns the area of the contour
    NxF64 _area();

    bool _snip(NxI32 u, NxI32 v, NxI32 w, NxI32 n, NxI32* V);

    ///     Processes the triangulation
    void _process(TU32Vector& indices);
    ///     Triangulates the contour
    void triangulate(TU32Vector& indices);
};

///     Default constructor
CTriangulator::CTriangulator(void)
{
}

///     Default destructor
CTriangulator::~CTriangulator()
{
}

///     Triangulates the contour
void CTriangulator::triangulate(TU32Vector& indices)
{
    _process(indices);
}

///     Processes the triangulation
void CTriangulator::_process(TU32Vector& indices)
{
    const NxI32 n = (const NxI32)mPoints.size();
    if (n < 3)
    {
        return;
    }
    NxI32* V = (NxI32*)MEMALLOC_MALLOC(sizeof(NxI32) * n);

    bool flipped = false;

    if (0.0f < _area())
    {
        for (NxI32 v = 0; v < n; v++)
        {
            V[v] = v;
        }
    }
    else
    {
        flipped = true;
        for (NxI32 v = 0; v < n; v++)
        {
            V[v] = (n - 1) - v;
        }
    }

    NxI32 nv = n;
    NxI32 count = 2 * nv;
    for (NxI32 m = 0, v = nv - 1; nv > 2; )
    {
        if (0 >= (count--))
        {
            return;
        }

        NxI32 u = v;
        if (nv <= u)
        {
            u = 0;
        }
        v = u + 1;
        if (nv <= v)
        {
            v = 0;
        }
        NxI32 w = v + 1;
        if (nv <= w)
        {
            w = 0;
        }

        if (_snip(u, v, w, nv, V))
        {
            NxI32 a, b, c, s, t;
            a = V[u];
            b = V[v];
            c = V[w];
            if (flipped)
            {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);
            }
            else
            {
                indices.push_back(c);
                indices.push_back(b);
                indices.push_back(a);
            }
            m++;
            for (s = v, t = v + 1; t < nv; s++, t++)
            {
                V[s] = V[t];
            }
            nv--;
            count = 2 * nv;
        }
    }

    MEMALLOC_FREE(V);
}

///     Returns the area of the contour
NxF64 CTriangulator::_area()
{
    NxI32 n = (NxU32)mPoints.size();
    NxF64 A = 0.0f;
    for (NxI32 p = n - 1, q = 0; q < n; p = q++)
    {
        const TVec& pval = mPoints[p];
        const TVec& qval = mPoints[q];
        A += pval.x * qval.y - qval.x * pval.y;
    }
    A *= 0.5f;
    return A;
}

bool CTriangulator::_snip(NxI32 u, NxI32 v, NxI32 w, NxI32 n, NxI32* V)
{
    NxI32 p;

    const TVec& A = mPoints[V[u]];
    const TVec& B = mPoints[V[v]];
    const TVec& C = mPoints[V[w]];

    if (mEpsilon > (((B.x - A.x) * (C.y - A.y)) - ((B.y - A.y) * (C.x - A.x))))
    {
        return false;
    }

    for (p = 0; p < n; p++)
    {
        if ((p == u) || (p == v) || (p == w))
        {
            continue;
        }
        const TVec& P = mPoints[V[p]];
        if (_insideTriangle(A, B, C, P))
        {
            return false;
        }
    }
    return true;
}

///     Tests if a point is inside the given triangle
bool CTriangulator::_insideTriangle(const TVec& A, const TVec& B, const TVec& C, const TVec& P)
{
    NxF64 ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
    NxF64 cCROSSap, bCROSScp, aCROSSbp;

    ax = C.x - B.x;
    ay = C.y - B.y;
    bx = A.x - C.x;
    by = A.y - C.y;
    cx = B.x - A.x;
    cy = B.y - A.y;
    apx = P.x - A.x;
    apy = P.y - A.y;
    bpx = P.x - B.x;
    bpy = P.y - B.y;
    cpx = P.x - C.x;
    cpy = P.y - C.y;

    aCROSSbp = ax * bpy - ay * bpx;
    cCROSSap = cx * apy - cy * apx;
    bCROSScp = bx * cpy - by * cpx;

    return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
}

