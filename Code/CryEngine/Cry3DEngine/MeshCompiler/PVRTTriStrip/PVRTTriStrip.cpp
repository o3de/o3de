// Modifications copyright Amazon.com, Inc. or its affiliates.

/******************************************************************************

 @File         PVRTTriStrip.cpp

 @Title        PVRTTriStrip

 @Version       @Version

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independent

 @Description  Strips a triangle list.

******************************************************************************/

/****************************************************************************
** Includes
****************************************************************************/
#include <platform.h>
#include <stdlib.h>

#include "PVRTTriStrip.h"

/****************************************************************************
** Defines
****************************************************************************/
#define RND_TRIS_ORDER
#define FREE(X)     { if(X) { free(X); (X) = 0; } }

/****************************************************************************
** Structures
****************************************************************************/

/****************************************************************************
** Class: CTri
****************************************************************************/
class CTri;

/*!***************************************************************************
 @Class             CTriState
 @Description       Stores a pointer to the triangles either side of itself,
                    as well as it's winding.
*****************************************************************************/
class CTriState
{
public:
    CTri    *pRev, *pFwd;
    bool    bWindFwd;

    CTriState()
    {
        bWindFwd    = true;     // Initial value irrelevent
        pRev        = NULL;
        pFwd        = NULL;
    }
};
/*!***************************************************************************
 @Class             CTri
 @Description       Object used to store information about the triangle, such as
                    the vertex indices it is made from, which triangles are
                    adjacent to it, etc.
*****************************************************************************/
class CTri
{
public:
    CTriState   sNew, sOld;

    CTri    *pAdj[3];
    bool    bInStrip;

    const unsigned int  *pIdx;      // three indices for the tri
    bool                    bOutput;

public:
    CTri();
    int FindEdge(const unsigned int pw0, const unsigned int pw1) const;
    void Cement();
    void Undo();
    int EdgeFromAdjTri(const CTri &tri) const;  // Find the index of the adjacent tri
};

/*!***************************************************************************
 @Class             CStrip
 @Description       Object used to store the triangles that a given strip is
                    composed from.
*****************************************************************************/
class CStrip
{
protected:
    unsigned int    m_nTriCnt;
    CTri            *m_pTri;
    unsigned int    m_nStrips;

    CTri            **m_psStrip;    // Working space for finding strips

public:
    CStrip(
        const unsigned int  * const pui32TriList,
        const unsigned int      nTriCnt);
    ~CStrip();

protected:
    bool StripGrow(
        CTri                &triFrom,
        const unsigned int  nEdgeFrom,
        const int           nMaxChange);

public:
    void StripFromEdges();
    void StripImprove();

    void Output(
        unsigned int    **ppui32Strips,
        unsigned int    **ppnStripLen,
        unsigned int    *pnStripCnt);
};

/****************************************************************************
** Constants
****************************************************************************/

/****************************************************************************
** Code: Class: CTri
****************************************************************************/
CTri::CTri()
{
    pAdj[0]     = NULL;
    pAdj[1]     = NULL;
    pAdj[2]     = NULL;
    bInStrip    = false;
    bOutput     = false;
}

/*!***************************************************************************
 @Function          FindEdge
 @Input             pw0         The first index
 @Input             pw1         The second index
 @Return            The index of the edge
 @Description       Finds the index of the edge that the current object shares
                    with the two vertex index values that have been passed in
                    (or returns -1 if they dont share an edge).
*****************************************************************************/
int CTri::FindEdge(const unsigned int pw0, const unsigned int pw1) const
{
    if((pIdx[0] == pw0 && pIdx[1] == pw1))
        return 0;
    if((pIdx[1] == pw0 && pIdx[2] == pw1))
        return 1;
    if((pIdx[2] == pw0 && pIdx[0] == pw1))
        return 2;
    return -1;
}
/*!***************************************************************************
 @Function          Cement
 @Description       Assigns the new state as the old state.
*****************************************************************************/
void CTri::Cement()
{
    sOld = sNew;
}
/*!***************************************************************************
 @Function          Undo
 @Description       Reverts the new state to the old state.
*****************************************************************************/
void CTri::Undo()
{
    sNew = sOld;
}
/*!***************************************************************************
 @Function          EdgeFromAdjTri
 @Input             tri         The triangle to compare
 @Return            int         Index of adjacent triangle (-1 if not adjacent)
 @Description       If the input triangle is adjacent to the current triangle,
                    it's index is returned.
*****************************************************************************/
int CTri::EdgeFromAdjTri(const CTri &tri) const
{
    for(int i = 0; i < 3; ++i)
    {
        if(pAdj[i] == &tri)
        {
            return i;
        }
    }
    assert(false);
    return -1;
}

