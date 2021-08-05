/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QLabel>

namespace PhysX
{
    namespace Editor
    {
        class DocumentationLinkWidget
            : public QLabel
        {
        public:
            explicit DocumentationLinkWidget(const QString& linkFormat, const QString& linkAddress);
        };
    }
}
