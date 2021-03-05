/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_QPROPERTYTREE_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_QPROPERTYTREE_H
#pragma once



#if !defined(Q_MOC_RUN)
#include "../EditorCommonAPI.h"
#include "ConstStringList.h"
#include "ValidatorBlock.h"
#include <string>
#include "Serialization/Serializer.h"
#include "Serialization/IArchive.h"
#include "Serialization/Pointers.h"
#include "Serialization/Object.h"
#include "PropertyRow.h"
#include <QWidget>
#include <vector>
#endif

namespace Serialization
{
    struct SContextLink;
    class IClassFactory;
}

class QMenu;
class QLineEdit;
class QScrollBar;

struct Color;
class TreeImpl;
class PropertyTreeModel;
class PopupMenuItem;
class PropertyTreeModel;
class PropertyRow;
class PropertyRowWidget;
class PropertyTreeOperator;
class Entry;
struct IconXPMCache;


// ---------------------------------------------------------------------------
struct QPropertyTreeStyle;
struct PropertyTreeConfig
{
    bool immediateUpdate;
    bool hideUntranslated;
    bool showContainerIndices;
    bool showContainerIndexLabels;
    bool containerIndicesZeroBased;
    bool filterWhenType;
    int filter;
    int sliderUpdateDelay;
    int expandLevels;
    bool undoEnabled;
    bool fullUndo;
    bool multiSelection;
    bool copyPasteEnabled;
    
    PropertyTreeConfig()
    : immediateUpdate(true)
    , hideUntranslated(true)
    , showContainerIndices(true)
    , showContainerIndexLabels(false)
    , containerIndicesZeroBased(true)    
    , filterWhenType(true)
    , filter(0)
    , sliderUpdateDelay(25)
    , undoEnabled(true)
    , fullUndo(true)
    , multiSelection(true)
    , copyPasteEnabled(true)
    {
    }
};

// ---------------------------------------------------------------------------

struct PropertyRowMenuHandler;

class DragWindow : public QWidget
{
    Q_OBJECT
public:
    DragWindow(QPropertyTree* tree);

    void set(QPropertyTree* tree, PropertyRow* row, const QRect& rowRect);
    void setWindowPos(bool visible);
    void show();
    void move(int deltaX, int deltaY);
    void hide();

    void drawRow(QPainter& p);
    void paintEvent(QPaintEvent* ev);

protected:
    bool useLayeredWindows_;
    PropertyRow* row_;
    QRect rect_;
    QPropertyTree* tree_;
    QPoint offset_;
};

class EDITOR_COMMON_API QPropertyTree : public QWidget
{
    Q_OBJECT
public:
    explicit QPropertyTree(QWidget* parent = nullptr);
    ~QPropertyTree();

    // Used to attach an object to a PropertyTree widget. Attached object should implement
    // Serialize method. Example of usage:
    //
    //   struct MyType
    //   {
    //     void Serialize(Serialization::IArchive& ar);
    //   };
    //   MyType object;
    //
    //   propertyTree->attach(Serialization::SStruct(object));
    // 
    // Attached object will be serialized through PropertyOArchive to populate the tree. On
    // every input made to the tree attached object will be deserialized through
    // PropertyIArchive and serialized back again through PropertyOArchive to make sure that
    // tree content is up-to-date.
    // Property archives can be identified by calling ar.IsEdit(). 
    // SStruct stores a pointer to an actual object that should either outlive property tree
    // or be detached on destruction.
    void attach(const Serialization::SStruct& serializer);
    // This form attaches an array of SStruct-s. This is used to edit multiple objects
    // simultaneously. Only shared properties will be shown (i.e.  intersection of all
    // properties). Properties with different values will be shown as "..." or a gray
    // checkbox.
    bool attach(const Serialization::SStructs& serializers);
    // Used for two-trees setup. In this case leader tree acts as an outliner, that shows
    // top level of the data (either by using setOutlineMode or setting different filter).
    // Attached, follower tree then shows properties ot the item selected in the main tree.
    // Temporary structures (e.g. created on the stack) should not be used in this mode
    // (except for decorators), as this may cause access to deallocated object when
    // selecting it in the tree.
    void attachPropertyTree(QPropertyTree* propertyTree);
    void detachPropertyTree();
    void setAutoHideAttachedPropertyTree(bool autoHide);
    // Effectively clears the tree.
    void detach();
    bool attached() const { return !attached_.empty(); }

