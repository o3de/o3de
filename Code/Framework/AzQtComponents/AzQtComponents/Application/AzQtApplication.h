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

        //! Initializes Qt DPI scaling to handle displays with high display densities, such as Retina displays.
            //! Currently, this uses Qt's system DPI awareness, in which a common device scaling factor will be
            //! calculated across all attached screens.
            //! \warning This must be called before this AzQtApplication instance is initialized.
        static void InitializeDpiScaling();
    };

} // namespace AzQtComponents



