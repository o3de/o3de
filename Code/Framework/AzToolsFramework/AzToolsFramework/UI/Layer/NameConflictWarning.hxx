/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <QMessageBox>

namespace AzToolsFramework
{
    namespace Layers
    {
        class NameConflictWarning
            : public QMessageBox
        {
        public:
            NameConflictWarning(QWidget* parent, const AZStd::unordered_map<AZStd::string, int>& nameConflictMapping);

        private:
            const QString replacementStr = " > ";
        };
    }
}
