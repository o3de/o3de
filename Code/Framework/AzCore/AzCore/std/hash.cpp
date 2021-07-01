/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/hash.h>
#include <AzCore/std/algorithm.h>

namespace AZStd
{
    static constexpr AZStd::size_t prime_list[] = {
        7ul,          23ul,
        53ul,         97ul,         193ul,       389ul,       769ul,
        1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
        49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
        1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
        50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
        1610612741ul, 3221225473ul, 4294967291ul
    };

    AZStd::size_t hash_next_bucket_size(AZStd::size_t n)
    {
        const AZStd::size_t* first = prime_list;
        const AZStd::size_t* last =  prime_list + AZ_ARRAY_SIZE(prime_list);
        const AZStd::size_t* pos = AZStd::lower_bound(first, last, n);
        return (pos == last ? *(last - 1) : *pos);
    }
}
