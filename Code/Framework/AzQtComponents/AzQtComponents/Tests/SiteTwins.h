/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// ----------------------------------------------------------------------------------
// SiteTwins: helpers of the tests that twin a site which set an inline style sheet: the census of inline style sheets
// (O3DE_UI_SHEET_CENSUS, GuiPerfFloor::installSheetCensus) for the widgets a test builds. A site the flattening has converted sets its
// sheet in the legacy GUI and none in the flattened one, and the census says which, one line for each carrier and sheet in the legacy
// rung, none for the site in the flat rung.
// Everything lives in the namespace SiteTwins, so that a unity build joining this file with other test files finds no second definition.
// ----------------------------------------------------------------------------------

#include <AzQtComponents/Components/GuiPerfFloor.h>

#include <QApplication>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include <functional>

namespace AzQtComponents
{
    namespace SiteTwins
    {
        //! The lines the census writes while `build` makes and polishes widgets at `rung`. The census is installed on the application
        //! (a second install does nothing) and forgets the sheets it has written before the build, so each call lists every sheet its
        //! widgets set. The rung of the process is put back.
        inline QStringList sheetLinesAt(GuiPerfFloor::Rung rung, const std::function<void()>& build)
        {
            const GuiPerfFloor::Rung before = GuiPerfFloor::rung();
            GuiPerfFloor::resetForTesting();
            GuiPerfFloor::setRungForTesting(rung);

            QStringList lines;
            GuiPerfFloor::captureLinesInto(&lines);
            GuiPerfFloor::installSheetCensus(qobject_cast<QApplication*>(QCoreApplication::instance()));
            build();
            GuiPerfFloor::captureLinesInto(nullptr);

            GuiPerfFloor::setRungForTesting(before);
            return lines;
        }

        //! Whether a line of the census names the carrier (the class, with or without the object name)
        inline bool namesCarrier(const QString& line, const QString& carrier)
        {
            return line.contains(QStringLiteral("| ") + carrier);
        }
    } // namespace SiteTwins
} // namespace AzQtComponents
