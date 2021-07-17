/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzCore/Math/Color.h>
#include <QWidget>
#include <QString>
#endif

class QLabel;

namespace AzQtComponents
{

    class AZ_QT_COMPONENTS_API ColorWarning
        : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(Mode mode READ mode WRITE setMode)
        Q_PROPERTY(AZ::Color color READ color WRITE setColor)
        Q_PROPERTY(QString message READ message WRITE setMessage)

    public:
        enum class Mode
        {
            Warning,
            Error
        };

        explicit ColorWarning(QWidget* parent = nullptr);
        explicit ColorWarning(Mode mode, const AZ::Color& color, const QString& message, QWidget* parent = nullptr);
        ~ColorWarning() override;

        Mode mode() const;
        void setMode(Mode mode);

        AZ::Color color() const;
        void setColor(const AZ::Color& color);

        const QString& message() const;
        void setMessage(const QString& message);

        void set(Mode mode, const QString& message);
        void clear();

    private:
        Mode m_mode = Mode::Warning;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        AZ::Color m_color;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QString m_message;

        QLabel* m_iconLabel;
        QLabel* m_messageLabel;
    };

} // namespace AzQtComponents
