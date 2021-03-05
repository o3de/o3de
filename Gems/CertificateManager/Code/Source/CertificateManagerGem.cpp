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

#include <CertificateManagerGem.h>
#include <CertificateManager/DataSource/IDataSource.h>

#include "Source/DataSource/FileDataSource.h"

namespace CertificateManager
{
    CertificateManagerModule::CertificateManagerModule()
        : m_fileDataSource(nullptr)
    {        
        FileDataSourceCreationBus::Handler::BusConnect();
    }

    CertificateManagerModule::~CertificateManagerModule()
    {
        FileDataSourceCreationBus::Handler::BusDisconnect();
        delete m_fileDataSource;
    }

    void CertificateManagerModule::CreateFileDataSource()
    {
        if (m_fileDataSource == nullptr)
        {
            m_fileDataSource = aznew FileDataSource();
        }        
    }

    void CertificateManagerModule::DestroyFileDataSource()
    {
        delete m_fileDataSource;
        m_fileDataSource = nullptr;
    }

} // namespace CertificateManager

AZ_DECLARE_MODULE_CLASS(Gem_CertificateManager, CertificateManager::CertificateManagerModule)
