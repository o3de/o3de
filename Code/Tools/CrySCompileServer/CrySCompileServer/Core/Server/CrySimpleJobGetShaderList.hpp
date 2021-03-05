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

#ifndef __CRYSIMPLEJOBGETSHADERLIST__
#define __CRYSIMPLEJOBGETSHADERLIST__

#include "CrySimpleJob.hpp"


class CCrySimpleJobGetShaderList
    :   public  CCrySimpleJob
{
public:
    CCrySimpleJobGetShaderList(uint32_t requestIP, std::vector<uint8_t>* pRVec);

    virtual bool Execute(const TiXmlElement* pElement);
    std::vector<uint8_t>* m_pRVec = nullptr;
};

#endif
