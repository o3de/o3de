/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

