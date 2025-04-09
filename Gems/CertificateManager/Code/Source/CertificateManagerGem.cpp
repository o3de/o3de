/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), CertificateManager::CertificateManagerModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_CertificateManager, CertificateManager::CertificateManagerModule)
#endif
