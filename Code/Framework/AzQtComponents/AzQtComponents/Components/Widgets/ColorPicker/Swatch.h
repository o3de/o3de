/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzCore/Math/Color.h>
#include <QFrame>
#endif

class QPainter;
class QStyleOption;

namespace AzQtComponents
{

    class AZ_QT_COMPONENTS_API Swatch
        : public QFrame
    {
        Q_OBJECT

        Q_PROPERTY(AZ::Color color READ color WRITE setColor NOTIFY colorChanged)

    public:

        explicit Swatch(QWidget* parent = nullptr);
        explicit Swatch(const AZ::Color& color, QWidget* parent = nullptr);
        ~Swatch() override;

        AZ::Color color() const;
        void setColor(const AZ::Color& color);

        bool drawSwatch(const QStyleOption* option, QPainter* painter, const QPoint& targetOffset = {}) const;

    Q_SIGNALS:
        void colorChanged(const AZ::Color& color);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        AZ::Color m_color;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };

} // namespace AzQtComponents
