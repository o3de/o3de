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

#ifndef __CRYSIMPLEJOBCACHE__
#define __CRYSIMPLEJOBCACHE__

#include "CrySimpleJob.hpp"
#include <Core/STLHelper.hpp>

class CCrySimpleJobCache
    :   public  CCrySimpleJob
{
    tdHash                      m_HashID;

protected:

    void                            CheckHashID(std::vector<uint8_t>& rVec, size_t Size);

public:
    CCrySimpleJobCache(uint32_t requestIP);

    tdHash                      HashID() const{return m_HashID; }
};

#endif
