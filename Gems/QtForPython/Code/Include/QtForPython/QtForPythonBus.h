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

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>

namespace QtForPython
{
    //////////////////////////////////////////////////////////////////////////
    // Used to fetch the data points a bootstrap script requires to hook in
    // QtForPython (aka PySide2) 
    struct QtBootstrapParameters
    {
        AZ_TYPE_INFO(QtBootstrapParameters, "{4103CF43-6CF7-413D-B2C8-D511E23BAB50}");

        //! The path of the Qt binary files such as qt5core.dll
        AZStd::string m_qtBinaryFolder;

        //! The path of the Qt plugins such as /qtlibs/plugins
        AZStd::string m_qtPluginsFolder;

        //! The 'winId' of the main Qt window in the Lumberyard editor
        AZ::u64 m_mainWindowId;

        //! PySide package folder to attach to the Python system path
        AZStd::string m_pySidePackageFolder;
    };

    //! Used to fetch tools framework data 
    class QtForPythonRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Check to make sure Python is ready and active
        virtual bool IsActive() const = 0;

        //! Fetches the data a bootstrap script requires to hook in QtForPython
        virtual QtBootstrapParameters GetQtBootstrapParameters() const = 0;
    };
    using QtForPythonRequestBus = AZ::EBus<QtForPythonRequests>;
}