/****************************************************************************
** Local code
****************************************************************************/
/*!***************************************************************************
 @Function          OrphanTri
 @Input             tri         The triangle test
 @Return            int         Returns 1 if change was made
 @Description       If the input triangle is not wound forward and is not the last
                    triangle in the strip, the connection with the next triangle
                    in the strip is removed.
*****************************************************************************/
static int OrphanTri(
    CTri        * const pTri)
{
    assert(!pTri->bInStrip);
    if(pTri->sNew.bWindFwd || !pTri->sNew.pFwd)
        return 0;

    pTri->sNew.pFwd->sNew.pRev = NULL;
    pTri->sNew.pFwd = NULL;
    return 1;
}
/*!***************************************************************************
 @Function          TakeTri
 @Input             pTri        The triangle to take
 @Input             pRevNew     The triangle that is before pTri in the new strip
 @Return            int         Returns 1 if a new strip has been created
 @Description       Removes the triangle from it's current strip
                    and places it in a new one (following pRevNew in the new strip).
*****************************************************************************/
static int TakeTri(
    CTri        * const pTri,
    CTri        * const pRevNew,
    const bool  bFwd)
{
    int nRet;

    assert(!pTri->bInStrip);

    if(pTri->sNew.pFwd && pTri->sNew.pRev)
    {
        assert(pTri->sNew.pFwd->sNew.pRev == pTri);
        pTri->sNew.pFwd->sNew.pRev = NULL;
        assert(pTri->sNew.pRev->sNew.pFwd == pTri);
        pTri->sNew.pRev->sNew.pFwd = NULL;

        // If in the middle of a Strip, this will generate a new Strip
        nRet = 1;

        // The second tri in the strip may need to be orphaned, or it will have wrong winding order
        nRet += OrphanTri(pTri->sNew.pFwd);
    }
    else if(pTri->sNew.pFwd)
    {
        assert(pTri->sNew.pFwd->sNew.pRev == pTri);
        pTri->sNew.pFwd->sNew.pRev = NULL;

        // If at the beginning of a Strip, no change
        nRet = 0;

        // The second tri in the strip may need to be orphaned, or it will have wrong winding order
        nRet += OrphanTri(pTri->sNew.pFwd);
    }
    else if(pTri->sNew.pRev)
    {
        assert(pTri->sNew.pRev->sNew.pFwd == pTri);
        pTri->sNew.pRev->sNew.pFwd = NULL;

        // If at the end of a Strip, no change
        nRet = 0;
    }
    else
    {
        // Otherwise it's a lonesome triangle; one Strip removed!
        nRet = -1;
    }

    pTri->sNew.pFwd     = NULL;
    pTri->sNew.pRev     = pRevNew;
    pTri->bInStrip      = true;
    pTri->sNew.bWindFwd = bFwd;

    if(pRevNew)
    {
        assert(!pRevNew->sNew.pFwd);
        pRevNew->sNew.pFwd  = pTri;
    }

    return nRet;
}
/*!***************************************************************************
 @Function          TryLinkEdge
 @Input             src         The source triangle
 @Input             cmp         The triangle to compare with
 @Input             nSrcEdge    The edge of souce triangle to compare
 @Input             idx0        Vertex index 0 of the compare triangle
 @Input             idx1        Vertex index 1 of the compare triangle
 @Description       If the triangle to compare currently has no adjacent
                    triangle along the specified edge, link the source triangle
                    (along it's specified edge) with the compare triangle.
*****************************************************************************/
static bool TryLinkEdge(
    CTri                &src,
    CTri                &cmp,
    const int           nSrcEdge,
    const unsigned int  idx0,
    const unsigned int  idx1)
{
    int nCmpEdge;

    nCmpEdge = cmp.FindEdge(idx0, idx1);
    if(nCmpEdge != -1 && !cmp.pAdj[nCmpEdge])
    {
        cmp.pAdj[nCmpEdge] = &src;
        src.pAdj[nSrcEdge] = &cmp;
        return true;
    }
    return false;
}

