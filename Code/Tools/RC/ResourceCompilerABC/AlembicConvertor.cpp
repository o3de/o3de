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

#include "ResourceCompilerABC_precompiled.h"
#include "AlembicConvertor.h"
#include "AlembicCompiler.h"

#include <AzCore/Memory/SystemAllocator.h>

AlembicConvertor::AlembicConvertor(ICryXML* pXMLParser, IPakSystem* pPakSystem)
    : m_refCount(1)
    , m_pXMLParser(pXMLParser)
    , m_pPakSystem(pPakSystem)
{
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
}

AlembicConvertor::~AlembicConvertor()
{
    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
}

void AlembicConvertor::Release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

ICompiler* AlembicConvertor::CreateCompiler()
{
    return new AlembicCompiler(m_pXMLParser);
}

const char* AlembicConvertor::GetExt(int index) const
{
    switch (index)
    {
    case 0:
        return "abc";
    default:
        return 0;
    }
}
