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

#include <Cry3DEngine/Cry_LegacyPhysUtils.h>

namespace LegacyCryPhysicsUtils
{
    namespace qhull_IMPL
    {
        static int __qhullcalled = 0;

        int qhull(strided_pointer<Vec3> _pts, int npts, index_t*& pTris, qhullmalloc qmalloc)
        {
#if defined(PLATFORM_64BIT)
            static ptitem ptbuf[4096];
            static qhtritem trbuf[4096];
            static qhtritem* tmparr_ptr_buf[2048];
            static int tmparr_idx_buf[2048];
#else
            static ptitem ptbuf[1024];
            static qhtritem trbuf[1024];
            static qhtritem* tmparr_ptr_buf[512];
            static int tmparr_idx_buf[512];
#endif
            static volatile int g_lockQhull;
            int iter = 0, maxiter = 0;
            __qhullcalled++;
            strided_pointer<Vec3mem> pts = strided_pointer<Vec3mem>((Vec3mem*)_pts.data, _pts.iStride);

            ptitem* pt, * ptmax, * ptdeleted, * ptlist = npts > sizeof(ptbuf) / sizeof(ptbuf[0]) ? new ptitem[npts] : ptbuf;
            qhtritem* tr, * trnext, * trend, * trnew, * trdata = trbuf, * trstart = 0, * trlast, * trbest;
            int i, j, k, ti, trdatasz = sizeof(trbuf) / sizeof(trbuf[0]), bidx[6], n, next_iter, delbuds;
            qhtritem** tmparr_ptr = tmparr_ptr_buf;
            int* tmparr_idx = tmparr_idx_buf, tmparr_sz = 512;
            float dist, maxdist /*,e*/;
            Vec3 pmin(VMAX), pmax(VMIN);
            WriteLock lock(g_lockQhull);

            // select points for initial tetrahedron
            // first, find 6 points corresponding to min and max coordinates
            for (i = 1; i < npts; i++)
            {
                if (pts[i].x > pmax.x)
                {
                    pmax.x = pts[i].x;
                    bidx[0] = i;
                }
                if (pts[i].x < pmin.x)
                {
                    pmin.x = pts[i].x;
                    bidx[1] = i;
                }
                if (pts[i].y > pmax.y)
                {
                    pmax.y = pts[i].y;
                    bidx[2] = i;
                }
                if (pts[i].y < pmin.y)
                {
                    pmin.y = pts[i].y;
                    bidx[3] = i;
                }
                if (pts[i].z > pmax.z)
                {
                    pmax.z = pts[i].z;
                    bidx[4] = i;
                }
                if (pts[i].z < pmin.z)
                {
                    pmin.z = pts[i].z;
                    bidx[5] = i;
                }
            }
            //  e = max(max(pmax.x-pmin.x,pmax.y-pmin.y),pmax.z-pmin.z)*0.01f;

            for (bidx[0] = 0, i = 1; i < npts; i++)
            {
                bidx[0] += i - bidx[0] & -isneg(pts[i].x - pts[bidx[0]].x);
            }
            for (bidx[1] = 0, i = 1; i < npts; i++)
            {
                bidx[1] += i - bidx[1] & -isneg((pts[bidx[1]] - pts[bidx[0]]).len2() - (pts[i] - pts[bidx[0]]).len2());
            }
            Vec3 norm = pts[bidx[1]] - pts[bidx[0]];
            for (bidx[2] = 0, i = 1; i < npts; i++)
            {
                bidx[2] += i - bidx[2] & -isneg((norm ^ pts[bidx[2]] - pts[bidx[0]]).len2() - (norm ^ pts[i] - pts[bidx[0]]).len2());
            }
            norm = pts[bidx[1]] - pts[bidx[0]] ^ pts[bidx[2]] - pts[bidx[0]];
            for (bidx[3] = 0, i = 1; i < npts; i++)
            {
                bidx[3] += i - bidx[3] & -isneg(fabs_tpl((pts[bidx[3]] - pts[bidx[0]]) * norm) - fabs_tpl((pts[i] - pts[bidx[0]]) * norm));
            }
            if ((pts[bidx[3]] - pts[bidx[0]]) * norm > 0)
            {
                i = bidx[1];
                bidx[1] = bidx[2];
                bidx[2] = i;
            }

            // build a double linked list from all points
            for (i = 0; i < npts; i++)
            {
                ptlist[i].prev = ptlist + i - 1;
                ptlist[i].next = ptlist + i + 1;
            }
            ptlist[0].prev = ptlist + npts - 1;
            ptlist[npts - 1].next = ptlist;
            // remove selected points from the list
            for (i = 0; i < 4; i++)
            {
                delete_item_from_list(ptlist + bidx[i]);
            }
            // assign 3 points to each of 4 initial triangles
            for (i = 0; i < 4; i++)
            {
                for (j = k = 0; j < 4; j++)
                {
                    if (j != i)
                    {
                        trbuf[i].idx[k++] = bidx[j];     // skip i.th point in i.th triangle
                    }
                }
                trbuf[i].n = pts[trbuf[i].idx[1]] - pts[trbuf[i].idx[0]] ^ pts[trbuf[i].idx[2]] - pts[trbuf[i].idx[0]];
                trbuf[i].pt0 = pts[trbuf[i].idx[0]];
                if (e_cansee(pts[bidx[i]] - trbuf[i].pt0, trbuf[i].n, 0)) // flip the orientation so that ccw normal points outwards
                {
                    ti = trbuf[i].idx[0];
                    trbuf[i].idx[0] = trbuf[i].idx[2];
                    trbuf[i].idx[2] = ti;
                    trbuf[i].n = -trbuf[i].n;
                }
                trbuf[i].ptassoc = 0;
                trbuf[i].deleted = 0;
                add_item_to_list(trstart, trbuf + i);
            }
            // fill buddy links for each triangle
            for (i = 0; i < 4; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    if (j != i)
                    {
                        for (k = 0; k < 3; k++)
                        {
                            for (ti = 0; ti < 3; ti++)
                            {
                                if (trbuf[i].idx[k] == trbuf[j].idx[ti] && trbuf[i].idx[k == 2 ? 0 : k + 1] == trbuf[j].idx[ti == 0 ? 2 : ti - 1])
                                {
                                    trbuf[i].buddy[k] = trbuf + j;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            trend = trstart + 4;
            for (i = 0; i < 4; i++)
            {
                if (trbuf[i].n.len2() < 1E-6f)
                {
#ifdef _DEBUG
                    //OutputDebugString("WARNING: convex hull not computed because of degenerate initial triangles\n");
#endif
                    n = 0;
                    goto endqhull;  // some degenerate case, don't party with it
                }
            }
            // associate points with one of the initial triangles
            for (i = 0; i < npts; i++)
            {
                if (ptlist[i].next)
                {
                    break;
                }
            }
            associate_ptlist_with_trilist(ptlist + i, trstart, ptlist, pts);

#define DELETE_TRI(ptri) {                                \
        merge_lists(ptdeleted, (ptri)->ptassoc);          \
        if ((ptri) == trstart) {trstart = (ptri)->next; } \
        if ((ptri) == trnext) {trnext = (ptri)->next; }   \
        delete_item_from_list(ptri); (ptri)->deleted = 1; }


            // main loop
            iter = 0;
            maxiter = npts * npts * 2;
            ptmax = trstart->ptassoc;
            tr = trstart;
            do
            {
                trnext = tr->next;
                pt = tr->ptassoc;
                if (pt)
                {
                    // find the fartherst of the associated with the triangle points
                    maxdist = -1E37f;
                    do
                    {
                        if ((dist = pts[(int)(pt - ptlist)] * tr->n) > maxdist)
                        {
                            maxdist = dist;
                            ptmax = pt;
                        }
                        pt = pt->next;
                    } while (pt != tr->ptassoc);
                    ptdeleted = 0;
                    if (tr->ptassoc == ptmax)
                    {
                        tr->ptassoc = ptmax->next;
                    }
                    delete_item_from_list(ptmax);
                    if (tr->ptassoc == ptmax)
                    {
                        tr->ptassoc = 0;
                    }

                    // find the triangle that the point can see "most confidently"
                    tr = trstart;
                    trlast = tr->prev;
                    ti = static_cast<int>(ptmax - ptlist);
                    maxdist = -1E37f;
                    do
                    {
                        trnext = tr->next;
                        if ((pts[ti] - tr->pt0) * tr->n > maxdist)
                        {
                            maxdist = (pts[ti] - tr->pt0) * tr->n;
                            trbest = tr;
                        }
                        if (tr == trlast)
                        {
                            break;
                        }
                        tr = trnext;
                    } while (true);

                    // "flood fill" triangles that the point can see around that one
                    DELETE_TRI(trbest)
                        tr = trbest->next = trbest->prev = trbest;
                    do
                    {
                        if (tr->buddy[0] && !tr->buddy[0]->deleted && e_cansee(pts[ti] - tr->buddy[0]->pt0, tr->buddy[0]->n, 0))
                        {
                            DELETE_TRI(tr->buddy[0])
                                add_item_to_list(tr, tr->buddy[0]);
                        }
                        if (tr->buddy[1] && !tr->buddy[1]->deleted && e_cansee(pts[ti] - tr->buddy[1]->pt0, tr->buddy[1]->n, 0))
                        {
                            DELETE_TRI(tr->buddy[1])
                                add_item_to_list(tr, tr->buddy[1]);
                        }
                        if (tr->buddy[2] && !tr->buddy[2]->deleted && e_cansee(pts[ti] - tr->buddy[2]->pt0, tr->buddy[2]->n, 0))
                        {
                            DELETE_TRI(tr->buddy[2])
                                add_item_to_list(tr, tr->buddy[2]);
                        }
                        tr = tr->next;
                    } while (tr != trbest);

                    // delete near-visible triangles around deleted area edges to preserve hole convexity
                    // do as many iterations as needed
                    do
                    {
                        tr = trstart;
                        trlast = tr->prev;
                        next_iter = 0;
                        do
                        {
                            trnext = tr->next;
                            if (e_cansee(pts[ti] - tr->pt0, tr->n, -0.001f))
                            {
                                delbuds = tr->buddy[0]->deleted + tr->buddy[1]->deleted + tr->buddy[2]->deleted;
                                if (delbuds >= 2)   // delete triangles that have 2+ buddies deleted
                                {
                                    if (tr == trlast)
                                    {
                                        trlast = tr->next;
                                    }
                                    DELETE_TRI(tr);
                                    next_iter = 1;
                                }
                                else if (delbuds == 1)   // follow triangle fan around both shared edge ends
                                {
                                    int bi, bi0, bi1, nfantris, fandir;
                                    qhtritem* fantris[64], * tr1;

                                    for (bi0 = 0; bi0 < 3 && !tr->buddy[bi0]->deleted; bi0++)
                                    {
                                        ;                                                 // bi0 - deleted buddy index
                                    }
                                    for (fandir = -1; fandir <= 1; fandir += 2) // follow fans in 2 possible directions
                                    {
                                        tr1 = tr;
                                        bi1 = bi0;
                                        nfantris = 0;
                                        do
                                        {
                                            if (nfantris == 64)
                                            {
                                                break;
                                            }
                                            bi = bi1 + fandir;
                                            if (bi > 2)
                                            {
                                                bi -= 3;
                                            }
                                            if (bi < 0)
                                            {
                                                bi += 3;
                                            }
                                            for (bi1 = 0; bi1 < 3 && tr1->buddy[bi]->buddy[bi1] != tr1; bi1++)
                                            {
                                                ;
                                            }
                                            fantris[nfantris++] = tr1; // store this triangle in a temporary fan list
                                            tr1 = tr1->buddy[bi];
                                            bi = bi1;                   // go to the next fan triangle
                                            if (!e_cansee(pts[ti] - tr1->pt0, tr1->n, -0.002f))
                                            {
                                                break; // discard this fan
                                            }
                                            if (tr1->deleted)
                                            {
                                                if (tr1 != tr->buddy[bi0])
                                                {
                                                    // delete fan only if it ended on _another_ deleted triangle
                                                    for (--nfantris; nfantris >= 0; nfantris--)
                                                    {
                                                        if (fantris[nfantris] == trlast)
                                                        {
                                                            trlast = fantris[nfantris]->next;
                                                        }
                                                        DELETE_TRI(fantris[nfantris])
                                                    }
                                                    next_iter = 1;
                                                }
                                                break;     // fan end
                                            }
                                        } while (true);
                                    }
                                }
                            }
                            if (tr == trlast)
                            {
                                break;
                            }
                            tr = trnext;
                        } while (tr);
                    } while (next_iter && trstart);

                    if (!trstart || trstart->deleted)
                    {
                        n = 0;
                        goto endqhull;
                    }

                    // find triangles that shared an edge with deleted triangles
                    trnew = 0;
                    tr = trstart;
                    do
                    {
                        for (i = 0; i < 3; i++)
                        {
                            if (tr->buddy[i]->deleted)
                            {
                                // create a new triangle
                                if (trend >= trdata + trdatasz)
                                {
                                    qhtritem* trdata_new = new qhtritem[trdatasz += 256];
                                    memcpy(trdata_new, trdata, (trend - trdata) * sizeof(qhtritem));
                                    intptr_t diff = (intptr_t)trdata_new - (intptr_t)trdata;
                                    for (n = 0; n < trdatasz - 256; n++)
                                    {
                                        relocate_tritem(trdata_new + n, diff);
                                    }
                                    relocate_ptritem(trend, diff);
                                    relocate_ptritem(trstart, diff);
                                    relocate_ptritem(trnext, diff);
                                    relocate_ptritem(tr, diff);
                                    relocate_ptritem(trbest, diff);
                                    relocate_ptritem(trnew, diff);
                                    if (trdata != trbuf)
                                    {
                                        delete[] trdata;
                                    }
                                    trdata = trdata_new;
                                }
                                trend->idx[0] = static_cast<int>(ptmax - ptlist);
                                trend->idx[1] = tr->idx[i == 2 ? 0 : i + 1];
                                trend->idx[2] = tr->idx[i];
                                trend->ptassoc = 0;
                                trend->deleted = 0;
                                trend->n = pts[trend->idx[1]] - pts[trend->idx[0]] ^ pts[trend->idx[2]] - pts[trend->idx[0]];
                                trend->pt0 = pts[trend->idx[0]];
                                trend->buddy[1] = tr;
                                tr->buddy[i] = trend;
                                trend->buddy[0] = trend->buddy[2] = 0;
                                add_item_to_list(trnew, trend++);
                            }
                        }
                        tr = tr->next;
                    } while (tr != trstart);

                    // sort pointers to the new triangles by their 2nd vertex index
                    n = static_cast<int>(trend - trnew);
                    if (tmparr_sz < n)
                    {
                        if (tmparr_idx != tmparr_idx_buf)
                        {
                            delete[] tmparr_idx;
                        }
                        if (tmparr_ptr != tmparr_ptr_buf)
                        {
                            delete[] tmparr_ptr;
                        }
                        tmparr_idx = new int[n];
                        tmparr_ptr = new qhtritem * [n];
                    }
                    for (tr = trnew, i = 0; tr < trend; tr++, i++)
                    {
                        tmparr_idx[i] = tr->idx[2];
                        tmparr_ptr[i] = tr;
                    }
                    qsort(tmparr_idx, (void**)tmparr_ptr, 0, static_cast<int>(trend - trnew - 1));

                    // find 0th buddy for each new triangle (i.e. the triangle, which has its idx[2]==tr->idx[1]
                    for (tr = trnew; tr < trend; tr++)
                    {
                        i = bin_search(tmparr_idx, n, tr->idx[1]);
                        tr->buddy[0] = tmparr_ptr[i];
                        tmparr_ptr[i]->buddy[2] = tr;
                    }
                    for (tr = trnew; tr < trend; tr++)
                    {
                        if (!tr->buddy[0] || !tr->buddy[2])
                        {
                            goto endqh;
                        }
                    }

                    // assign all points from the deleted triangles to the new triangles
                    associate_ptlist_with_trilist(ptdeleted, trnew, ptlist, pts);
                    // add new triangles to the list
                    merge_lists(trnext, trnew);
                }
                else if (trnext == trstart)
                {
                    break; // all triangles in queue have no associated vertices
                }
                tr = trnext;
            } while (++iter < maxiter);

        endqh:

            // build the final triangle list
            for (tr = trstart, n = 1; tr->next != trstart; tr = tr->next, n++)
            {
                ;
            }
            if (!pTris)
            {
                pTris = !qmalloc ? new index_t[n * 3] : (index_t*)qmalloc(sizeof(index_t) * n * 3);
            }
            i = 0;
            tr = trstart;
            do
            {
                pTris[i] = tr->idx[0];
                pTris[i + 1] = tr->idx[1];
                pTris[i + 2] = tr->idx[2];
                tr = tr->next;
                i += 3;
            } while (tr != trstart);

        endqhull:
            if (ptlist != ptbuf)
            {
                delete[] ptlist;
            }
            if (tmparr_idx != tmparr_idx_buf)
            {
                delete[] tmparr_idx;
            }
            if (tmparr_ptr != tmparr_ptr_buf)
            {
                delete[] tmparr_ptr;
            }
            if (trdata != trbuf)
            {
                delete[] trdata;
            }

            return n;
        }
#undef DELETE_TRI

        void associate_ptlist_with_trilist(ptitem* ptlist, qhtritem* trilist, ptitem* pt0, strided_pointer<Vec3mem> pvtx)
        {
            if (!ptlist)
            {
                return;
            }
            ptitem* pt = ptlist, * ptnext, * ptlast = ptlist->prev;
            qhtritem* tr;
            int i;
            do
            {
                ptnext = pt->next;
                delete_item_from_list(pt);
                tr = trilist;
                i = static_cast<int>(pt - pt0);
                do
                {
                    if (e_cansee(pvtx[i] - tr->pt0, tr->n))
                    {
                        add_item_to_list(tr->ptassoc, pt);
                        break;
                    }
                    tr = tr->next;
                } while (tr != trilist);
                if (pt == ptlast)
                {
                    break;
                }
                pt = ptnext;
            } while (true);
        }

        void qsort(int* v, void** p, int left, int right)
        {
            if (left >= right)
            {
                return;
            }
            int i, last;
            swap(v, p, left, (left + right) >> 1);
            for (last = left, i = left + 1; i <= right; i++)
            {
                if (v[i] < v[left])
                {
                    swap(v, p, ++last, i);
                }
            }
            swap(v, p, left, last);

            qsort(v, p, left, last - 1);
            qsort(v, p, last + 1, right);
        }

        int bin_search(int* v, int n, int idx)
        {
            int left = 0, right = n, m;
            do
            {
                m = (left + right) >> 1;
                if (v[m] == idx)
                {
                    return m;
                }
                if (v[m] < idx)
                {
                    left = m;
                }
                else
                {
                    right = m;
                }
            } while (left < right - 1);
            return left;
        }
    }

    int qhull(strided_pointer<Vec3> _pts, int npts, index_t*& pTris, qhullmalloc qmalloc)
    {
        return qhull_IMPL::qhull(_pts, npts, pTris, qmalloc);
    }

    int TriangulatePoly(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf)
    {
        return TriangulatePoly_IMPL::TriangulatePoly(pVtx, nVtx, pTris, szTriBuf);
    }

    namespace TriangulatePoly_IMPL
    {
        int TriangulatePolyBruteforce(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf)
        {
            int i, nThunks, nNonEars, nTris = 0;
            vtxthunk* ptr, * ptr0, bufThunks[32], * pThunks = nVtx <= 31 ? bufThunks : new vtxthunk[nVtx + 1];

            ptr = ptr0 = pThunks;
            for (i = nThunks = 0; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    pThunks[nThunks].next[0] = pThunks + nThunks - 1;
                    pThunks[nThunks].next[1] = pThunks + nThunks + 1;
                    pThunks[nThunks].pt = pVtx + i;
                    ptr = pThunks + nThunks++;
                }
            }
            if (nThunks < 3)
            {
                return 0;
            }
            ptr->next[1] = ptr0;
            ptr0->next[0] = ptr;
            for (i = 0; i < nThunks; i++)
            {
                pThunks[i].bProcessed = (*pThunks[i].next[1]->pt - *pThunks[i].pt ^ *pThunks[i].next[0]->pt - *pThunks[i].pt) > 0;
            }

            for (nNonEars = 0; nNonEars < nThunks && nTris < szTriBuf; ptr0 = ptr0->next[1])
            {
                if (nThunks == 3)
                {
                    pTris[nTris * 3] = static_cast<int>(ptr0->pt - pVtx);
                    pTris[nTris * 3 + 1] = static_cast<int>(ptr0->next[1]->pt - pVtx);
                    pTris[nTris * 3 + 2] = static_cast<int>(ptr0->next[0]->pt - pVtx);
                    nTris++;
                    break;
                }
                for (i = 0; (*ptr0->next[1]->pt - *ptr0->pt ^ *ptr0->next[0]->pt - *ptr0->pt) < 0 && i < nThunks; ptr0 = ptr0->next[1], i++)
                {
                    ;
                }
                if (i == nThunks)
                {
                    break;
                }
                for (ptr = ptr0->next[1]->next[1]; ptr != ptr0->next[0] && ptr->bProcessed; ptr = ptr->next[1])
                {
                    ;                                                                                       // find the 1st non-convex vertex after ptr0
                }
                for (; ptr != ptr0->next[0] && min(min(*ptr0->pt - *ptr0->next[0]->pt ^ *ptr->pt - *ptr0->next[0]->pt,
                    *ptr0->next[1]->pt - *ptr0->pt ^ *ptr->pt - *ptr0->pt),
                    *ptr0->next[0]->pt - *ptr0->next[1]->pt ^ *ptr->pt - *ptr0->next[1]->pt) < 0; ptr = ptr->next[1])
                {
                    ;
                }
                if (ptr == ptr0->next[0]) // vertex is an ear, output the corresponding triangle
                {
                    pTris[nTris * 3] = static_cast<int>(ptr0->pt - pVtx);
                    pTris[nTris * 3 + 1] = static_cast<int>(ptr0->next[1]->pt - pVtx);
                    pTris[nTris * 3 + 2] = static_cast<int>(ptr0->next[0]->pt - pVtx);
                    nTris++;
                    ptr0->next[1]->next[0] = ptr0->next[0];
                    ptr0->next[0]->next[1] = ptr0->next[1];
                    nThunks--;
                    nNonEars = 0;
                }
                else
                {
                    nNonEars++;
                }
            }

            if (pThunks != bufThunks)
            {
                delete[] pThunks;
            }
            return nTris;
        }

        int TriangulatePoly(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf)
        {
            if (nVtx < 3)
            {
                return 0;
            }
            vtxthunk* pThunks, * pPrevThunk, * pContStart, ** pSags, ** pBottoms, * pPinnacle, * pBounds[2], * pPrevBounds[2], * ptr, * ptr_next;
            vtxthunk bufThunks[32], * bufSags[16], * bufBottoms[16];
            int i, nThunks, nBottoms = 0, nSags = 0, iBottom = 0, nConts = 0, j, isag, nThunks0, nTris = 0, nPrevSags, nTrisCnt, iter, nDegenTris = 0;
            float ymax, ymin, e, area0 = 0, area1 = 0, cntarea, minCntArea;

            isag = is_unused(pVtx[0].x);
            ymin = ymax = pVtx[isag].y;
            for (i = isag; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    ymin = min(ymin, pVtx[i].y);
                    ymax = max(ymax, pVtx[i].y);
                }
            }
            e = (ymax - ymin) * 0.0005f;
            for (i = 1 + isag; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    j = i < nVtx - 1 && !is_unused(pVtx[i + 1].x) ? i + 1 : isag;
                    if ((ymin = min(pVtx[j].y, pVtx[i - 1].y)) > pVtx[i].y - e)
                    {
                        if ((pVtx[j] - pVtx[i] ^ pVtx[i - 1] - pVtx[i]) > 0)
                        {
                            nBottoms++; // we have a bottom
                        }
                        else if (ymin > pVtx[i].y + 1E-8f)
                        {
                            nSags++; // we have a sag
                        }
                    }
                }
                else
                {
                    nConts++;
                    isag = ++i;
                }
            }
            nSags += nConts;
            if ((nConts - 2) >> 31 & g_bBruteforceTriangulation)
            {
                return TriangulatePolyBruteforce(pVtx, nVtx, pTris, szTriBuf);
            }
            pThunks = nVtx + nSags * 2 <= sizeof(bufThunks) / sizeof(bufThunks[0]) ? bufThunks : new vtxthunk[nVtx + nSags * 2];

            for (i = nThunks = 0, pContStart = pPrevThunk = pThunks; i < nVtx; i++)
            {
                if (!is_unused(pVtx[i].x))
                {
                    pThunks[nThunks].next[1] = pThunks + nThunks;
                    pThunks[nThunks].next[1] = pPrevThunk->next[1];
                    pPrevThunk->next[1] = pThunks + nThunks;
                    pThunks[nThunks].next[0] = pPrevThunk;
                    pThunks[nThunks].jump = 0;
                    pPrevThunk = pThunks + nThunks;
                    pThunks[nThunks].bProcessed = 0;
                    pThunks[nThunks++].pt = &pVtx[i];
                }
                else
                {
                    pPrevThunk->next[1] = pContStart;
                    pContStart->next[0] = pThunks + nThunks - 1;
                    pContStart = pPrevThunk = pThunks + nThunks;
                }
            }

            for (i = j = 0, cntarea = 0, minCntArea = 1; i < nThunks; i++)
            {
                cntarea += *pThunks[i].pt ^ *pThunks[i].next[1]->pt;
                j++;
                if (pThunks[i].next[1] != pThunks + i + 1)
                {
                    if (j >= 3)
                    {
                        area0 += cntarea;
                        minCntArea = min(cntarea, minCntArea);
                    }
                    cntarea = 0;
                    j = 0;
                }
            }
            if (minCntArea > 0 && nConts > 1)
            {
                // if all contours are positive, triangulate them as separate (it's more safe)
                for (i = 0; i < nThunks; i++)
                {
                    if (pThunks[i].next[0] != pThunks + i - 1)
                    {
                        nTrisCnt = TriangulatePoly(pThunks[i].pt, static_cast<int>((pThunks[i].next[0]->pt - pThunks[i].pt) + 2), pTris + nTris * 3, szTriBuf - nTris * 3);
                        for (j = 0, isag = static_cast<int>(pThunks[i].pt - pVtx); j < nTrisCnt * 3; j++)
                        {
                            pTris[nTris * 3 + j] += isag;
                        }
                        i = static_cast<int>(pThunks[i].next[0] - pThunks);
                        nTris += nTrisCnt;
                    }
                }
                if (pThunks != bufThunks)
                {
                    delete[] pThunks;
                }
                return nTris;
            }

            pSags = nSags <= sizeof(bufSags) / sizeof(bufSags[0]) ? bufSags : new vtxthunk * [nSags];
            pBottoms = nSags + nBottoms <= sizeof(bufBottoms) / sizeof(bufBottoms[0]) ? bufBottoms : new vtxthunk * [nSags + nBottoms];

            for (i = nSags = nBottoms = 0; i < nThunks; i++)
            {
                if ((ymin = min(pThunks[i].next[1]->pt->y, pThunks[i].next[0]->pt->y)) > pThunks[i].pt->y - e)
                {
                    if ((*pThunks[i].next[1]->pt - *pThunks[i].pt ^ *pThunks[i].next[0]->pt - *pThunks[i].pt) >= 0)
                    {
                        pBottoms[nBottoms++] = pThunks + i; // we have a bottom
                    }
                    else if (ymin > pThunks[i].pt->y + e)
                    {
                        pSags[nSags++] = pThunks + i; // we have a sag
                    }
                }
            }
            iBottom = -1;
            pBounds[0] = pBounds[1] = pPrevBounds[0] = pPrevBounds[1] = 0;
            nThunks0 = nThunks;
            nPrevSags = nSags;
            iter = nThunks * 4;

            do
            {
            nextiter:
                if (!pBounds[0])   // if bounds are empty, get the next available bottom
                {
                    for (++iBottom; iBottom < nBottoms && !pBottoms[iBottom]->next[0]; iBottom++)
                    {
                        ;
                    }
                    if (iBottom >= nBottoms)
                    {
                        break;
                    }
                    pBounds[0] = pBounds[1] = pPinnacle = pBottoms[iBottom];
                }
                pBounds[0]->bProcessed = pBounds[1]->bProcessed = 1;
                if (pBounds[0] == pPrevBounds[0] && pBounds[1] == pPrevBounds[1] && nSags == nPrevSags || !pBounds[0]->next[0] || !pBounds[1]->next[0])
                {
                    pBounds[0] = pBounds[1] = 0;
                    continue;
                }
                pPrevBounds[0] = pBounds[0];
                pPrevBounds[1] = pBounds[1];
                nPrevSags = nSags;

                // check if left or right is a top
                for (i = 0; i < 2; i++)
                {
                    if (pBounds[i]->next[0]->pt->y < pBounds[i]->pt->y && pBounds[i]->next[1]->pt->y <= pBounds[i]->pt->y &&
                        (*pBounds[i]->next[0]->pt - *pBounds[i]->pt ^ *pBounds[i]->next[1]->pt - *pBounds[i]->pt) > 0)
                    {
                        if (pBounds[i]->jump)
                        {
                            do
                            {
                                ptr = pBounds[i]->jump;
                                pBounds[i]->jump = 0;
                                pBounds[i] = ptr;
                            } while (pBounds[i]->jump);
                        }
                        else
                        {
                            pBounds[i]->jump = pBounds[i ^ 1];
                            pBounds[0] = pBounds[1] = 0;
                            goto nextiter;
                        }
                        if (!pBounds[0]->next[0] || !pBounds[1]->next[0])
                        {
                            pBounds[0] = pBounds[1] = 0;
                            goto nextiter;
                        }
                    }
                }
                i = isneg(pBounds[1]->next[1]->pt->y - pBounds[0]->next[0]->pt->y);
                ymax = pBounds[i ^ 1]->next[i ^ 1]->pt->y;
                ymin = min(pBounds[0]->pt->y, pBounds[1]->pt->y);

                for (j = 0, isag = -1; j < nSags; j++)
                {
                    if (inrange(pSags[j]->pt->y, ymin, ymax) &&                           // find a sag in next left-left-right-next right quad
                        pSags[j] != pBounds[0]->next[0] && pSags[j] != pBounds[1]->next[1] &&
                        (*pBounds[0]->pt - *pBounds[0]->next[0]->pt ^ *pSags[j]->pt - *pBounds[0]->next[0]->pt) >= 0 &&
                        (*pBounds[1]->pt - *pBounds[0]->pt ^ *pSags[j]->pt - *pBounds[0]->pt) >= 0 &&
                        (*pBounds[1]->next[1]->pt - *pBounds[1]->pt ^ *pSags[j]->pt - *pBounds[1]->pt) >= 0 &&
                        (*pBounds[0]->next[0]->pt - *pBounds[1]->next[1]->pt ^ *pSags[j]->pt - *pBounds[1]->next[1]->pt) >= 0)
                    {
                        ymax = pSags[j]->pt->y;
                        isag = j;
                    }
                }

                if (isag >= 0) // build a bridge between the sag and the highest active point
                {
                    if (pSags[isag]->next[0])
                    {
                        pPinnacle->next[1]->next[0] = pThunks + nThunks;
                        pSags[isag]->next[0]->next[1] = pThunks + nThunks + 1;
                        pThunks[nThunks].next[0] = pThunks + nThunks + 1;
                        pThunks[nThunks].next[1] = pPinnacle->next[1];
                        pThunks[nThunks + 1].next[1] = pThunks + nThunks;
                        pThunks[nThunks + 1].next[0] = pSags[isag]->next[0];
                        pPinnacle->next[1] = pSags[isag];
                        pSags[isag]->next[0] = pPinnacle;
                        pThunks[nThunks].pt = pPinnacle->pt;
                        pThunks[nThunks + 1].pt = pSags[isag]->pt;
                        pThunks[nThunks].jump = pThunks[nThunks + 1].jump = 0;
                        pThunks[nThunks].bProcessed = pThunks[nThunks + 1].bProcessed = 0;
                        if (pBounds[1] == pPinnacle)
                        {
                            pBounds[1] = pThunks + nThunks;
                        }
                        for (ptr = pThunks + nThunks, j = 0; ptr != pBounds[1]->next[1] && j < nThunks; ptr = ptr->next[1], j++)
                        {
                            if (min(ptr->next[0]->pt->y, ptr->next[1]->pt->y) > ptr->pt->y)  // ptr is a bottom
                            {
                                pBottoms[nBottoms++] = ptr;
                                break;
                            }
                        }
                        pBounds[1] = pPinnacle;
                        pPinnacle = pSags[isag];
                        nThunks += 2;
                    }
                    for (j = isag; j < nSags - 1; j++)
                    {
                        pSags[j] = pSags[j + 1];
                    }
                    --nSags;
                    continue;
                }

                // create triangles featuring the new vertex
                for (ptr = pBounds[i]; ptr != pBounds[i ^ 1] && nTris < szTriBuf; ptr = ptr_next)
                {
                    if ((*ptr->next[i ^ 1]->pt - *ptr->pt ^ *ptr->next[i]->pt - *ptr->pt) * (1 - i * 2) > 0 || pBounds[0]->next[0] == pBounds[1]->next[1])
                    {
                        // output the triangle
                        pTris[nTris * 3] = static_cast<int>(pBounds[i]->next[i]->pt - pVtx);
                        pTris[nTris * 3 + 1 + i] = static_cast<int>(ptr->pt - pVtx);
                        pTris[nTris * 3 + 2 - i] = static_cast<int>(ptr->next[i ^ 1]->pt - pVtx);
                        vector2df edge0 = pVtx[pTris[nTris * 3 + 1]] - pVtx[pTris[nTris * 3]], edge1 = pVtx[pTris[nTris * 3 + 2]] - pVtx[pTris[nTris * 3]];
                        float darea = edge0 ^ edge1;
                        area1 += darea;
                        nDegenTris += isneg(sqr(darea) - sqr(0.02f) * (edge0 * edge0) * (edge1 * edge1));
                        nTris++;
                        ptr->next[i ^ 1]->next[i] = ptr->next[i];
                        ptr->next[i]->next[i ^ 1] = ptr->next[i ^ 1];
                        pBounds[i] = ptr_next = ptr->next[i ^ 1];
                        if (pPinnacle == ptr)
                        {
                            pPinnacle = ptr->next[i];
                        }
                        ptr->next[0] = ptr->next[1] = 0;
                        ptr->bProcessed = 1;
                    }
                    else
                    {
                        break;
                    }
                }

                if ((pBounds[i] = pBounds[i]->next[i]) == pBounds[i ^ 1]->next[i ^ 1])
                {
                    pBounds[0] = pBounds[1] = 0;
                }
                else if (pBounds[i]->pt->y > pPinnacle->pt->y)
                {
                    pPinnacle = pBounds[i];
                }
            } while (nTris < szTriBuf && --iter);

            if (pThunks != bufThunks)
            {
                delete[] pThunks;
            }
            if (pBottoms != bufBottoms)
            {
                delete[] pBottoms;
            }
            if (pSags != bufSags)
            {
                delete[] pSags;
            }

            int bProblem = nTris<nThunks0 - nConts * 2 || fabs_tpl(area0 - area1)>area0 * 0.003f || nTris >= szTriBuf;
            if (bProblem || nDegenTris)
            {
                if (nConts == 1)
                {
                    return TriangulatePolyBruteforce(pVtx, nVtx, pTris, szTriBuf);
                }
                else
                {
                    g_nTriangulationErrors += bProblem;
                }
            }

            return nTris;
        }
    }
}
