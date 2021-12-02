/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#endif

namespace O3DE::ProjectManager
{
    static constexpr char ProjectManagerKeyPrefix[] = "/O3DE/ProjectManager";

    void SaveProjectManagerSettings();
    QString GetProjectBuiltSuccessfullyKey(const QString& projectName);
    QString GetExternalLinkWarningKey();
}