    // Forces serialization of attached object to update properties.  Can be used to update
    // property tree when attached object was changed for some reason.
    void revert();
    // Same as revert(), except that it will but interrupt editing or mouse action in
    // progress.
    void revertNoninterrupting();
    // Forces deserialization of attached objects from property items. 
    void apply(bool continuousUpdate);
    // Useful to apply edit boxes that are being edited at the moment.
    // May be needed when click on toolbar button doesn't steal the focus, leaving input
    // data effectively not saved.
    void applyInplaceEditor();

    // Reduces width of the tree by removing expansion arrow/plus on the first level of the
    // tree (first level is always expanded).
    void setCompact(bool compact);
    bool compact() const;
    // Puts checkboxes into two columns when possible.
    void setPackCheckboxes(bool pack);
    bool packCheckboxes() const;
    // Changes distance between rows, multiplier of row height.
    void setRowSpacing(float rowSpacing);
    float rowSpacing() const;
    // Sets default width of the value column, 0..1 (relative to widget width)
    void setValueColumnWidth(float valueColumnWidth);
    float valueColumnWidth() const;
    // Allows to override background color, useful when placing on tabs or panels that have 
    // have different background.
    void setBackgroundColor(const QColor& color);
    const QColor& backgroundColor() const { return backgroundColor_; }
    // Set number of levels to be expanded by default. Note that this function should be
    // invoked before first call to attach attach() to have an effect.
    void setExpandLevels(int levels);
    // Can be used to control if container(array) elements have numbered labels.
    void setShowContainerIndices(bool showContainerIndices) { config_.showContainerIndices = showContainerIndices; }
    bool showContainerIndices() const{ return config_.showContainerIndices; }
    // Can be used to control if container(array) elements should prepend the number to the existing label
    void setShowContainerIndexLabels(bool showContainerIndexLabels) { config_.showContainerIndexLabels = showContainerIndexLabels; }
    bool showContainerIndexLabels() const{ return config_.showContainerIndexLabels; }
    // Can be used to control if container(array) elements should be zero- (default) or one-based
    void setContainerIndicesZeroBased(bool containerIndicesZeroBased) { config_.containerIndicesZeroBased = containerIndicesZeroBased; }
    bool containerIndicesZeroBased() const{ return config_.containerIndicesZeroBased; }
    // Allows control of copy/paste functionality
    void setCopyPasteEnabled(bool copyPasteEnabled) { config_.copyPasteEnabled = copyPasteEnabled; }
    bool copyPasteEnabled() const{ return config_.copyPasteEnabled; }

    // Limits the rate at which sliders emit change signal.
    void setSliderUpdateDelay(int delayMS) { config_.sliderUpdateDelay = delayMS; }

    // Limit number of mouse-movement updates per-frame. Used to prevent large tree updates
    // from draining all the idle time.
    void setAggregateMouseEvents(bool aggregate) { aggregateMouseEvents_ = aggregate; }
    void flushAggregatedMouseEvents();

    // Can be used to disable internal undo.
    void setUndoEnabled(bool enabled, bool full = false);
    // This can be used to disable automatic serialization after each deserialization call.
    // May be useful to prevent double-revert when signalChange is connected to some
    // external data model, which fires an event that reverts tree automatically.
    void setAutoRevert(bool autoRevert) { autoRevert_ = autoRevert; }
    // Default size.
    void setSizeHint(const QSize& size) { sizeHint_ = size; }
    // Sets minimal size of the widget to the size of the visible content of the tree.
    void setSizeToContent(bool sizeToContent);
    bool sizeToContent() const{ return sizeToContent_; }
    // Retrieves size of the content, doesn't require sizeToContent to be set.
    QSize contentSize() const{ return contentSize_; }
    // When set filtering is started just by typing in the property tree
    void setFilterWhenType(bool filterWhenType) {    config_.filterWhenType = filterWhenType; }