/****************************************************************************
** Code: Class: CStrip
****************************************************************************/
CStrip::CStrip(
    const unsigned int  * const pui32TriList,
    const unsigned int  nTriCnt)
{
    unsigned int    i, j;
    bool            b0, b1, b2;

    m_nTriCnt = nTriCnt;

    /*
        Generate adjacency info
    */
    m_pTri = new CTri[nTriCnt];
    for(i = 0; i < nTriCnt; ++i)
    {
        // Set pointer to indices
        m_pTri[i].pIdx = &pui32TriList[3 * i];

        b0 = false;
        b1 = false;
        b2 = false;
        for(j = 0; j < i && !(b0 & b1 & b2); ++j)
        {
            if(!b0)
                b0 = TryLinkEdge(m_pTri[i], m_pTri[j], 0, m_pTri[i].pIdx[1], m_pTri[i].pIdx[0]);

            if(!b1)
                b1 = TryLinkEdge(m_pTri[i], m_pTri[j], 1, m_pTri[i].pIdx[2], m_pTri[i].pIdx[1]);

            if(!b2)
                b2 = TryLinkEdge(m_pTri[i], m_pTri[j], 2, m_pTri[i].pIdx[0], m_pTri[i].pIdx[2]);
        }
    }

    // Initially, every triangle is a strip.
    m_nStrips = m_nTriCnt;

    // Allocate working space for the strippers
    m_psStrip = new CTri*[m_nTriCnt];
}

