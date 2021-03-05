/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#pragma once

#if !defined(Q_MOC_RUN)
#include <typeinfo>
#include <algorithm>
#include "Serialization/Serializer.h"
#include "Serialization/StringList.h"
#include <Serialization/Pointers.h>
#include "Factory.h"
#include "ConstStringList.h"
#include "Strings.h"
#include "../EditorCommonAPI.h"
#include "Serialization/ClassFactory.h"

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QCursor>
#endif

namespace Serialization {    struct ICallback; }

class QWidget;
class QFont;
class QPainter;
class QMenu;
class QKeyEvent;

using std::vector;
class QPropertyTree;
class PropertyRow;
class PropertyTreeModel;
class PopupMenuItem;
struct PropertyDrawContext;
struct EDITOR_COMMON_API ContainerMenuHandler;
class PropertyRowContainer;

enum ScanResult {
    SCAN_FINISHED, 
    SCAN_CHILDREN, 
    SCAN_SIBLINGS, 
    SCAN_CHILDREN_SIBLINGS,
};

struct EDITOR_COMMON_API PropertyRowMenuHandler : QObject
{
public:
    virtual ~PropertyRowMenuHandler() {}

};

struct PropertyActivationEvent
{
    enum Reason
    {
        REASON_PRESS,
        REASON_RELEASE,
        REASON_DOUBLECLICK,
        REASON_KEYBOARD,
        REASON_NEW_ELEMENT
    };

    QPropertyTree* tree;
    Reason reason;
    bool force;
    QPoint clickPoint;

    PropertyActivationEvent()
    : force(false)
    , clickPoint(0, 0)
    , tree(0)
    , reason(REASON_PRESS)
    {
    }
};

struct PropertyDragEvent
{
    QPropertyTree* tree;
    QPoint pos;
    QPoint start;
    QPoint lastDelta;
    QPoint totalDelta;
};

struct PropertyHoverInfo
{
    QCursor cursor;
    QString toolTip;

    PropertyHoverInfo()
    : cursor()
    {
    }
};

enum DragCheckBegin {
    DRAG_CHECK_IGNORE,
    DRAG_CHECK_SET,
    DRAG_CHECK_UNSET
};

class PropertyRowWidget : public QObject
{
    Q_OBJECT
public:
    PropertyRowWidget(PropertyRow* row, QPropertyTree* tree);
    virtual ~PropertyRowWidget();
    virtual QWidget* actualWidget() { return 0; }
    virtual void showPopup() {}
    virtual void commit() = 0;
    PropertyRow* row() { return row_; }
    PropertyTreeModel* model() { return model_; }
protected:
    PropertyRow* row_;
    QPropertyTree* tree_;
    PropertyTreeModel* model_;
};

class PropertyTreeTransaction;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class EDITOR_COMMON_API PropertyRow : public Serialization::RefCounter
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    enum WidgetPlacement {
        WIDGET_NONE,
        WIDGET_ICON,
        WIDGET_AFTER_NAME,
        WIDGET_VALUE,
        WIDGET_AFTER_PULLED,
        WIDGET_INSTEAD_OF_TEXT
    };

    typedef std::vector< Serialization::SharedPtr<PropertyRow> > Rows;
    typedef Rows::iterator iterator;
    typedef Rows::const_iterator const_iterator;

    PropertyRow();
    virtual ~PropertyRow();

    void setNames(const char* name, const char* label, const char* typeName);

    bool selected() const{ return selected_; }
    void setSelected(bool selected) { selected_ = selected; }
    bool expanded() const{ return expanded_; }
    void _setExpanded(bool expanded); // use QPropertyTree::expandRow
    void setExpandedRecursive(QPropertyTree* tree, bool expanded);

    void setMatchFilter(bool matchFilter) { matchFilter_ = matchFilter; }
    bool matchFilter() const { return matchFilter_; }

    void setBelongsToFilteredRow(bool belongs) { belongsToFilteredRow_ = belongs; }
    bool belongsToFilteredRow() const { return belongsToFilteredRow_; }

    bool visible(const QPropertyTree* tree) const;
    bool hasVisibleChildren(const QPropertyTree* tree, bool internalCall = false) const;

    const PropertyRow* hit(const QPropertyTree* tree, QPoint point) const;
    PropertyRow* hit(const QPropertyTree* tree, QPoint point);
    PropertyRow* parent() { return parent_; }
    const PropertyRow* parent() const{ return parent_; }
    void setParent(PropertyRow* row) { parent_ = row; }
    bool isRoot() const { return !parent_; }
    int level() const;

    void add(PropertyRow* row);
    void addAfter(PropertyRow* row, PropertyRow* after);
    void addBefore(PropertyRow* row, PropertyRow* before);

    template<class Op> bool scanChildren(Op& op);
    template<class Op> bool scanChildren(Op& op, QPropertyTree* tree);
    template<class Op> bool scanChildrenReverse(Op& op, QPropertyTree* tree);
    template<class Op> bool scanChildrenBottomUp(Op& op, QPropertyTree* tree);

    PropertyRow* childByIndex(int index);
    const PropertyRow* childByIndex(int index) const;
    int childIndex(const PropertyRow* row) const;
    bool isChildOf(const PropertyRow* row) const;

    bool empty() const{ return children_.empty(); }
    iterator find(PropertyRow* row) { return std::find(children_.begin(), children_.end(), row); }
    PropertyRow* findFromIndex(int* outIndex, const char* name, const char* typeName, int startIndex) const;
    PropertyRow* findByAddress(const void* handle);
    virtual const void* searchHandle() const;
    iterator begin() { return children_.begin(); }
    iterator end() { return children_.end(); }
    const_iterator begin() const{ return children_.begin(); }
    const_iterator end() const{ return children_.end(); }
    std::size_t count() const{ return children_.size(); }
    iterator erase(iterator it){ return children_.erase(it); }
    void clear(){ children_.clear(); }
    void erase(PropertyRow* row);
    void swapChildren(PropertyRow* row, PropertyTreeModel* model);

    void assignRowState(const PropertyRow& row, bool recurse);
    void assignRowProperties(PropertyRow* row);
    void replaceAndPreserveState(PropertyRow* oldRow, PropertyRow* newRow, PropertyTreeModel* model);

    const char* name() const{ return name_; }
    void setName(const char* name) { name_ = name; }
    const char* label() const { return label_; }
    const char* labelUndecorated() const { return labelUndecorated_; }
    void setLabel(const char* label);
    void setLabelChanged();
    void setTooltip(const char* tooltip);
    bool setValidatorEntry(int index, int count);
    int validatorCount() const{ return validatorCount_; }
    int validatorIndex() const{ return validatorIndex_; }
    void resetValidatorIcons();
    void addValidatorIcons(bool hasWarnings, bool hasErrors);
    const char* tooltip() const { return tooltip_; }
    void setLayoutChanged();
    void setLabelChangedToChildren();
    void setLayoutChangedToChildren();
    void setHideChildren(bool hideChildren) { hideChildren_ = hideChildren; }
    bool hideChildren() const { return hideChildren_; }
    void updateLabel(const QPropertyTree* tree, int index, bool parentHidesNonInlineChildren);
    void updateTextSizeInitial(const QPropertyTree* tree, int index, bool force);
    virtual void labelChanged() {}
    void parseControlCodes(const QPropertyTree* tree, const char* label, bool changeLabel);
    const char* typeName() const{ return typeName_; }
    virtual const char* typeNameForFilter(QPropertyTree* tree) const;
    void setTypeName(const char* typeName) { typeName_ = typeName; }
    const char* rowText(char* containerLabelBuffer, size_t bufsiz, const QPropertyTree* tree, int rowIndex) const;

    PropertyRow* findSelected();
    PropertyRow* find(const char* name, const char* nameAlt, const char* typeName);
    const PropertyRow* find(const char* name, const char* nameAlt, const char* typeName) const;
    void intersect(const PropertyRow* row);

    int verticalIndex(QPropertyTree* tree, PropertyRow* row);
    PropertyRow* rowByVerticalIndex(QPropertyTree* tree, int index);
    int horizontalIndex(QPropertyTree* tree, PropertyRow* row);
    PropertyRow* rowByHorizontalIndex(QPropertyTree* tree, int index);

    virtual bool assignToPrimitive([[maybe_unused]] void* object, [[maybe_unused]] size_t size) const{ return false; }
    virtual bool assignTo([[maybe_unused]] const Serialization::SStruct& ser) const{ return false; }
    virtual bool assignToByPointer(void* instance, const Serialization::TypeID& type) const{ return assignTo(Serialization::SStruct(type, instance, type.sizeOf(), 0)); }
    virtual void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) { serializer_ = ser; }
    virtual void handleChildrenChange() {}
    virtual string valueAsString() const;
    virtual wstring valueAsWString() const;

    int height() const{ return size_.y(); }

    virtual int widgetSizeMin(const QPropertyTree*) const { return userWidgetSize() >= 0 ? userWidgetSize() : 0; } 
    virtual int floorHeight() const{ return 0; }

    void calcPulledRows(int* minTextSize, int* freePulledChildren, int* minimalWidth, const QPropertyTree* tree, int index);
    void calculateMinimalSize(const QPropertyTree* tree, int posX, int availableWidth, bool force, int* extraSizeRemainder, int* _extraSize, int index);
    void setTextSize(const QPropertyTree* tree, int rowIndex, float multiplier);
    void calculateTotalSizes(int* minTextSize);
    void adjustVerticalPosition(const QPropertyTree* tree, int& totalHeight);

    virtual bool isWidgetFixed() const{ return userFixedWidget_ || (widgetPlacement() != WIDGET_VALUE && widgetPlacement() != WIDGET_INSTEAD_OF_TEXT); }

    virtual WidgetPlacement widgetPlacement() const{ return WIDGET_NONE; }

    QRect rect() const{ return QRect(pos_.x(), pos_.y(), size_.x(), size_.y()); }
    QRect rectIncludingChildren(const QPropertyTree* tree) const;
    QRect textRect(const QPropertyTree* tree) const;
    QRect widgetRect(const QPropertyTree* tree) const;
    QRect plusRect(const QPropertyTree* tree) const;
    QRect floorRect(const QPropertyTree* tree) const;
    QRect validatorRect(const QPropertyTree* tree) const;
    QRect validatorWarningIconRect(const QPropertyTree* tree) const;
    QRect validatorErrorIconRect(const QPropertyTree* tree) const;
    void adjustHoveredRect(QRect& hoveredRect);
    int heightIncludingChildren() const{ return heightIncludingChildren_; }
    const QFont* rowFont(const QPropertyTree* tree) const;

    void drawRow(QPainter& painter, const QPropertyTree* tree, int rowIndex, bool selectionPass);
    void drawPlus(QPainter& p, const QPropertyTree* tree, const QRect& rect, bool expanded, bool selected, bool grayed) const;
    void drawStaticText(QPainter& p, const QRect& widgetRect);

    virtual void redraw(const PropertyDrawContext& context);
    virtual PropertyRowWidget* createWidget([[maybe_unused]] QPropertyTree* tree) { return 0; }

    virtual bool isContainer() const{ return false; }
    virtual bool isPointer() const{ return false; }
    virtual bool isObject() const{ return false; }

    virtual bool isLeaf() const{ return false; }
    virtual void closeNonLeaf([[maybe_unused]] const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) {}
    virtual bool isStatic() const{ return pulledContainer_ == 0; }
    virtual bool isSelectable() const{ return (!userReadOnly() && !userReadOnlyRecurse()) || (!pulledUp() && !pulledBefore()); }
    virtual bool activateOnAdd() const{ return false; }
    virtual bool inlineInShortArrays() const{ return false; }

    bool canBeToggled(const QPropertyTree* tree) const;
    bool canBeDragged() const;
    bool canBeDroppedOn(const PropertyRow* parentRow, const PropertyRow* beforeChild, const QPropertyTree* tree) const;
    void dropInto(PropertyRow* parentRow, PropertyRow* cursorRow, QPropertyTree* tree, bool before);
    virtual bool getHoverInfo(PropertyHoverInfo* hit, [[maybe_unused]] const QPoint& cursorPos, [[maybe_unused]] const QPropertyTree* tree) const { 
        hit->toolTip = QString::fromUtf8(tooltip_);
        return true; 
    }

    virtual bool onActivate(const PropertyActivationEvent& e);
    virtual bool processesKey(QPropertyTree* tree, const QKeyEvent* ev); // returns true if it wants to process key events; otherwise, they will get processed by shortcuts in some cases, like delete
    virtual bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev);
    virtual bool onMouseDown([[maybe_unused]] QPropertyTree* tree, [[maybe_unused]] QPoint point, [[maybe_unused]] bool& changed) { return false; }
    virtual void onMouseDrag([[maybe_unused]] const PropertyDragEvent& e) {}
    virtual void onMouseStill([[maybe_unused]] const PropertyDragEvent& e) {}
    virtual void onMouseUp([[maybe_unused]] QPropertyTree* tree, [[maybe_unused]] QPoint point) {}
    // "drag check" allows you to "paint" with the mouse through checkboxes to set all values at once
    virtual DragCheckBegin onMouseDragCheckBegin() { return DRAG_CHECK_IGNORE; }
    virtual bool onMouseDragCheck([[maybe_unused]] QPropertyTree* tree, [[maybe_unused]] bool value) { return false; }
    virtual bool onContextMenu(QMenu &menu, QPropertyTree* tree);
    virtual ContainerMenuHandler* createMenuHandler(QPropertyTree* tree, PropertyRowContainer* container);

    virtual bool isFullRow(const QPropertyTree* tree) const;

    // User states.
    // Assigned using control codes (characters in the beginning of label)
    // fixed widget doesn't expand automatically to occupy all available place
    bool userFixedWidget() const{ return userFixedWidget_; }
    bool userFullRow() const { return userFullRow_; }
    void setUserReadOnly(bool userReadOnly) { userReadOnly_ = userReadOnly; }
    virtual bool userReadOnly() const { return userReadOnly_; }
    void propagateFlagsTopToBottom();
    virtual bool userReadOnlyRecurse() const { return userReadOnlyRecurse_; }
    bool userWidgetToContent() const { return userWidgetToContent_; }
    int userWidgetSize() const{ return userWidgetSize_; }
    bool userNonCopyable() const { return userNonCopyable_; }

    // multiValue is used to edit properties of multiple objects simulateneously
    bool multiValue() const { return multiValue_; }
    void setMultiValue(bool multiValue) { multiValue_ = multiValue; }

    // pulledRow - is the one that is pulled up to the parents row
    // (created with ^ in the beginning of label)
    bool pulledUp() const { return pulledUp_; }
    bool pulledBefore() const { return pulledBefore_; }
    bool hasPulled() const { return hasPulled_; }
    bool packedAfterPreviousRow() const { return packedAfterPreviousRow_; }
    bool pulledSelected() const;
    PropertyRow* nonPulledParent();
    void setPulledContainer(PropertyRow* container){ pulledContainer_ = container; }
    PropertyRow* pulledContainer() { return pulledContainer_; }
    const PropertyRow* pulledContainer() const{ return pulledContainer_; }

    Serialization::SharedPtr<PropertyRow> clone(ConstStringList* constStrings) const;

    Serialization::SStruct serializer() const{ return serializer_; }
    virtual Serialization::TypeID typeId() const{ return serializer_.type(); }
    void setSerializer(const Serialization::SStruct& ser) { serializer_ = ser; }
    virtual void serializeValue([[maybe_unused]] Serialization::IArchive& ar) {}
    void setCallback(Serialization::ICallback* callback);
    Serialization::ICallback* callback() { return callback_; }
    virtual void Serialize(Serialization::IArchive& ar);

    static void setConstStrings(ConstStringList* constStrings){ constStrings_ = constStrings; }

protected:
    void init(const char* name, const char* nameAlt, const char* typeName);
    PropertyRow* findChildFromDescendant(PropertyRow* row) const;

    virtual void overrideTextColor([[maybe_unused]] QColor& textColor) {}

    const char* name_;
    const char* label_;
    const char* labelUndecorated_;
    const char* typeName_;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Serialization::SStruct serializer_;
    PropertyRow* parent_;
    Serialization::ICallback* callback_;
    const char* tooltip_;
    Rows children_;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    unsigned int textHash_;

    // do we really need QPoint here? 
    QPoint pos_;
    QPoint size_;
    short int textPos_;
    short int textSizeInitial_;
    short int textSize_;
    short int widgetPos_; // widget == icon!
    short int widgetSize_;
    short int userWidgetSize_;
    unsigned short heightIncludingChildren_;
    unsigned short validatorIndex_;    
    unsigned short validatorsHeight_;
    unsigned char validatorCount_;
    unsigned char plusSize_;
    bool visible_ : 1;
    bool matchFilter_ : 1;
    bool belongsToFilteredRow_ : 1;
    bool expanded_ : 1;
    bool selected_ : 1;
    bool labelChanged_ : 1;
    bool layoutChanged_ : 1;
    bool userReadOnly_ : 1;
    bool userReadOnlyRecurse_ : 1;
    bool userFixedWidget_ : 1;
    bool userFullRow_ : 1;
    bool userPackCheckboxes_ : 1;
    bool userWidgetToContent_ : 1;
    bool pulledUp_ : 1;
    bool pulledBefore_ : 1;
    bool packedAfterPreviousRow_ : 1;
    bool hasPulled_ : 1;
    bool multiValue_ : 1;
    bool hideChildren_ : 1;
    bool validatorHasErrors_ : 1;
    bool validatorHasWarnings_ : 1;
    bool userNonCopyable_ : 1;
    enum class FontWeight
    {
        Undefined,
        Bold,
        Regular
    };
    FontWeight fontWeight_;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Serialization::SharedPtr<PropertyRow> pulledContainer_;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    static ConstStringList* constStrings_;
    friend class PropertyOArchive;
    friend class PropertyIArchive;
};

inline unsigned int calculateHash(const char* str, unsigned hash = 5381)
{
    while(*str)
        hash = hash * 33 + (unsigned char)*str++;
    return hash;
}

template<class T>
inline unsigned int calculateHash(const T& t, unsigned hash = 5381)
{
    for (int i = 0; i < sizeof(T); i++)
        hash = hash * 33 + ((unsigned char*)&t)[i];
    return hash;
}

struct RowWidthCache
{
    unsigned int valueHash;
    int width;

    RowWidthCache() : valueHash(0), width(-1) {}
    int getOrUpdate(const QPropertyTree* tree, const PropertyRow* rowForValue, int extraSpace);
};

typedef vector<Serialization::SharedPtr<PropertyRow> > PropertyRows;

template<bool value>
struct StaticBool{
    enum { Value = value };
};

struct LessStrCmp
{
    bool operator()(const char* a, const char* b) const {
        return strcmp(a, b) < 0;
    }
};

typedef Factory<const char*, PropertyRow, LessStrCmp> PropertyRowFactory;

template<class Op>
bool PropertyRow::scanChildren(Op& op)
{
    Rows::iterator it;

    for(it = children_.begin(); it != children_.end(); ++it){
        ScanResult result = op(*it);
        if(result == SCAN_FINISHED)
            return false;
        if(result == SCAN_CHILDREN || result == SCAN_CHILDREN_SIBLINGS){
            if(!(*it)->scanChildren(op))
                return false;
            if(result == SCAN_CHILDREN)
                return false;
        }
    }
    return true;
}

template<class Op>
bool PropertyRow::scanChildren(Op& op, QPropertyTree* tree)
{
    int numChildren = int(children_.size());
    for(int index = 0; index < numChildren; ++index){
        PropertyRow* child = children_[index];
        ScanResult result = op(child, tree, index);
        if(result == SCAN_FINISHED)
            return false;
        if(result == SCAN_CHILDREN || result == SCAN_CHILDREN_SIBLINGS){
            if(!child->scanChildren(op, tree))
                return false;
            if(result == SCAN_CHILDREN)
                return false;
        }
    }
    return true;
}

template<class Op>
bool PropertyRow::scanChildrenReverse(Op& op, QPropertyTree* tree)
{
    int numChildren = (int)children_.size();
    for(int index = numChildren - 1; index >= 0; --index){
        PropertyRow* child = children_[index];
        ScanResult result = op(child, tree, index);
        if(result == SCAN_FINISHED)
            return false;
        if(result == SCAN_CHILDREN || result == SCAN_CHILDREN_SIBLINGS){
            if(!child->scanChildrenReverse(op, tree))
                return false;
            if(result == SCAN_CHILDREN)
                return false;
        }
    }
    return true;
}

template<class Op>
bool PropertyRow::scanChildrenBottomUp(Op& op, QPropertyTree* tree)
{
    size_t numChildren = children_.size();
    for(size_t i = 0; i < numChildren; ++i)
    {
        PropertyRow* child = children_[i];
        if(!child->scanChildrenBottomUp(op, tree))
            return false;
        ScanResult result = op(child, tree);
        if(result == SCAN_FINISHED)
            return false;
    }
    return true;
}

EDITOR_COMMON_API PropertyRowFactory& GlobalPropertyRowFactory();
EDITOR_COMMON_API Serialization::ClassFactory<PropertyRow>& GlobalPropertyRowClassFactory();

struct PropertyRowPtrSerializer : Serialization::SharedPtrSerializer<PropertyRow>
{
    PropertyRowPtrSerializer(Serialization::SharedPtr<PropertyRow>& ptr) : SharedPtrSerializer(ptr) {}
    Serialization::ClassFactory<PropertyRow>* factory() const override { return &GlobalPropertyRowClassFactory(); }
};

inline bool Serialize(Serialization::IArchive& ar, Serialization::SharedPtr<PropertyRow>& ptr, const char* name, const char* label)
{
    PropertyRowPtrSerializer serializer(ptr);
    return ar(static_cast<Serialization::IPointer&>(serializer), name, label);
}

#define REGISTER_PROPERTY_ROW(DataType, RowType) \
    PropertyRow* _Factory_For_##RowType() {return new RowType; }; \
    REGISTER_IN_FACTORY(PropertyRowFactory, Serialization::TypeID::get<DataType>().name(), RowType, _Factory_For_##RowType); \
    SERIALIZATION_CLASS_NAME_FOR_FACTORY(GlobalPropertyRowClassFactory(), PropertyRow, RowType, #DataType, #DataType);

// Exposes the necessary class factories to extend the property tree
// Exposes the necessary class factories to extend the property tree
EDITOR_COMMON_API Serialization::ClassFactory<PropertyRow>& GetPropertyRowClassFactory();
EDITOR_COMMON_API PropertyRowFactory& GetPropertyRowFactory();
