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

#include "CrySimpleSock.hpp"
#include "CrySimpleJobCompile2.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <tinyxml/tinyxml.h>



CCrySimpleJobCompile2::CCrySimpleJobCompile2(EProtocolVersion version, uint32_t requestIP, std::vector<uint8_t>* pRVec)
    : CCrySimpleJobCompile(requestIP, version, pRVec)
{
}

size_t CCrySimpleJobCompile2::SizeOf(std::vector<uint8_t>& rVec)
{
    const char* pXML        =   reinterpret_cast<const char*>(&rVec[0]);
    const char* pFirst  =   strstr(pXML, "HashStop");
    return pFirst ? pFirst - pXML : rVec.size();
}
