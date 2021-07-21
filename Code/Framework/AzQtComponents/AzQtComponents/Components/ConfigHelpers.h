/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QFile>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QString>

#include <functional>

namespace AzQtComponents
{
    namespace ConfigHelpers
    {
        /* The aim of the *Config.ini files is to provide a human read/writable file which allows
         * developers and designers to tweak UI settings without having to rebuild the application.
         * Types that QVariant can convert to QString are stored as one might expect:
         *
         * MyBool=true
         * MyInt=5
         * MyDecimal=3.14
         * MyString=A stored string
         *
         * QColor is not normally stored by QSettings as a human readable string, it is encoded as a
         * QVariant, but the following syntax works with the QVariant::value<QColor> call:
         *
         * MyColor=#ffffff
         *
         * Many types that QVariant cannot convert to QString, such as QPoint, QRect and QSize, can
         * be specified as follows:
         *
         * MyPoint=@Point(12 16)
         * MyRect=@Rect(0 0 16 16)
         * MySize=@Size(4 4)
         *
         * However there are some types which QVariant does not store in a human readable manner, or
         * cannot store at all. For these we provide template specialisations. For example, QPixmap
         * can be read as follows:
         *
         * QPixmap pixmap;
         * ConfigHelpers::read<QPixmap>(settings, QStringLiteral("MyPixmap"), pixmap);
         *
         * QPixmap is specified like this:
         *
         * MyPixmap=path/to/image.png
         */
        template <class T>
        void read(QSettings& settings, const QString& key, T& configValue)
        {
            // Sets configValue to the value of key in settings. If key does not exist, configValue
            // is unchanged.
            configValue = settings.value(key, QVariant::fromValue(configValue)).template value<T>();
        }

        template <>
        void read(QSettings& settings, const QString& key, QPixmap& configValue);

        template <>
        void read(QSettings& settings, const QString& key, QCursor& configValue);

        /* ConfigHelpers::loadConfig loads the ConfigType from a QSettings IniFormat file and
         * watches that file for further changes. When changes occur, the notify function is called.
         * Note that in Qt terminology, the notify function can be either a signal or a slot.
         *
         * Note that in case it does not go without saying, the config pointer must be valid as long
         * as the watcher is, otherwise a file change on disk will result in a memory access
         * violation.
         *
         * ConfigType is simply a struct containing the configuration options:
         *
         * struct Config
         * {
         *     int height = -1;
         * };
         *
         * This function expects WidgetType to have the following static functions:
         *
         * static Config loadConfig(QSettings& settings);
         * static Config defaultConfig();
         */
        template <typename ConfigType, typename WidgetType>
        void loadConfig(QFileSystemWatcher* watcher, ConfigType* config, const QString& path, const QObject* context, const std::function<void()>& notify)
        {
            if (QFile::exists(path))
            {
                // add to the file watcher
                watcher->addPath(path);

                // connect the relead slot()
                QObject::connect(watcher, &QFileSystemWatcher::fileChanged, context, [path, config, notify](const QString& changedPath) {
                    if (changedPath == path)
                    {
                        QSettings settings(path, QSettings::IniFormat);
                        *config = WidgetType::loadConfig(settings);

                        Q_EMIT notify();
                    }
                });

                QSettings settings(path, QSettings::IniFormat);
                *config = WidgetType::loadConfig(settings);
            }
            else
            {
                *config = WidgetType::defaultConfig();
            }
        }

        /* GroupGuard ensures that QSettings::endGroup is called when it is destroyed.
         */
        class GroupGuard
        {
        public:
            GroupGuard(QSettings* settings, const QString& prefix)
                : m_settings(settings)
            {
                m_settings->beginGroup(prefix);
            }

            ~GroupGuard()
            {
                m_settings->endGroup();
            }

        private:
            QSettings* m_settings;
        };
    } // namespace ConfigHelpers
} // namespace AzQtComponents
