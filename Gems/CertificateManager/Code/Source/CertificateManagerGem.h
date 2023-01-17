/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef INCLUDE_CERTIFICATEMANAGERGEM_HEADER
#define INCLUDE_CERTIFICATEMANAGERGEM_HEADER

#include <CertificateManager/ICertificateManagerGem.h>
#include <CertificateManager/DataSource/FileDataSourceBus.h>

namespace CertificateManager
{
    class FileDataSource;

    class CertificateManagerModule
        : public CryHooksModule
        , public FileDataSourceCreationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CertificateManagerModule, AZ::SystemAllocator)
        AZ_RTTI(CertificateManagerModule,"{11C0C40E-3576-4AFD-A708-B1EE70DF907B}",CryHooksModule);

        CertificateManagerModule();
        ~CertificateManagerModule() override;
        
    private:        

        // FileDataSourceCreationBus
        void CreateFileDataSource() override;
        void DestroyFileDataSource() override;

        FileDataSource* m_fileDataSource;
    };
} // namespace CertificateManager
#endif
