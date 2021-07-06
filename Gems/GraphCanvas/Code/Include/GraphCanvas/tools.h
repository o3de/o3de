/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QDebug>
#include <QPoint>
#include <QString>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>

namespace AZ
{
    class Component;
    class Entity;
    class Vector2;
}

QDebug operator<<(QDebug debug, const AZ::Entity* entity);
QDebug operator<<(QDebug debug, const AZ::EntityId& entity);
QDebug operator<<(QDebug debug, const AZ::Component* component);
QDebug operator<<(QDebug debug, const AZ::Vector2& position);

namespace GraphCanvas
{
    static const int GraphicsItemName = 0;

    namespace Tools
    {
        inline QString qStringFromUtf8(const AZStd::string& native)
        {
            return QString::fromUtf8(native.data(), static_cast<int>(native.size()));
        }

        inline AZStd::string utf8FromqString(const QString& qt)
        {
            const QByteArray utf8 = qt.toUtf8();
            return AZStd::string(utf8.constData(), utf8.size());
        }

        inline bool IsClose(const QPointF& left, const QPointF& right)
        {
            return AZ::Vector2(aznumeric_cast<float>(left.x()), aznumeric_cast<float>(left.y())).IsClose(AZ::Vector2(aznumeric_cast<float>(right.x()), aznumeric_cast<float>(right.y())));
        }

        inline bool IsClose(qreal left, qreal right, float tolerance)
        {
            return AZ::IsClose(static_cast<float>(left), static_cast<float>(right), tolerance);
        }

    } // namespace Tools
} // namespace GraphCanvas
