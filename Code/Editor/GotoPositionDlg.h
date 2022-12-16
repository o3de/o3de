/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

#include <SandboxAPI.h>

#include <AzCore/std/functional.h>

namespace Ui
{
    class GotoPositionDialog;
}

//! Utility to deal with ensuring camera pitch values are in the expected range.
struct GoToPositionPitchConstraints
{
    using AngleRangeConfigureFn = AZStd::function<void(float, float)>;
    //! Notify a callback with the min and max camera pitch constraints (no tolerance included).
    SANDBOX_API void DeterminePitchRange(const AngleRangeConfigureFn& configurePitchRangeFn) const;
    //! Returns the clamped pitch value (including tolerance with range extents).
    SANDBOX_API float PitchClampedRadians(float pitchDegrees) const;
};

//! GotoPositionDialog for setting camera position and rotation.
class GotoPositionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GotoPositionDialog(QWidget* parent = nullptr);
    ~GotoPositionDialog();

protected:
    void OnInitDialog();
    void accept() override;

    void OnUpdateNumbers();
    void OnChangeEdit();

public:
    QString m_transform;

private:
    GoToPositionPitchConstraints m_goToPositionPitchConstraints;
    QScopedPointer<Ui::GotoPositionDialog> m_ui;
};