CStrip::~CStrip()
{
    delete [] m_pTri;
    delete [] m_psStrip;
}
/*!***************************************************************************
 @Function          StripGrow
 @Input             triFrom         The triangle to begin from
 @Input             nEdgeFrom       The edge of the triangle to begin from
 @Input             maxChange       The maximum number of changes to be made
 @Description       Takes triFrom as a starting point of triangles to add to
                    the list and adds triangles sequentially by finding the next
                    triangle that is adjacent to the current triangle.
                    This is repeated until the maximum number of changes
                    have been made.
*****************************************************************************/
bool CStrip::StripGrow(
    CTri                &triFrom,
    const unsigned int  nEdgeFrom,
    const int           nMaxChange)
{
    unsigned int    i;
    bool            bFwd;
    int             nDiff, nDiffTot, nEdge;
    CTri            *pTri, *pTriPrev, *pTmp;
    unsigned int    nStripLen;

    // Start strip from this tri
    pTri        = &triFrom;
    pTriPrev    = NULL;

    nDiffTot    = 0;
    nStripLen   = 0;

    // Start strip from this edge
    nEdge   = nEdgeFrom;
    bFwd    = true;

    // Extend the strip until we run out, or we find an improvement
    nDiff = 1;
    while(nDiff > nMaxChange)
    {
        // Add pTri to the strip
        assert(pTri);
        nDiff += TakeTri(pTri, pTriPrev, bFwd);
        assert(nStripLen < m_nTriCnt);
        m_psStrip[nStripLen++] = pTri;

        // Jump to next tri
        pTriPrev = pTri;
        pTri = pTri->pAdj[nEdge];
        if(!pTri)
            break;  // No more tris, gotta stop

        if(pTri->bInStrip)
            break;  // No more tris, gotta stop

        // Find which edge we came over
        nEdge = pTri->EdgeFromAdjTri(*pTriPrev);

        // Find the edge to leave over
        if(bFwd)
        {
            if(--nEdge < 0)
                nEdge = 2;
        }
        else
        {
            if(++nEdge > 2)
                nEdge = 0;
        }

        // Swap the winding order for the next tri
        bFwd = !bFwd;
    }
    assert(!pTriPrev->sNew.pFwd);

    /*
        Accept or reject this strip.

        Accepting changes which don't change the number of strips
        adds variety, which can help better strips to develop.
    */
    if(nDiff <= nMaxChange)
    {
        nDiffTot += nDiff;

        // Great, take the Strip
        for(i = 0; i < nStripLen; ++i)
        {
            pTri = m_psStrip[i];
            assert(pTri->bInStrip);

            // Cement affected tris
            pTmp = pTri->sOld.pFwd;
            if(pTmp && !pTmp->bInStrip)
            {
                if(pTmp->sOld.pFwd && !pTmp->sOld.pFwd->bInStrip)
                    pTmp->sOld.pFwd->Cement();
                pTmp->Cement();
            }

            pTmp = pTri->sOld.pRev;
            if(pTmp && !pTmp->bInStrip)
            {
                pTmp->Cement();
            }

            // Cement this tris
            pTri->bInStrip = false;
            pTri->Cement();
        }
    }
    else
    {
        // Shame, undo the strip
        for(i = 0; i < nStripLen; ++i)
        {
            pTri = m_psStrip[i];
            assert(pTri->bInStrip);

            // Undo affected tris
            pTmp = pTri->sOld.pFwd;
            if(pTmp && !pTmp->bInStrip)
            {
                if(pTmp->sOld.pFwd && !pTmp->sOld.pFwd->bInStrip)
                    pTmp->sOld.pFwd->Undo();
                pTmp->Undo();
            }

            pTmp = pTri->sOld.pRev;
            if(pTmp && !pTmp->bInStrip)
            {
                pTmp->Undo();
            }

            // Undo this tris
            pTri->bInStrip = false;
            pTri->Undo();
        }
    }

#ifdef _DEBUG
    for(int nDbg = 0; nDbg < (int)m_nTriCnt; ++nDbg)
    {
        assert(m_pTri[nDbg].bInStrip == false);
        assert(m_pTri[nDbg].bOutput == false);
        assert(m_pTri[nDbg].sOld.pRev == m_pTri[nDbg].sNew.pRev);
        assert(m_pTri[nDbg].sOld.pFwd == m_pTri[nDbg].sNew.pFwd);

        if(m_pTri[nDbg].sNew.pRev)
        {
            assert(m_pTri[nDbg].sNew.pRev->sNew.pFwd == &m_pTri[nDbg]);
        }

        if(m_pTri[nDbg].sNew.pFwd)
        {
            assert(m_pTri[nDbg].sNew.pFwd->sNew.pRev == &m_pTri[nDbg]);
        }
    }
#endif

    if(nDiffTot)
    {
        m_nStrips += nDiffTot;
        return true;
    }
    return false;
}

/*!***************************************************************************
 @Function          StripFromEdges
 @Description       Creates a strip from the object's edge information.
*****************************************************************************/
void CStrip::StripFromEdges()
{
    unsigned int    i, j, nTest;
    CTri            *pTri, *pTriPrev;
    int             nEdge = 0;

    /*
        Attempt to create grid-oriented strips.
    */
    for(i = 0; i < m_nTriCnt; ++i)
    {
        pTri = &m_pTri[i];

        // Count the number of empty edges
        nTest = 0;
        for(j = 0; j < 3; ++j)
        {
            if(!pTri->pAdj[j])
            {
                ++nTest;
            }
            else
            {
                nEdge = j;
            }
        }

        if(nTest != 2)
            continue;

        for(;;)
        {
            // A tri with two empty edges is a corner (there are other corners too, but this works so...)
            while(StripGrow(*pTri, nEdge, -1)) {};

            pTriPrev = pTri;
            pTri = pTri->pAdj[nEdge];
            if(!pTri)
                break;

            // Find the edge we came over
            nEdge = pTri->EdgeFromAdjTri(*pTriPrev);

            // Step around to the next edge
            if(++nEdge > 2)
                nEdge = 0;

            pTriPrev = pTri;
            pTri = pTri->pAdj[nEdge];
            if(!pTri)
                break;

            // Find the edge we came over
            nEdge = pTri->EdgeFromAdjTri(*pTriPrev);

            // Step around to the next edge
            if(--nEdge < 0)
                nEdge = 2;

#if 0
            // If we're not tracking the edge, give up
            nTest = nEdge - 1;
            if(nTest < 0)
                nTest = 2;
            if(pTri->pAdj[nTest])
                break;
            else
                continue;
#endif
        }
    }
}

