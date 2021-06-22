#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    BaseLibrary.h
    BaseLibraryItem.h
    UsedResources.h
    UIEnumsDatabase.h
    Include/EditorCoreAPI.cpp
    Include/IErrorReport.h
    Include/IBaseLibraryManager.h
    Include/IFileUtil.h
    Include/EditorCoreAPI.h
    Include/IEditorMaterial.h
    Include/IEditorMaterialManager.h
    Include/IImageUtil.h
    Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.qrc
    Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.cpp
    Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h
    Controls/ReflectedPropertyControl/ReflectedPropertyItem.cpp
    Controls/ReflectedPropertyControl/ReflectedPropertyItem.h
    Controls/ReflectedPropertyControl/ReflectedVar.cpp
    Controls/ReflectedPropertyControl/ReflectedVar.h
    Controls/ReflectedPropertyControl/ReflectedVarWrapper.cpp
    Controls/ReflectedPropertyControl/ReflectedVarWrapper.h
    Controls/QBitmapPreviewDialog.cpp
    Controls/QBitmapPreviewDialog.h
    Controls/QBitmapPreviewDialog.ui
    Controls/QBitmapPreviewDialogImp.cpp
    Controls/QBitmapPreviewDialogImp.h
    Controls/QToolTipWidget.h
    Controls/QToolTipWidget.cpp
    BaseLibraryItem.cpp
    BaseLibrary.cpp
    UsedResources.cpp
    UIEnumsDatabase.cpp
    LyViewPaneNames.h
    QtViewPaneManager.cpp
    QtViewPaneManager.h
    ErrorRecorder.cpp
    ErrorRecorder.h
    ErrorDialog.cpp
    ErrorDialog.h
    ErrorDialog.ui
    Clipboard.cpp
    Util/MemoryBlock.cpp
    Util/Variable.cpp
    Util/UndoUtil.cpp
    Util/VariablePropertyType.cpp
    Clipboard.h
    Util/MemoryBlock.h
    Util/Variable.h
    Util/UndoUtil.h
    Util/VariablePropertyType.h
    Util/RefCountBase.h
    Util/PathUtil.h
    Util/PathUtil.cpp
    Util/ImageHistogram.cpp
    Util/Image.cpp
    Util/ImageHistogram.h
    Util/Image.h
    Util/ColorUtils.cpp
    Util/ColorUtils.h
    Undo/Undo.cpp
    Undo/IUndoManagerListener.h
    Undo/IUndoObject.h
    Undo/Undo.h
    Undo/UndoVariableChange.h
    WinWidgetId.h
    QtUI/ColorButton.cpp
    QtUI/ColorButton.h
    QtUtil.h
    QtUtilWin.h
    QtViewPane.h
)
