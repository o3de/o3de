/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <AzCore/Math/Color.h>
#endif

namespace AzQtComponents
{
    namespace Internal
    {
        class ColorController;
    }

    /**
     * Abstract base class for validating and adjusting colors, and providing warnings. Used by the color picker to validate
     */
    class ColorValidator : public QObject
    {
        Q_OBJECT

    public:
        explicit ColorValidator(QObject* parent = nullptr) : QObject(parent) {}

        //! Returns true if the input color is valid, false otherwise. False will trigger adjust to be called.
        virtual bool isValid(const Internal::ColorController* controller) = 0;

        //! Returns color adjusted to something acceptable
        virtual void adjust(Internal::ColorController* controller) = 0;

        //! Must emit colorWarning when called
        virtual void warn() = 0;

        //! Emits the colorAccepted signal
        void acceptColor() { Q_EMIT colorAccepted(); }

    Q_SIGNALS:
        void colorWarning(const QString& message);
        void colorAccepted();
    };

    /**
     * Validates that a color has full alpha in the alpha channel
     */
    class RGBColorValidator : public ColorValidator
    {
    public:
        using ColorValidator::ColorValidator;

        bool isValid(const Internal::ColorController* controller) override;
        void adjust(Internal::ColorController* controller) override;
        void warn() override;
    };

    /**
     * Validates the input colors are in not high dynamic range (i.e. that the colors are in the 0.0 - 1.0 range)
     */
    class RGBALowRangeValidator : public ColorValidator
    {
    public:
        using ColorValidator::ColorValidator;

        bool isValid(const Internal::ColorController* controller) override;
        void adjust(Internal::ColorController* controller) override;
        void warn() override;
    };

    /**
     * Validates the 'value' field of a color is set to the default, as it must be in HueSaturation mode
     */
    class HueSaturationValidator : public ColorValidator
    {
    public:
        explicit HueSaturationValidator(float defaultV, QObject* parent = nullptr);

        bool isValid(const Internal::ColorController* controller) override;
        void adjust(Internal::ColorController* controller) override;
        void warn() override;

    private:
        float m_defaultV;
    };
} // namespace AzQtComponents
