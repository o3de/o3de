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

#ifndef __CRYSIMPLEJOBCOMPILE2__
#define __CRYSIMPLEJOBCOMPILE2__

#include "CrySimpleJobCompile.hpp"


class CCrySimpleJobCompile2
    :   public  CCrySimpleJobCompile
{
    virtual size_t      SizeOf(std::vector<uint8_t>& rVec);

public:
    CCrySimpleJobCompile2(EProtocolVersion version, uint32_t requestIP, std::vector<uint8_t>* pRVec);
};

#endif
