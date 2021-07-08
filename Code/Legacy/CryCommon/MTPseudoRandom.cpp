/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Marsenne Twister PRNG. See MT.h for more info.

#include "MTPseudoRandom.h"
// non-inline function definitions and static member definitions cannot
// reside in header file because of the risk of multiple declarations

void CMTRand_int32::gen_state()   // generate new m_nState vector
{
    for (int i = 0; i < (n - m); ++i)
    {
        m_nState[i] = m_nState[i + m] ^ twiddle(m_nState[i], m_nState[i + 1]);
    }
    for (int i = n - m; i < (n - 1); ++i)
    {
        m_nState[i] = m_nState[i + m - n] ^ twiddle(m_nState[i], m_nState[i + 1]);
    }
    m_nState[n - 1] = m_nState[m - 1] ^ twiddle(m_nState[n - 1], m_nState[0]);
    p = 0; // reset position
}

void CMTRand_int32::seed(uint32 s)    // init by 32 bit seed
{   //if (s == 0)
    //m_nRandom = 1;
    for (int i = 0; i < n; ++i)
    {
        m_nState[i] = 0x0UL;
    }
    m_nState[0] = s;
    for (int i = 1; i < n; ++i)
    {
        m_nState[i] = 1812433253UL * (m_nState[i - 1] ^ (m_nState[i - 1] >> 30)) + i;
        // see Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier
        // in the previous versions, MSBs of the seed affect only MSBs of the array m_nState
        // 2002/01/09 modified by Makoto Matsumoto
    }
    p = n; // force gen_state() to be called for next random number
}

void CMTRand_int32::seed(const uint32* array, int size)   // init by array
{
    seed(19650218UL);
    int i = 1, j = 0;
    for (int k = ((n > size) ? n : size); k; --k)
    {
        m_nState[i] = (m_nState[i] ^ ((m_nState[i - 1] ^ (m_nState[i - 1] >> 30)) * 1664525UL))
            + array[j] + j; // non linear
        ++j;
        j %= size;
        if ((++i) == n)
        {
            m_nState[0] = m_nState[n - 1];
            i = 1;
        }
    }
    for (int k = n - 1; k; --k)
    {
        PREFAST_SUPPRESS_WARNING(6385)  PREFAST_SUPPRESS_WARNING(6386)  m_nState[i] = (m_nState[i] ^ ((m_nState[i - 1] ^ (m_nState[i - 1] >> 30)) * 1566083941UL)) - i;
        if ((++i) == n)
        {
            m_nState[0] = m_nState[n - 1];
            i = 1;
        }
    }
    m_nState[0] = 0x80000000UL; // MSB is 1; assuring non-zero initial array
    p = n; // force gen_state() to be called for next random number
}
