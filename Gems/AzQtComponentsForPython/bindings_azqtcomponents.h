/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once
/*
#include <QPointer>

//#include <AzCore/RTTI/TypeInfo.h>
//#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/chrono/chrono.h>
*/

//#include <AzQtComponents/Components/AutoCustomWindowDecorations.h>

// Ensure the correct AZ_VE_NUM_ARGS macro is used inside variadic.h
#define FORCE_VA_OPT_ARG 1
///////////////////////
#include <AzQtComponents/Components/ButtonDivider.h>
#include <AzQtComponents/Components/ButtonStripe.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/DockBar.h>
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/DockTabWidget.h>

#include <AzQtComponents/Components/ExtendedLabel.h>    
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/FancyDockingDropZoneWidget.h>
#include <AzQtComponents/Components/FancyDockingGhostWidget.h>
#include <AzQtComponents/Components/FlowLayout.h>
//#include <AzQtComponents/Components/ui_FilteredSearchWidget.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/GlobalEventFilter.h>
#include <AzQtComponents/Components/HelpButton.h>
#include <AzQtComponents/Components/InteractiveWindowGeometryChanger.h>
#include <AzQtComponents/Components/SearchLineEdit.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/StyledDialog.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/StyledLineEdit.h>
#include <AzQtComponents/Components/StyledSpinBox.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Components/TagSelector.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>

#include <AzQtComponents/Components/ToolBarArea.h>
#include <AzQtComponents/Components/ToolButtonComboBox.h>
#include <AzQtComponents/Components/ToolButtonLineEdit.h>
#include <AzQtComponents/Components/ToolButtonWithWidget.h>

#include <AzQtComponents/Components/VectorEdit.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>


#include <AzQtComponents/Components/Widgets/AssetFolderListView.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/ColorLabel.h>
//#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Widgets/DialogButtonBox.h>
#include <AzQtComponents/Components/Widgets/DragAndDrop.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/Eyedropper.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
// #include <AzQtComponents/Components/Widgets/LogicalTabOrderingWidget.h>
#include <AzQtComponents/Components/Widgets/Menu.h>
#include <AzQtComponents/Components/Widgets/MessageBox.h>
#include <AzQtComponents/Components/Widgets/OverlayWidget.h>

#include <AzQtComponents/Components/Widgets/ProgressBar.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>
#include <AzQtComponents/Components/Widgets/RadioButton.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>
#include <AzQtComponents/Components/Widgets/SegmentBar.h>
#include <AzQtComponents/Components/Widgets/SegmentControl.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/StatusBar.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <AzQtComponents/Components/Widgets/TabWidgetActionToolBar.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <AzQtComponents/Components/Widgets/ToolButton.h>
#include <AzQtComponents/Components/Widgets/TreeView.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorComponentSliders.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
//#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorHexEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorPreview.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorRGBAEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorWarning.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/GammaEdit.h>
// #include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
// #include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCard.h>
// #include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCardCollection.h>
//#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>