#ifdef RND_TRIS_ORDER
struct pair
{
    unsigned int i, o;
};

static int compare(const void *arg1, const void *arg2)
{
    return ((pair*)arg1)->i - ((pair*)arg2)->i;
}
#endif
/*!***************************************************************************
 @Function          StripImprove
 @Description       Optimises the strip
*****************************************************************************/
void CStrip::StripImprove()
{
    unsigned int    i, j;
    bool            bChanged;
    int             nRepCnt, nChecks;
    int             nMaxChange;
#ifdef RND_TRIS_ORDER
    pair            *pnOrder;

    /*
        Create a random order to process the tris
    */
    pnOrder = new pair[m_nTriCnt];
#endif

    nRepCnt = 0;
    nChecks = 2;
    nMaxChange  = 0;

    /*
        Reduce strip count by growing each of the three strips each tri can start.
    */
    while(nChecks)
    {
        --nChecks;

        bChanged = false;

#ifdef RND_TRIS_ORDER
        /*
            Create a random order to process the tris
        */
        for(i = 0; i < m_nTriCnt; ++i)
        {
            pnOrder[i].i = rand() * rand();
            pnOrder[i].o = i;
        }
        qsort(pnOrder, m_nTriCnt, sizeof(*pnOrder), compare);
#endif

        /*
            Process the tris
        */
        for(i = 0; i < m_nTriCnt; ++i)
        {
            for(j = 0; j < 3; ++j)
            {
#ifdef RND_TRIS_ORDER
                bChanged |= StripGrow(m_pTri[pnOrder[i].o], j, nMaxChange);
#else
                bChanged |= StripGrow(m_pTri[i], j, nMaxChange);
#endif
            }
        }
        ++nRepCnt;

        // Check the results once or twice
        if(bChanged)
            nChecks = 2;

        nMaxChange = (nMaxChange == 0 ? -1 : 0);
    }

#ifdef RND_TRIS_ORDER
    delete [] pnOrder;
#endif
    //_RPT1(_CRT_WARN, "Reps: %d\n", nRepCnt);
}
/*!***************************************************************************
 @Function          Output
 @Output            ppui32Strips
 @Output            ppnStripLen         The length of the strip
 @Output            pnStripCnt
 @Description       Outputs key information about the strip to the output
                    parameters.
*****************************************************************************/
void CStrip::Output(
    unsigned int    **ppui32Strips,
    unsigned int    **ppnStripLen,
    unsigned int    *pnStripCnt)
{
    unsigned int    *pui32Strips;
    unsigned int    *pnStripLen;
    unsigned int    i, j, nIdx, nStrip;
    CTri            *pTri;

    /*
        Output Strips
    */
    pnStripLen = (unsigned int*)malloc(m_nStrips * sizeof(*pnStripLen));
    pui32Strips = (unsigned int*)malloc((m_nTriCnt + m_nStrips * 2) * sizeof(*pui32Strips));
    nStrip = 0;
    nIdx = 0;
    for(i = 0; i < m_nTriCnt; ++i)
    {
        pTri = &m_pTri[i];

        if(pTri->sNew.pRev)
            continue;
        assert(!pTri->sNew.pFwd || pTri->sNew.bWindFwd);
        assert(pTri->bOutput == false);

        if(!pTri->sNew.pFwd)
        {
            pui32Strips[nIdx++] = pTri->pIdx[0];
            pui32Strips[nIdx++] = pTri->pIdx[1];
            pui32Strips[nIdx++] = pTri->pIdx[2];
            pnStripLen[nStrip] = 1;
            pTri->bOutput = true;
        }
        else
        {
            if(pTri->sNew.pFwd == pTri->pAdj[0])
            {
                pui32Strips[nIdx++] = pTri->pIdx[2];
                pui32Strips[nIdx++] = pTri->pIdx[0];
            }
            else if(pTri->sNew.pFwd == pTri->pAdj[1])
            {
                pui32Strips[nIdx++] = pTri->pIdx[0];
                pui32Strips[nIdx++] = pTri->pIdx[1];
            }
            else
            {
                assert(pTri->sNew.pFwd == pTri->pAdj[2]);
                pui32Strips[nIdx++] = pTri->pIdx[1];
                pui32Strips[nIdx++] = pTri->pIdx[2];
            }

            pnStripLen[nStrip] = 0;
            do
            {
                assert(pTri->bOutput == false);

                // Increment tris-in-this-strip counter
                ++pnStripLen[nStrip];

                // Output the new vertex index
                for(j = 0; j < 3; ++j)
                {
                    if(
                        (pui32Strips[nIdx-2] != pTri->pIdx[j]) &&
                        (pui32Strips[nIdx-1] != pTri->pIdx[j]))
                    {
                        break;
                    }
                }
                assert(j != 3);
                pui32Strips[nIdx++] = pTri->pIdx[j];

                // Double-check that the previous three indices are the indices of this tris (in some order)
                assert(
                    ((pui32Strips[nIdx-3] == pTri->pIdx[0]) && (pui32Strips[nIdx-2] == pTri->pIdx[1]) && (pui32Strips[nIdx-1] == pTri->pIdx[2])) ||
                    ((pui32Strips[nIdx-3] == pTri->pIdx[1]) && (pui32Strips[nIdx-2] == pTri->pIdx[2]) && (pui32Strips[nIdx-1] == pTri->pIdx[0])) ||
                    ((pui32Strips[nIdx-3] == pTri->pIdx[2]) && (pui32Strips[nIdx-2] == pTri->pIdx[0]) && (pui32Strips[nIdx-1] == pTri->pIdx[1])) ||
                    ((pui32Strips[nIdx-3] == pTri->pIdx[2]) && (pui32Strips[nIdx-2] == pTri->pIdx[1]) && (pui32Strips[nIdx-1] == pTri->pIdx[0])) ||
                    ((pui32Strips[nIdx-3] == pTri->pIdx[1]) && (pui32Strips[nIdx-2] == pTri->pIdx[0]) && (pui32Strips[nIdx-1] == pTri->pIdx[2])) ||
                    ((pui32Strips[nIdx-3] == pTri->pIdx[0]) && (pui32Strips[nIdx-2] == pTri->pIdx[2]) && (pui32Strips[nIdx-1] == pTri->pIdx[1])));

                // Check that the latest three indices are not degenerate
                assert(pui32Strips[nIdx-1] != pui32Strips[nIdx-2]);
                assert(pui32Strips[nIdx-1] != pui32Strips[nIdx-3]);
                assert(pui32Strips[nIdx-2] != pui32Strips[nIdx-3]);

                pTri->bOutput = true;

                // Check that the next triangle is adjacent to this triangle
                assert(
                    (pTri->sNew.pFwd == pTri->pAdj[0]) ||
                    (pTri->sNew.pFwd == pTri->pAdj[1]) ||
                    (pTri->sNew.pFwd == pTri->pAdj[2]) ||
                    (!pTri->sNew.pFwd));
                // Check that this triangle is adjacent to the next triangle
                assert(
                    (!pTri->sNew.pFwd) ||
                    (pTri == pTri->sNew.pFwd->pAdj[0]) ||
                    (pTri == pTri->sNew.pFwd->pAdj[1]) ||
                    (pTri == pTri->sNew.pFwd->pAdj[2]));

                pTri = pTri->sNew.pFwd;
            } while(pTri);
        }

        ++nStrip;
    }
    assert(nIdx == m_nTriCnt + m_nStrips * 2);
    assert(nStrip == m_nStrips);

    // Check all triangles have been output
    for(i = 0; i < m_nTriCnt; ++i)
    {
        assert(m_pTri[i].bOutput == true);
    }

    // Check all triangles are present
    j = 0;
    for(i = 0; i < m_nStrips; ++i)
    {
        j += pnStripLen[i];
    }
    assert(j == m_nTriCnt);

    // Output data
    *pnStripCnt     = m_nStrips;
    *ppui32Strips       = pui32Strips;
    *ppnStripLen    = pnStripLen;
}