    // Outline mode hides content of the elements of the container (excepted for
    // inlined/pulled-up properties). Can be used together with second property
    // tree through attachPropertyTree.
    void setOutlineMode(bool outlineMode) { outlineMode_ = outlineMode; }
    bool outlineMode() const{ return outlineMode_; }
    // Hide selection when widget is out of focus. Disables selection for parent of inline items.
    void setHideSelection(bool hideSelection) { hideSelection_ = hideSelection; }
    bool hideSelection() const{ return hideSelection_; }
    // Can be used to disable selection of multiple properties at the same time.
    void setMultiSelection(bool multiSelection) { config_.multiSelection = multiSelection; }
    bool multiSelection() const{ return config_.multiSelection; }
    
    // Sets head of the context-list. Can be used to pass additional data to nested decorators.
    void setArchiveContext(Serialization::SContextLink* context) { archiveContext_ = context; }
    // Sets archive filter. Filter is a bit mask stored within archive that can be used to
    // affect behavior of serialization. For example one can have two trees that shows
    // different portions of the same object.
    void setFilter(int filter) { config_.filter = filter; }

    // This methods returns array of SStruct-s for all selected properties.
    // This is useful for manual implementation of attachPropertyTree behavior
    // with type filtering or special logic.
    void getSelectionSerializers(Serialization::SStructs* serializers);
    // Can be used to select once serialized object(s) in the tree.
    bool selectByAddress(const void*, bool keepSelectionIfChildSelected = false);
    bool selectByAddresses(const void* const* addresses, size_t addressCount, bool keepSelectionIfChildSelected);
    // Can be used to query information about selection in the tree.
    bool setSelectedRow(PropertyRow* row);
    PropertyRow* selectedRow();
    int selectedRowCount() const;
    PropertyRow* selectedRowByIndex(int index);

    // Reports if serialized data contains errors. Errors are reported
    // through IArchive::Error() method.
    bool containsErrors() const;
    void focusFirstError();

    void ensureVisible(PropertyRow* row, bool update = true, bool considerChildren = true);
    void expandRow(PropertyRow* row, bool expanded = true, bool updateHeights = true);

    // PropertyTreeStyle used to customize visual appearance of the property tree
    const QPropertyTreeStyle& treeStyle() const{ return *style_; }
    void setTreeStyle(const QPropertyTreeStyle& style);

    // Config used to store behavioral settings
    const PropertyTreeConfig& config() const{ return config_; }

    // Instance of PropertyTree can be serialized. In this case the expansion state
    // of the rows and list of selected rows will be saved (not the property values).
    void Serialize(Serialization::IArchive& ar);

    // OBSOLETE: Serialization::Object will be gone
    void attach(const Serialization::Object& object);
    int revertObjects(vector<void*> objectAddresses);
    bool revertObject(void* objectAddress);
signals:
    // Emited for every finished changed of the value.  E.g. when you drag a slider,
    // signalChanged will be invoked when you release a mouse button.
    void signalChanged();
    // Used fast-pace changes, like movement of the slider before mouse gets released.
    void signalContinuousChange();
    // Invoked whenever selection changed.
    void signalSelected();
    // Invoked after each revert() call (can be caused by user intput).
    void signalReverted();
    // Invoked before any change is going to occur and can be used to store current version
    // of data for own undo stack.
    void signalPushUndo();
    void signalPushRedo();
    
    // Called before and after serialization is invoked. Can be used to update context list
    // in archive.
    void signalAboutToSerialize(Serialization::IArchive& ar);
    void signalSerialized(Serialization::IArchive& ar);

    // OBSOLETE: do not use
    void signalObjectChanged(const Serialization::Object& obj);

    // Called when visual size of the tree changes, i.e. when things are deserialized and
    // and when rows are expanded/collapsed.
    void signalSizeChanged();

