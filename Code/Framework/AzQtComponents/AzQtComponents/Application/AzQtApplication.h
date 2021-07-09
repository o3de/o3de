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

#include <QApplication>

#include <AzQtComponents/Utilities/HandleDpiAwareness.h>

namespace AzQtComponents
{
    //! Base class for O3DE Tools Applications
    class AZ_QT_COMPONENTS_API AzQtApplication
        : public QApplication
    {
    public:       
        AzQtApplication(int& argc, char** argv);

        //! DPI Scaling so that we support HighDpi monitors, like the Retina displays on Windows 10
            //! Must be set before QApplication is initialized, 
        static void SetDpiScaling();
    };

} // namespace AzQtComponents



