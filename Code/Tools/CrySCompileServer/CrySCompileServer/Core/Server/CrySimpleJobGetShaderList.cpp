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

#include "CrySimpleJobGetShaderList.hpp"
#include "ShaderList.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <Core/Common.h>
#include <tinyxml/tinyxml.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/string/string.h>


CCrySimpleJobGetShaderList::CCrySimpleJobGetShaderList(uint32_t requestIP, std::vector<uint8_t>* pRVec)
    : CCrySimpleJob(requestIP)
    , m_pRVec(pRVec)
{
}

bool CCrySimpleJobGetShaderList::Execute(const TiXmlElement* pElement)
{
    AZStd::string shaderListFilename;

    const char* project = pElement->Attribute("Project");
    const char* shaderList = pElement->Attribute("ShaderList");
    const char* platform = pElement->Attribute("Platform");
    const char* compiler = pElement->Attribute("Compiler");
    const char* language = pElement->Attribute("Language");

    shaderListFilename = AZStd::string::format("./Cache/%s%s-%s-%s/%s", project, platform, compiler, language, shaderList);

    //open the file and read into the rVec
    
    FILE* pFile = nullptr;
    azfopen(&pFile, shaderListFilename.c_str(), "rb");
    if (!pFile)
    {
        // Fake a good result. We can't be sure if this file name is bad or if it doesn't exist *yet*, so we'll just assume the latter.
        m_pRVec->resize(4, '\0');
        State(ECSJS_DONE);
        return true;
    }

    fseek(pFile, 0, SEEK_END);
    size_t fileSize = ftell(pFile);
    m_pRVec->resize(fileSize);
    fseek(pFile, 0, SEEK_SET);

    size_t remaining = fileSize;
    size_t read = 0;
    while (remaining)
    {
        read += fread(m_pRVec->data() + read, 1, remaining, pFile);
        remaining -= read;
    }

    fclose(pFile);

    //compress before sending
    tdDataVector rDataRaw;
    rDataRaw.swap(*m_pRVec);
    if (!CSTLHelper::Compress(rDataRaw, *m_pRVec))
    {
        State(ECSJS_ERROR_COMPRESS);
        CrySimple_ERROR("failed to compress request");
        return false;
    }
    State(ECSJS_DONE);

    return true;
}
