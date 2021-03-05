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
