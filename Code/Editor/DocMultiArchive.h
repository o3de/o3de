/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_DOCMULTIARCHIVE_H
#define CRYINCLUDE_EDITOR_DOCMULTIARCHIVE_H
#pragma once

#include "Util/XmlArchive.h"

// Segmented World need XML Archive of editor level being split into multiple Archives
// Because multiArchive is a super set of single archive, Doc can always use it, with or without SW Level
// so some of the interface functions was written as always using MultiArchive
// However, to decouple other sandbox interfaces with SW stuff(prevent important header files from being polluted),
// it's better put it here as a generic implement and be included in both sides

// the current available slots for MultiArchive
// used by segmented world code through enum mapping (to EWDBType)
enum EDocMultiArchiveSlot
{
    DMAS_GENERAL = 0,
    DMAS_TERRAIN_LAYERS,
    DMAS_VEGETATION,
    DMAS_TIME_OF_DAY,
    DMAS_ENVIRONMENT,
    DMAS_GENERAL_NAMED_DATA,

    DMAS_USER,                      // Per user data,

    DMAS_COUNT,
};


typedef CXmlArchive* TDocMultiArchive [DMAS_COUNT];
inline void FillXmlArArray(TDocMultiArchive& arrXmlAr, CXmlArchive* pXmlAr)
{
    for (int i = 0; i < DMAS_COUNT; i++)
    {
        arrXmlAr[i] = pXmlAr;
    }
}

inline bool IsLoadingXmlArArray(TDocMultiArchive& arrXmlAr)
{
    bool bLoading = false;
    for (int i = 0; i < DMAS_COUNT; i++)
    {
        if (arrXmlAr[i])
        {
            bLoading = arrXmlAr[i]->bLoading;
            break;
        }
    }
    return bLoading;
}

#endif // CRYINCLUDE_EDITOR_DOCMULTIARCHIVE_H
