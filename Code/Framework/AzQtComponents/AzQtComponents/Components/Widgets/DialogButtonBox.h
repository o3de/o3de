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

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QProxyStyle>

class QWidget;

namespace AzQtComponents
{
    class Style;

    //! Class to handle styling and painting of QToolButton controls.
    class AZ_QT_COMPONENTS_API DialogButtonBox
    {
    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget);
    };
} // namespace AzQtComponents
