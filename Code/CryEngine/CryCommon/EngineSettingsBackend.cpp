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


#include "EngineSettingsBackend.h"

#ifdef CRY_ENABLE_RC_HELPER

CEngineSettingsBackend::CEngineSettingsBackend(CEngineSettingsManager* parent, const wchar_t* moduleName)
    : m_parent(parent)
    , m_moduleName()
{
    if (moduleName != nullptr)
    {
        m_moduleName = moduleName;
    }
}

CEngineSettingsBackend::~CEngineSettingsBackend()
{

}

#endif // CRY_ENABLE_RC_HELPER

