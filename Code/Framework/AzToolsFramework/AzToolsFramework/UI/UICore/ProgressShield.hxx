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
        AZ_CLASS_ALLOCATOR(ProgressShield, AZ::SystemAllocator, 0);
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
