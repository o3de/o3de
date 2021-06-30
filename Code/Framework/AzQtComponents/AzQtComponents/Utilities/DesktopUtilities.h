/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QString>

namespace AzQtComponents
{
    AZ_QT_COMPONENTS_API void ShowFileOnDesktop(const QString& path);

    AZ_QT_COMPONENTS_API QString fileBrowserActionName();
};