    // Called when undo/redo are triggered via keyboard shortcut
    void signalUndo();
    void signalRedo();
public slots:
    void expandAll(PropertyRow* root = 0);
    void collapseAll(PropertyRow* root = 0);
    void onAttachedTreeChanged();
public:
    // internal methods:
    void setFullRowMode(bool fullRowMode);
    bool fullRowMode() const;
    void setHideUntranslated(bool hideUntranslated) { config_.hideUntranslated = hideUntranslated; }
    bool hideUntranslated() const{ return config_.hideUntranslated; }
    void setImmediateUpdate(bool immediateUpdate) { config_.immediateUpdate = immediateUpdate; }
    bool immediateUpdate() const{ return config_.immediateUpdate; }
    int _defaultRowHeight() const { return defaultRowHeight_; }
    PropertyTreeModel* model() { return model_.data(); }
    const PropertyTreeModel* model() const { return model_.data(); }

    QPoint treeSize() const;
    int leftBorder() const { return leftBorder_; }
    int rightBorder() const { return rightBorder_; }
    bool multiSelectable() const { return attachedPropertyTree_ != 0 || config_.multiSelection; }
    void expandParents(PropertyRow* row);
    bool spawnWidget(PropertyRow* row, bool ignoreReadOnly);
    bool getSelectedObject(Serialization::Object* object);
    void onSignalChanged() { signalChanged(); }

    void onRowSelected(const std::vector<PropertyRow*>& row, bool addSelection, bool adjustCursorPos);
    const ValidatorBlock* _validatorBlock() const { return validatorBlock_.data(); }
    QPoint _toScreen(QPoint point) const;
    void _cancelWidget(){ widget_.reset(); }
    void _drawRowLabel(QPainter& p, const wchar_t* text, const QFont* font, const QRect& rect, const QColor& color) const;
    void _drawRowValue(QPainter& p, const wchar_t* text, const QFont* font, const QRect& rect, const QColor& color, bool pathEllipsis, bool center) const;
    QRect _visibleRect() const;
    bool _isDragged(const PropertyRow* row) const;
    bool _isCapturedRow(const PropertyRow* row) const;
    PropertyRow* _pressedRow() const { return pressedRow_; }
    void _setPressedRow(PropertyRow* row) { pressedRow_ = row; }
    int _applyTime() const{ return applyTime_; }
    int _revertTime() const{ return revertTime_; }
    int _updateHeightsTime() const{ return updateHeightsTime_; }
    int _paintTime() const{ return paintTime_; }
    const QFont& _boldFont() const{ return boldFont_; }
    bool hasFocusOrInplaceHasFocus() const;
    void addMenuHandler(PropertyRowMenuHandler* handler);
    IconXPMCache* _iconCache() const{ return iconCache_.data(); }
public slots:
    void onFilterChanged(const QString& str);
protected slots:
    void onScroll(int pos);
    void onModelUpdated(const PropertyRows& rows, bool apply);
    void onModelPushUndo(PropertyTreeOperator* op, bool* handled);
    void onModelPushRedo(PropertyTreeOperator* op, bool* handled);
    void onMouseStillTimeout();

private:
    QPropertyTree(const QPropertyTree&);
    QPropertyTree& operator=(const QPropertyTree&);
protected:
    class DragController;
    enum HitTest{
        TREE_HIT_PLUS,
        TREE_HIT_TEXT,
        TREE_HIT_ROW,
        TREE_HIT_NONE
    };
    PropertyRow* rowByPoint(const QPoint& point);
    HitTest hitTest(PropertyRow* row, const QPoint& pointInWindowSpace, const QRect& rowRect);
    void onRowMenuDecompose(PropertyRow* row);
    void onMouseStill(QPoint point);

    QSize sizeHint() const override;
    bool event(QEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;
    void moveEvent(QMoveEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    void focusInEvent(QFocusEvent* ev) override;

    void updateArea();
    bool toggleRow(PropertyRow* row);

    struct RowFilter {
        enum Type {
            NAME_VALUE,
            NAME,
            VALUE,
            TYPE,
            NUM_TYPES
        };

        string start[NUM_TYPES];
        bool tillEnd[NUM_TYPES];
        std::vector<string> substrings[NUM_TYPES];

        void parse(const char* filter);
        bool match(const char* text, Type type, size_t* matchStart, size_t* matchEnd) const;
        bool typeRelevant(Type type) const{
            return !start[type].empty() || !substrings[type].empty();
        }

        RowFilter()
        {
            for (int i = 0; i < NUM_TYPES; ++i)
                tillEnd[i] = false;
        }
    };

    QPoint pointToRootSpace(const QPoint& pointInWindowSpace) const;
    QPoint pointFromRootSpace(const QPoint& point) const;

    void interruptDrag();
    void updateHeights(bool recalculateTextSize=false);
    void updateValidatorIcons();
    bool updateScrollBar();
    void applyValidation();
    void jumpToNextHiddenValidatorIssue(bool isError, PropertyRow* start);

    bool onContextMenu(PropertyRow* row, QMenu& menu);
    void clearMenuHandlers();
    bool onRowKeyDown(PropertyRow* row, const QKeyEvent* ev);
    bool rowProcessesKey(PropertyRow* row, const QKeyEvent* ev);
    // points here are specified in root-row space
    bool onRowLMBDown(PropertyRow* row, const QRect& rowRect, QPoint point, bool controlPressed, bool shiftPressed);
    void onRowLMBUp(PropertyRow* row, const QRect& rowRect, QPoint point);
    void onRowRMBDown(PropertyRow* row, const QRect& rowRect, QPoint point);
    void onRowMouseMove(PropertyRow* row, const QRect& rowRect, QPoint point);

    bool canBePasted(PropertyRow* destination);
    bool canBePasted(const char* destinationType);

    void setFilterMode(bool inFilterMode);
    void startFilter(const char* filter);
    void setWidget(PropertyRowWidget* widget);
    void _arrangeChildren();

    void updateAttachedPropertyTree(bool revert);
    void drawFilteredString(QPainter& p, const wchar_t* text, RowFilter::Type type, const QFont* font, const QRect& rect, const QColor& color, bool pathEllipsis, bool center) const;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<PropertyTreeModel> model_;
    int cursorX_;

    QScopedPointer<PropertyRowWidget> widget_; // in-place widget
    vector<PropertyRowMenuHandler*> menuHandlers_;

    typedef vector<Serialization::Object> Objects;
    Objects attached_;
    QPropertyTree* attachedPropertyTree_;
    bool autoHideAttachedPropertyTree_;

    bool filterMode_;
    RowFilter rowFilter_;
    QScopedPointer<QLineEdit> filterEntry_; 
    QScopedPointer<IconXPMCache> iconCache_; 
    Serialization::SContextLink* archiveContext_;
    QScopedPointer<ValidatorBlock> validatorBlock_;
    bool outlineMode_;
    bool sizeToContent_;
    bool hideSelection_;

    bool autoRevert_;
    bool needUpdate_;

    QScrollBar* scrollBar_;
    QFont boldFont_;
    QColor backgroundColor_;
    QRect area_;
    int leftBorder_;
    int rightBorder_;
    QPoint size_;
    QPoint offset_;
    QSize sizeHint_;
    QSize contentSize_;
    DragController* dragController_;
    Serialization::SharedPtr<PropertyRow> lastSelectedRow_;
    QPoint pressPoint_;
    QPoint pressDelta_;
    bool pointerMovedSincePress_;
    QPoint lastStillPosition_;
    PropertyRow* capturedRow_;
    PropertyRow* pressedRow_;
    QTimer* mouseStillTimer_;

    bool aggregateMouseEvents_;
    int aggregatedMouseEventCount_;
    QScopedPointer<QMouseEvent> lastMouseMoveEvent_;

    PropertyTreeConfig config_;
    QScopedPointer<QPropertyTreeStyle> style_;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    int defaultRowHeight_;

    int applyTime_;
    int revertTime_;
    int updateHeightsTime_;
    int paintTime_;
    int zoomLevel_;
    bool dragCheckMode_;
    bool dragCheckValue_;

    friend class TreeImpl;
    friend class FilterEntry;
    friend class DragWindow;
    friend struct FilterVisitor;
    friend struct PropertyTreeMenuHandler;
};

wstring generateDigest(Serialization::SStruct& ser);
// vim: tw=90:

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_QPROPERTYTREE_H