/****************************************************************************
** Code
****************************************************************************/

/*!***************************************************************************
 @Function          PVRTTriStrip
 @Output            ppui32Strips
 @Output            ppnStripLen
 @Output            pnStripCnt
 @Input             pui32TriList
 @Input             nTriCnt
 @Description       Reads a triangle list and generates an optimised triangle strip.
*****************************************************************************/
void PVRTTriStrip(
    unsigned int            **ppui32Strips,
    unsigned int            **ppnStripLen,
    unsigned int            *pnStripCnt,
    const unsigned int  * const pui32TriList,
    const unsigned int      nTriCnt)
{
    unsigned int    *pui32Strips;
    unsigned int    *pnStripLen;
    unsigned int    nStripCnt;

    /*
        If the order in which triangles are tested as strip roots is
        randomised, then several attempts can be made. Use the best result.
    */
    for(int i = 0; i <
#ifdef RND_TRIS_ORDER
        5
#else
        1
#endif
        ; ++i)
    {
        CStrip stripper(pui32TriList, nTriCnt);

#ifdef RND_TRIS_ORDER
        srand(i);
#endif

        stripper.StripFromEdges();
        stripper.StripImprove();
        stripper.Output(&pui32Strips, &pnStripLen, &nStripCnt);

        if(!i || nStripCnt < *pnStripCnt)
        {
            if(i)
            {
                FREE(*ppui32Strips);
                FREE(*ppnStripLen);
            }

            *ppui32Strips       = pui32Strips;
            *ppnStripLen    = pnStripLen;
            *pnStripCnt     = nStripCnt;
        }
        else
        {
            FREE(pui32Strips);
            FREE(pnStripLen);
        }
    }
}

