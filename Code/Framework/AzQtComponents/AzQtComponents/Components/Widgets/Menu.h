/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

class QLineEdit;
class QSettings;
class QStyleOption;

namespace AzQtComponents
{
    class Style;

    //! Class to handle styling and painting of QMenu controls.
    class AZ_QT_COMPONENTS_API Menu
    {
    public:
        //! Style configuration for the Menu class.
        struct Config
        {
            int subMenuOverlap; //!< Amount of pixels submenus overlap with their parent Menu.
        };

        //! Sets the Menu style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the Menu.
        static Config loadConfig(QSettings& settings);
        //! Gets the default Menu style configuration.
        static Config defaultConfig();

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);

        static int subMenuOverlap(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents
