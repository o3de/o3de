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

#include "CrySimpleJobCache.hpp"
#include "CrySimpleCache.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <Core/Common.h>
#include <tinyxml/tinyxml.h>

CCrySimpleJobCache::CCrySimpleJobCache(uint32_t requestIP)
    : CCrySimpleJob(requestIP)
{
}

void CCrySimpleJobCache::CheckHashID(std::vector<uint8_t>& rVec, size_t Size)
{
    m_HashID = CSTLHelper::Hash(rVec, Size);
    if (CCrySimpleCache::Instance().Find(m_HashID, rVec))
    {
        State(ECSJS_CACHEHIT);
        logmessage("\r"); // Just update cache hit number
    }
}