/*!***************************************************************************
 @Function          PVRTTriStripList
 @Modified          pui32TriList
 @Input             nTriCnt
 @Description       Reads a triangle list and generates an optimised triangle strip.
                    Result is converted back to a triangle list.
*****************************************************************************/
void PVRTTriStripList(unsigned int * const pui32TriList, const unsigned int nTriCnt)
{
    unsigned int    *pui32Strips;
    unsigned int    *pnStripLength;
    unsigned int    nNumStrips;
    unsigned int    *pui32TriPtr, *pui32StripPtr;

    /*
        Strip the geometry
    */
    PVRTTriStrip(&pui32Strips, &pnStripLength, &nNumStrips, pui32TriList, nTriCnt);

    /*
        Convert back to a triangle list
    */
    pui32StripPtr   = pui32Strips;
    pui32TriPtr = pui32TriList;
    for(unsigned int i = 0; i < nNumStrips; ++i)
    {
        *pui32TriPtr++ = *pui32StripPtr++;
        *pui32TriPtr++ = *pui32StripPtr++;
        *pui32TriPtr++ = *pui32StripPtr++;

        for(unsigned int j = 1; j < pnStripLength[i]; ++j)
        {
            // Use two indices from previous triangle, flipping tri order alternately.
            if(j & 0x01)
            {
                *pui32TriPtr++ = pui32StripPtr[-1];
                *pui32TriPtr++ = pui32StripPtr[-2];
            }
            else
            {
                *pui32TriPtr++ = pui32StripPtr[-2];
                *pui32TriPtr++ = pui32StripPtr[-1];
            }

            *pui32TriPtr++ = *pui32StripPtr++;
        }
    }

    free(pui32Strips);
    free(pnStripLength);
}

/*****************************************************************************
 End of file (PVRTTriStrip.cpp)
*****************************************************************************/

