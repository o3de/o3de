/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/function/function_fwd.h>

#include <QWidget>
#endif

namespace Ui
{
    class progressShield;
}

namespace AzToolsFramework
{
    class ProgressShield
        : public QWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(ProgressShield, AZ::SystemAllocator);
        ProgressShield(QWidget* pParent);
        ~ProgressShield() override;

        // For porting legacy systems, have the progress shield display like a dialog until "completeCallback" returns true
        // Prefer fully async implementations
        static void LegacyShowAndWait(QWidget* pParent, QString label, AZStd::function<bool(int& current, int& max)> completeCallback, int delayMS = 0);

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;
    public slots:
        void setProgress(int current, int max, const QString& label);
        void UpdateGeometry();

    private:
        Ui::progressShield* uiConstructor;
    };
} // namespace AzToolsFramework
