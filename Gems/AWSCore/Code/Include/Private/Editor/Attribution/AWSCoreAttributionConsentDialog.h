/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <QMessageBox>

namespace AWSCore
{
    //! Defines AWSCoreAttributionConsent QT dialog as QT message box.
    class AWSCoreAttributionConsentDialog :
        public QMessageBox
    {
    public:
        AZ_CLASS_ALLOCATOR(AWSCoreAttributionConsentDialog, AZ::SystemAllocator, 0);
        AWSCoreAttributionConsentDialog();
        virtual ~AWSCoreAttributionConsentDialog() = default;
       
    };
} // namespace AWSCore
