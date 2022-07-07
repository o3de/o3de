// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QWidget>
#endif

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Widget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit ${SanitizedCppName}Widget(QWidget* parent = nullptr);
    };
} 
