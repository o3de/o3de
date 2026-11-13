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
            // The flattened GUI's element reaches the links by this class. The legacy GUI's sheets reach it as a QLabel, as
            // before: a type selector matches a subclass.
            Q_OBJECT

        public:
            explicit DocumentationLinkWidget(const QString& linkFormat, const QString& linkAddress);

            //! Flattened GUI: the elements that draw the link's fill and the fill of the menu it pops up, which the legacy GUI draws
            //! with the sheet the link sets on itself. The first link built in the flattened GUI registers them, once.
            static void RegisterFlatElements();
        };
    }
}
