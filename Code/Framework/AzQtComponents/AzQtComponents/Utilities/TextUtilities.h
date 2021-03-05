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

class QWidget;

namespace AzQtComponents
{

    /**
         Forces the tooltip text on a QWidget to do line wrapping.

         Qt will only line wrap tooltips if they are rich text / HTML so if Qt doesn't think this text
         is rich text, we throw on some unnecessary <b></b> tags to make it think it is HTML. Also
         replaces \n characters with <br/> tags.
     */
    AZ_QT_COMPONENTS_API void forceToolTipLineWrap(QWidget* widget);

} // namespace AzQtComponents

