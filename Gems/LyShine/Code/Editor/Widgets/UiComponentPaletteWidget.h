/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteWidget.hxx>

//! Subclass of the standard ComponentPaletteWidget that overrides component addition
//! to use LyShine's entity management and undo system instead of the standard
//! EntityCompositionRequestBus.
class UiComponentPaletteWidget : public AzToolsFramework::ComponentPaletteWidget
{
    Q_OBJECT

public:
    UiComponentPaletteWidget(QWidget* parent);

protected slots:
    //! Override to add components through LyShine's undo system
    void ActivateSelection(const QModelIndex& index) override;
};
