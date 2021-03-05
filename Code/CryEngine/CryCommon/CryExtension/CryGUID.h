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

// Description : Part of CryEngine's extension framework.


#ifndef CRYINCLUDE_CRYEXTENSION_CRYGUID_H
#define CRYINCLUDE_CRYEXTENSION_CRYGUID_H
#pragma once

#include "Serialization/IArchive.h"
#include "Random.h"

#include <functional>

struct CryGUID
{
    uint64 hipart;
    uint64 lopart;

    // !!! Do NOT turn CryGUID into a non-aggregate !!!
    // It will prevent inlining and type list unrolling opportunities within
    // cryinterface_cast<T>() and cryiidof<T>(). As such prevent constructors,
    // non-public members, base classes and virtual functions!

    //CryGUID() : hipart(0), lopart(0) {}
    //CryGUID(uint64 h, uint64 l) : hipart(h), lopart(l) {}

    static CryGUID Construct(const uint64& hipart, const uint64& lopart)
    {
        CryGUID guid = {hipart, lopart};
        return guid;
    }

    static CryGUID Create()
    {
        uint64 lopart = 0;
        uint64 hipart = 0;
        while (lopart == 0 || hipart == 0)
        {
            const uint32 a = cry_random_uint32();
            const uint32 b = cry_random_uint32();
            const uint32 c = cry_random_uint32();
            const uint32 d = cry_random_uint32();
            lopart = (uint64)a | ((uint64)b << 32);
            hipart = (uint64)c | ((uint64)d << 32);
        }

        return Construct(lopart, hipart);
    }

    static CryGUID Null()
    {
        return Construct(0, 0);
    }

    bool operator ==(const CryGUID& rhs) const {return hipart == rhs.hipart && lopart == rhs.lopart; }
    bool operator !=(const CryGUID& rhs) const {return hipart != rhs.hipart || lopart != rhs.lopart; }
    bool operator <(const CryGUID& rhs) const {return hipart == rhs.hipart ? lopart < rhs.lopart : hipart < rhs.hipart; }

    void Serialize(Serialization::IArchive& ar)
    {
        if (ar.IsInput())
        {
            uint32 dwords[4];
            ar(dwords, "guid");
            lopart = (((uint64)dwords[1]) << 32) | (uint64)dwords[0];
            hipart = (((uint64)dwords[3]) << 32) | (uint64)dwords[2];
        }
        else
        {
            uint32 guid[4] = {
                (uint32)(lopart & 0xFFFFFFFF), (uint32)((lopart >> 32) & 0xFFFFFFFF),
                (uint32)(hipart & 0xFFFFFFFF), (uint32)((hipart >> 32) & 0xFFFFFFFF)
            };
            ar(guid, "guid");
        }
    }
};

// This is only used by the editor where we use C++ 11.
namespace std
{
    template<>
    struct hash<CryGUID>
    {
    public:
        size_t operator()(const CryGUID& guid) const
        {
            std::hash<uint64> hasher;
            return hasher(guid.lopart) ^ hasher(guid.hipart);
        }
    };
}

namespace AZStd
{
    template<>
    struct hash<CryGUID>
    {
    public:
        size_t operator()(const CryGUID& guid) const
        {
            std::hash<CryGUID> hasher;
            return hasher(guid);
        }
    };
}

#define MAKE_CRYGUID(high, low) CryGUID::Construct((uint64) high##LL, (uint64) low##LL)


#endif // CRYINCLUDE_CRYEXTENSION_CRYGUID_H
