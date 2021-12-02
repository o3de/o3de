/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Definition of basic Editor object.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_BASEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_BASEOBJECT_H
#pragma once


#if !defined(Q_MOC_RUN)
#include "Include/HitContext.h"
#include "ClassDesc.h"
#include "DisplayContext.h"
#include "ObjectLoader.h"
#include "Util/Variable.h"

#include "AzCore/Math/Guid.h"
#endif

//////////////////////////////////////////////////////////////////////////
// forward declarations.
class CUndoBaseObject;
class CObjectManager;
class CGizmo;
class CObjectArchive;
struct SSubObjSelectionModifyContext;
struct SRayHitInfo;
class CPopupMenuItem;
class QMenu;
struct IRenderNode;
struct IStatObj;

//////////////////////////////////////////////////////////////////////////
typedef _smart_ptr<CBaseObject> CBaseObjectPtr;
typedef std::vector<CBaseObjectPtr> TBaseObjects;

//////////////////////////////////////////////////////////////////////////
/*!
This class used for object references remapping during cloning operation.
*/
class CObjectCloneContext
{
public:
    //! Add cloned object.
    SANDBOX_API void AddClone(CBaseObject* pFromObject, CBaseObject* pToObject);

    //! Find cloned object for given object.
    SANDBOX_API CBaseObject* FindClone(CBaseObject* pFromObject);

    // Find id of the cloned object.
    GUID ResolveClonedID(REFGUID guid);

private:
    typedef std::map<CBaseObject*, CBaseObject*> ObjectsMap;
    ObjectsMap m_objectsMap;
};

//////////////////////////////////////////////////////////////////////////
enum EObjectChangedOpType
{
    eOCOT_Empty = 0,
    eOCOT_Modify,
    eOCOT_ModifyTransform,
    eOCOT_ModifyTransformInLibOnly,
    eOCOT_Add,
    eOCOT_Delete,
    eOCOT_Count
};

struct SObjectChangedContext
{
    GUID m_modifiedObjectGlobalId; //! The object id globaly unique and used in ObjectManager
    EObjectChangedOpType m_operation; //! What was the operation on the modified object
    Matrix34 m_localTM; //! If we are in modify transform case this is the local TM info

    SObjectChangedContext(EObjectChangedOpType optype)
        : m_modifiedObjectGlobalId(GUID_NULL)
        , m_operation(optype) { m_localTM.SetIdentity(); }
    SObjectChangedContext()
        : m_modifiedObjectGlobalId(GUID_NULL)
        , m_operation(eOCOT_Empty) { m_localTM.SetIdentity(); }
};

//////////////////////////////////////////////////////////////////////////
enum ObjectFlags
{
    OBJFLAG_SELECTED    = 0x0001,   //!< Object is selected. (Do not set this flag explicitly).
    OBJFLAG_HIDDEN      = 0x0002,   //!< Object is hidden.
    OBJFLAG_FROZEN      = 0x0004,   //!< Object is frozen (Visible but cannot be selected)
    OBJFLAG_FLATTEN     = 0x0008,   //!< Flatten area around object.
    OBJFLAG_SHARED      = 0x0010,   //!< This object is shared between missions.

    OBJFLAG_KEEP_HEIGHT = 0x0040,   //!< This object should try to preserve height when snapping to flat objects.

    OBJFLAG_NO_HITTEST  = 0x0080,   //!< This object will be not a target of ray hit test for deep selection mode.
    OBJFLAG_IS_PARTICLE = 0x0100,
    // object is in editing mode.
    OBJFLAG_EDITING     = 0x01000,
    OBJFLAG_ATTACHING   = 0x02000,  //!< Object in attaching to group mode.
    OBJFLAG_DELETED     = 0x04000,  //!< This object is deleted.
    OBJFLAG_HIGHLIGHT   = 0x08000,  //!< Object is highlighted (When mouse over).
    OBJFLAG_INVISIBLE       = 0x10000,  //!< This object is invisible.
    OBJFLAG_SUBOBJ_EDITING = 0x20000,   //!< This object is in the sub object editing mode.

    OBJFLAG_SHOW_ICONONTOP = 0x100000,  //!< Icon will be drawn on top of the object.
    OBJFLAG_HIDE_HELPERS = 0x200000,    //!< Helpers will be hidden.
    OBJFLAG_DONT_SAVE   = 0x400000, //!< Object will not be saved with editor xml data.

    OBJFLAG_PERSISTMASK = OBJFLAG_HIDDEN | OBJFLAG_FROZEN | OBJFLAG_FLATTEN,
};

#define ERF_GET_WRITABLE(flags) (flags)

//////////////////////////////////////////////////////////////////////////
//! This flags passed to CBaseObject::BeginEditParams method.
enum ObjectEditFlags
{
    OBJECT_CREATE   = 0x001,
    OBJECT_EDIT     = 0x002,
    OBJECT_COLLAPSE_OBJECTPANEL = 0x004
};

//////////////////////////////////////////////////////////////////////////
//! Return values from CBaseObject::MouseCreateCallback method.
enum MouseCreateResult
{
    MOUSECREATE_CONTINUE = 0,   //!< Continue placing this object.
    MOUSECREATE_ABORT,              //!< Abort creation of this object.
    MOUSECREATE_OK,                     //!< Accept this object.
};

//////////////////////////////////////////////////////////////////////////
// Interface to the object create with the mouse callback.
//////////////////////////////////////////////////////////////////////////
struct IMouseCreateCallback
{
    virtual void Release() = 0;
    virtual MouseCreateResult OnMouseEvent(CViewport* view, EMouseEvent event, QPoint& point, int flags) = 0;
    // Some process of creation need to be able to be displayed such as creation for custom solid.
    virtual void Display([[maybe_unused]] DisplayContext& dc){}
    // Called after accepting an object to see if new object creation mode should be continued.
    virtual bool ContinueCreation() = 0;
};

// Flags used for object interaction
enum EObjectUpdateFlags
{
    eObjectUpdateFlags_UserInput                = 0x00001,
    eObjectUpdateFlags_PositionChanged  = 0x00002,
    eObjectUpdateFlags_RotationChanged  = 0x00004,
    eObjectUpdateFlags_ScaleChanged         = 0x00008,
    eObjectUpdateFlags_DoNotInvalidate  = 0x00100, // Do not cause InvalidateTM call.
    eObjectUpdateFlags_ParentChanged        = 0x00200, // When parent transformation change.
    eObjectUpdateFlags_Undo                         = 0x00400, // When doing undo operation.
    eObjectUpdateFlags_RestoreUndo          = 0x00800, // When doing RestoreUndo operation (This is different from normal undo).
    eObjectUpdateFlags_Animated                 = 0x01000, // When doing animation.
    eObjectUpdateFlags_MoveTool                 = 0x02000, // Transformation changed by the move tool
    eObjectUpdateFlags_ScaleTool                = 0x04000, // Transformation changed by the scale tool
    eObjectUpdateFlags_UserInputUndo        = 0x20000, // Undo operation related to user input rather than actual Undo
};

#define OBJECT_TEXTURE_ICON_SIZEX 32
#define OBJECT_TEXTURE_ICON_SIZEY 32
#define OBJECT_TEXTURE_ICON_SCALE 10.0f

enum EScaleWarningLevel
{
    eScaleWarningLevel_None,
    eScaleWarningLevel_Rescaled,
    eScaleWarningLevel_RescaledNonUniform,
};

enum ERotationWarningLevel
{
    eRotationWarningLevel_None = 0,
    eRotationWarningLevel_Rotated,
    eRotationWarningLevel_RotatedNonRectangular,
};

// Used for external control of object position without changing the object's real position (e.g. TrackView)
class ITransformDelegate
{
public:
    // Called when matrix got invalidatd
    virtual void MatrixInvalidated() = 0;

    // Returns current delegated transforms, base transform is passed for delegates
    // that need it, e.g. for overriding only X
    virtual Vec3 GetTransformDelegatePos(const Vec3& basePos) const = 0;
    virtual Quat GetTransformDelegateRotation(const Quat& baseRotation) const = 0;
    virtual Vec3 GetTransformDelegateScale(const Vec3& baseScale) const = 0;

    // Sets the delegate transform.
    virtual void SetTransformDelegatePos(const Vec3& position) = 0;
    virtual void SetTransformDelegateRotation(const Quat& rotation) = 0;
    virtual void SetTransformDelegateScale(const Vec3& scale) = 0;

    // If those return true the base object uses its own transform instead
    virtual bool IsPositionDelegated() const = 0;
    virtual bool IsRotationDelegated() const = 0;
    virtual bool IsScaleDelegated() const = 0;
};

/*!
 *  CBaseObject is the base class for all objects which can be placed in map.
 *  Every object belongs to class specified by ClassDesc.
 *  Specific object classes must override this class, to provide specific functionality.
 *  Objects are reference counted and only destroyed when last reference to object
 *  is destroyed.
 *
 */
class SANDBOX_API CBaseObject
    : public CVarObject
{
    Q_OBJECT
public:
    //! Events sent by object to EventListeners
    enum EObjectListenerEvent
    {
        ON_DELETE = 0,// Sent after object was deleted from object manager.
        ON_ADD,       // Sent after object was added to object manager.
        ON_SELECT,      // Sent when objects becomes selected.
        ON_UNSELECT,    // Sent when objects unselected.
        ON_TRANSFORM, // Sent when object transformed.
        ON_VISIBILITY, // Sent when object visibility changes.
        ON_RENAME,      // Sent when object changes name.
        ON_CHILDATTACHED, // Sent when object gets a child attached.
        ON_PREDELETE, // Sent before an object is processed to be deleted from the object manager.
        ON_CHILDDETACHED, // Sent when the object gets a child detached.
        ON_DETACHFROMPARENT, // Sent when the object detaches from a parent.
        ON_PREATTACHED, // Sent when this object is about to get attached and is already in relative space
        ON_PREATTACHEDKEEPXFORM, // Sent when this object is about to get attached and needs to stay in place
        ON_ATTACHED, // Sent when this object got attached
        ON_PREDETACHED, // Sent when this object is about to get detached and is already in relative space
        ON_PREDETACHEDKEEPXFORM, // Sent when this object is about to get detached and needs to stay in place
        ON_DETACHED, // Sent when this object got detached
        ON_PREFAB_CHANGED, // Sent when prefab representation has been changed
    };

    //! This callback will be called if object is deleted.
    struct EventListener
    {
        virtual void OnObjectEvent(CBaseObject*, int) = 0;
    };

    //! Childs structure.
    typedef std::vector<_smart_ptr<CBaseObject> > Childs;

    //! Retrieve class description of this object.
    CObjectClassDesc*   GetClassDesc() const { return m_classDesc; };

    static bool IsEnabled() { return true; }

    /** Check if both object are of same class.
    */
    virtual bool IsSameClass(CBaseObject* obj);
    virtual void SetDefaultType() { m_objType = OBJTYPE_DUMMY; };
    virtual ObjectType GetType() const
    {
        if (m_objType == OBJTYPE_DUMMY)
        {
            return m_objType;
        }
        else
        {
            return m_classDesc->GetObjectType();
        }
    };
    //  const char* GetTypeName() const { return m_classDesc->ClassName(); };
    QString GetTypeName() const;
    virtual QString GetTypeDescription() const { return m_classDesc->ClassName(); };

    //////////////////////////////////////////////////////////////////////////
    // Flags.
    //////////////////////////////////////////////////////////////////////////
    void SetFlags(int flags) { m_flags |= flags; };
    void ClearFlags(int flags) { m_flags &= ~flags; };
    bool CheckFlags(int flags) const { return (m_flags & flags) != 0; };

    //////////////////////////////////////////////////////////////////////////
    // Hidden ID
    //////////////////////////////////////////////////////////////////////////
    static const uint64 s_invalidHiddenID = 0;
    uint64 GetHideOrder() const { return m_hideOrder; }
    void SetHideOrder(uint64 newID) { m_hideOrder = newID; }

    //! Returns true if object hidden.
    bool IsHidden() const;
    //! Check against min spec.
    bool IsHiddenBySpec() const;
    //! Returns true if object frozen.
    virtual bool IsFrozen() const;
    //! Returns true if object is shared between missions.
    bool IsShared() const { return CheckFlags(OBJFLAG_SHARED); }

    //! Returns true if object is selected.
    virtual bool IsSelected() const { return CheckFlags(OBJFLAG_SELECTED); }
    //! Returns true if object can be selected.
    virtual bool IsSelectable() const;

    // Return texture icon.
    bool HaveTextureIcon() const { return m_nTextureIcon != 0; };
    int GetTextureIcon() const { return m_nTextureIcon; }
    void SetTextureIcon(int nTexIcon) { m_nTextureIcon = nTexIcon; }

    //! Set shared between missions flag.
    virtual void SetShared(bool bShared);
    //! Set object hidden status.
    virtual void SetHidden(bool bHidden, uint64 hiddenId = CBaseObject::s_invalidHiddenID, bool bAnimated = false);
    //! Set object frozen status.
    virtual void SetFrozen(bool bFrozen);
    //! Set object selected status.
    virtual void SetSelected(bool bSelect);
    //! Return associated 3DEngine render node
    virtual IRenderNode* GetEngineNode() const { return nullptr; };
    //! Set object highlighted (Note: not selected)
    virtual void SetHighlight(bool bHighlight);
    //! Check if object is highlighted.
    bool IsHighlighted() const { return CheckFlags(OBJFLAG_HIGHLIGHT); }
    //! Check if object can have measurement axises.
    virtual bool HasMeasurementAxis() const {   return true;    }
    //! Check if the object is isolated when the editor is in Isolation Mode
    virtual bool IsIsolated() const { return false; }

    // Tooltip is rendered in CObjectMode, when you hover the object
    virtual QString GetTooltip() const { return QString(); }

    //////////////////////////////////////////////////////////////////////////
    // Object Id.
    //////////////////////////////////////////////////////////////////////////
    //! Get unique object id.
    //! Every object will have its own unique id assigned.
    REFGUID GetId() const { return m_guid; };

    //////////////////////////////////////////////////////////////////////////
    // Name.
    //////////////////////////////////////////////////////////////////////////
    //! Get name of object.
    const QString& GetName() const;
    virtual QString GetComment() const { return QString(); }
    virtual QString GetWarningsText() const;

    //! Change name of object.
    virtual void SetName(const QString& name);
    //! Set object name and make sure it is unique.
    void SetUniqueName(const QString& name);
    //! Generate unique object name based on a base name (e.g. class name)
    virtual void GenerateUniqueName();

    //////////////////////////////////////////////////////////////////////////
    // Geometry.
    //////////////////////////////////////////////////////////////////////////
    //! Set object position.
    virtual bool SetPos(const Vec3& pos, int flags = 0);

    //! Set object rotation angles.
    virtual bool SetRotation(const Quat& rotate, int flags = 0);

    //! Set object scale.
    virtual bool SetScale(const Vec3& scale, int flags = 0);

    //! Get object position.
    const Vec3 GetPos() const;

    //! Get object local rotation quaternion.
    const Quat GetRotation() const;

    //! Get object scale.
    const Vec3 GetScale() const;

    virtual bool StartScaling()                                                         {   return false;   }
    virtual bool GetUntransformedScale([[maybe_unused]] Vec3& scale) const {   return false;   }
    virtual bool TransformScale([[maybe_unused]] const Vec3& scale)                {   return false;   }

    //! Set flatten area.
    void SetArea(float area);
    float GetArea() const { return m_flattenArea; };

    //! Assign display color to the object.
    virtual void    ChangeColor(const QColor& color);
    //! Get object color.
    QColor GetColor() const { return m_color; };

    // Set current transform delegate. Pass nullptr to unset.
    virtual void SetTransformDelegate(ITransformDelegate* pTransformDelegate);
    ITransformDelegate* GetTransformDelegate() const { return m_pTransformDelegate; }

    //////////////////////////////////////////////////////////////////////////
    // CHILDS
    //////////////////////////////////////////////////////////////////////////

    //! Return true if node have childs.
    bool HaveChilds() const { return !m_childs.empty(); }
    //! Return true if have attached childs.
    size_t GetChildCount() const { return m_childs.size(); }

    //! Get child by index.
    CBaseObject* GetChild(size_t const i) const;
    //! Return parent node if exist.
    CBaseObject* GetParent() const { return m_parent; };
    //! Scans hierarchy up to determine if we child of specified node.
    virtual bool IsChildOf(CBaseObject* node);
    //! Clone Children
    void CloneChildren(CBaseObject* pFromObject);
    //! Attach new child node.
    //! @param bKeepPos if true Child node will keep its world space position.
    virtual void AttachChild(CBaseObject* child, bool bKeepPos = true);
    //! Attach new child node when the object is not a sort of a group object like AttachChild()
    //! but if the object is a group object, the group object should be set to all children objects recursively.
    //! and if the object is a prefab object, the prefab object should be loaded from the prefabitem.
    //! @param bKeepPos if true Child node will keep its world space position.
    virtual void AddMember(CBaseObject* pMember, bool bKeepPos = true);
    //! Detach all childs of this node.
    virtual void DetachAll(bool bKeepPos = true);
    // Detach this node from parent.
    virtual void DetachThis(bool bKeepPos = true);
    // Returns the link parent.
    virtual CBaseObject* GetLinkParent() const { return GetParent(); }

    //////////////////////////////////////////////////////////////////////////
    // MATRIX
    //////////////////////////////////////////////////////////////////////////
    //! Get objects' local transformation matrix.
    Matrix34 GetLocalTM() const { Matrix34 tm; CalcLocalTM(tm); return tm; };

    //! Get objects' world-space transformation matrix.
    const Matrix34& GetWorldTM() const;

    // Gets matrix of parent attachment point
    virtual Matrix34 GetParentAttachPointWorldTM() const;

    // Checks if the attachment point is valid
    virtual bool IsParentAttachmentValid() const;

    //! Set position in world space.
    virtual void SetWorldPos(const Vec3& pos, int flags = 0);

    //! Get position in world space.
    Vec3 GetWorldPos() const { return GetWorldTM().GetTranslation(); };
    Ang3 GetWorldAngles() const;

    //! Set xform of object given in world space.
    virtual void SetWorldTM(const Matrix34& tm, int flags = 0);

    //! Set object xform.
    virtual void SetLocalTM(const Matrix34& tm, int flags = 0);

    // Set object xform.
    virtual void SetLocalTM(const Vec3& pos, const Quat& rotate, const Vec3& scale, int flags = 0);

    //////////////////////////////////////////////////////////////////////////
    // Interface to be implemented in plugins.
    //////////////////////////////////////////////////////////////////////////

    //! Called when object is being created (use GetMouseCreateCallback for more advanced mouse creation callback).
    virtual int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    // Return pointer to the callback object used when creating object by the mouse.
    // If this function return nullptr MouseCreateCallback method will be used instead.
    virtual IMouseCreateCallback* GetMouseCreateCallback() { return nullptr; };

    //! Draw object to specified viewport.
    virtual void Display([[maybe_unused]] DisplayContext& disp) {}

    //! Perform intersection testing of this object.
    //! Return true if was hit.
    virtual bool HitTest([[maybe_unused]] HitContext& hc) { return false; };

    //! Perform intersection testing of this object with rectangle.
    //! Return true if was hit.
    virtual bool HitTestRect(HitContext& hc);

    //! Perform intersection testing of this object based on its icon helper.
    //! Return true if was hit.
    virtual bool HitHelperTest(HitContext& hc);

    //! Get bounding box of object in world coordinate space.
    virtual void GetBoundBox(AABB& box);

    //! Get bounding box of object in local object space.
    virtual void GetLocalBounds(AABB& box);

    //! Called after some parameter been modified.
    virtual void SetModified(bool boModifiedTransformOnly);

    //! Called when visibility of this object changes.
    //! Derived class may override this to respond to new visibility setting.
    virtual void UpdateVisibility(bool bVisible);

    //! Serialize object to/from xml.
    //! @param xmlNode XML node to load/save serialized data to.
    //! @param bLoading true if loading data from xml.
    //! @param bUndo true if loading or saving data for Undo/Redo purposes.
    virtual void Serialize(CObjectArchive& ar);

    //// Pre load called before serialize after all objects where completly loaded.
    //virtual void PreLoad( CObjectArchive &ar ) {};
    // Post load called after all objects where completely loaded.
    virtual void PostLoad([[maybe_unused]] CObjectArchive& ar) {};

    //! Export object to xml.
    //! Return created object node in xml.
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    //! Handle events received by object.
    //! Override in derived classes, to handle specific events.
    virtual void OnEvent(ObjectEvent event);

    //! Generate dynamic context menu for the object
    virtual void OnContextMenu(QMenu* menu);

    //////////////////////////////////////////////////////////////////////////
    // LookAt Target.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetLookAt(CBaseObject* target);
    CBaseObject* GetLookAt() const { return m_lookat; };
    //! Returns true if this object is a look-at target.
    bool IsLookAtTarget() const;
    CBaseObject* GetLookAtSource() const { return m_lookatSource; };


    IObjectManager* GetObjectManager() const;

    //! Store undo information for this object.
    void StoreUndo(const char* undoDescription, bool minimal = false, int flags = 0);

    //! Add event listener callback.
    void AddEventListener(EventListener* listener);
    //! Remove event listener callback.
    void RemoveEventListener(EventListener* listener);

    //////////////////////////////////////////////////////////////////////////
    //! Analyze errors for this object.
    virtual void Validate(IErrorReport* report);

    //////////////////////////////////////////////////////////////////////////
    //! Gather resources of this object.
    virtual void GatherUsedResources(CUsedResources& resources);

    //////////////////////////////////////////////////////////////////////////
    //! Check if specified object is very similar to this one.
    virtual bool IsSimilarObject(CBaseObject* pObject);

    //////////////////////////////////////////////////////////////////////////
    // Material Layers Mask.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetMaterialLayersMask(uint32 nLayersMask) { m_nMaterialLayersMask = nLayersMask; }
    uint32 GetMaterialLayersMask() const { return m_nMaterialLayersMask; };

    //////////////////////////////////////////////////////////////////////////
    // Object minimal usage spec (All/Low/Medium/High)
    //////////////////////////////////////////////////////////////////////////
    uint32 GetMinSpec() const { return m_nMinSpec; }
    virtual void SetMinSpec(uint32 nSpec, bool bSetChildren = true);

    //////////////////////////////////////////////////////////////////////////
    // SubObj selection.
    //////////////////////////////////////////////////////////////////////////
    // Return true if object support selecting of this sub object element type.
    virtual bool StartSubObjSelection([[maybe_unused]] int elemType) { return false; };
    virtual void EndSubObjectSelection() {};
    virtual void ModifySubObjSelection([[maybe_unused]] SSubObjSelectionModifyContext& modCtx) {};
    virtual void AcceptSubObjectModify() {};

    //! In This function variables of the object must be initialized.
    virtual void InitVariables() {};

    //////////////////////////////////////////////////////////////////////////
    // Procedural Floor Management.
    //////////////////////////////////////////////////////////////////////////
    int GetFloorNumber() const { return m_floorNumber; };
    void SetFloorNumber(int floorNumber) { m_floorNumber = floorNumber; };

    virtual void OnPropertyChanged(IVariable*);
    virtual void OnMultiSelPropertyChanged(IVariable*);

    //! Draw a reddish highlight indicating its budget usage.
    virtual void DrawBudgetUsage(DisplayContext& dc, const QColor& color);

    bool IntersectRayMesh(const Vec3& raySrc, const Vec3& rayDir, SRayHitInfo& outHitInfo) const;

    virtual void EditTags([[maybe_unused]] bool alwaysTag) {}
    virtual bool SupportsEditTags() const { return false; }

    bool CanBeHightlighted() const;
    bool IsSkipSelectionHelper() const;

    virtual IStatObj* GetIStatObj() {   return nullptr; }

    // Invalidates cached transformation matrix.
    // nWhyFlags - Flags that indicate the reason for matrix invalidation.
    virtual void InvalidateTM(int nWhyFlags);

protected:
    friend class CObjectManager;

    //! Ctor is protected to restrict direct usage.
    CBaseObject();
    //! Dtor is protected to restrict direct usage.
    virtual ~CBaseObject();

    //! Initialize Object.
    //! If previous object specified it must be of exactly same class as this object.
    //! All data is copied from previous object.
    //! Optional file parameter specify initial object or script for this object.
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);

    //////////////////////////////////////////////////////////////////////////
    //! Must be called after cloning the object on clone of object.
    //! This will make sure object references are cloned correctly.
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);

    //! Must be implemented by derived class to create game related objects.
    virtual bool CreateGameObject() { return true; };

    //! If true, all attached chilren will be cloned when the parent object is cloned.
    virtual bool ShouldCloneChildren() const { return true; }

    /** Called when object is about to be deleted.
            All Game resources should be freed in this function.
    */
    virtual void Done();

    /** Change current id of object.
    */
    //virtual void SetId( uint32 objectId ) { m_id = objectId; };

    //! Call this to delete an object.
    virtual void DeleteThis() = 0;

    //! Called when object need to be converted from different object.
    virtual bool ConvertFromObject(CBaseObject* object);

    //! Called when local transformation matrix is calculated.
    void CalcLocalTM(Matrix34& tm) const;

    //! Called when child position changed.
    virtual void OnChildModified() {};

    //! Remove child from our childs list.
    virtual void RemoveChild(CBaseObject* node);

    //! Resolve parent from callback.
    void ResolveParent(CBaseObject* object);
    void SetColor(const QColor& color);

    //! Draw default object items.
    virtual void DrawDefault(DisplayContext& dc, const QColor& labelColor = QColor(255, 255, 255));
    //! Draw object label.
    void    DrawLabel(DisplayContext& dc, const Vec3& pos, const QColor& labelColor = QColor(255, 255, 255), float alpha = 1.0f, float size = 1.f);
    //! Draw 3D Axis at object position.
    void    DrawAxis(DisplayContext& dc, const Vec3& pos, float size);
    //! Draw area around object.
    void    DrawArea(DisplayContext& dc);
    //! Draw selection helper.
    void    DrawSelectionHelper(DisplayContext& dc, const Vec3& pos, const QColor& labelColor = QColor(255, 255, 255), float alpha = 1.0f);
    //! Draw helper icon.
    virtual void DrawTextureIcon(DisplayContext& dc, const Vec3& pos, float alpha = 1.0f);
    //! Draw warning icons
    virtual void DrawWarningIcons(DisplayContext& dc, const Vec3& pos);
    //! Check if dimension's figures can be displayed before draw them.
    virtual void DrawDimensions(DisplayContext& dc, AABB* pMergedBoundBox = nullptr);

    //! Draw highlight.
    virtual void DrawHighlight(DisplayContext& dc);

    //! Returns if the object can be drawn, and if its selection helper should also be drawn.
    bool CanBeDrawn(const DisplayContext& dc, bool& outDisplaySelectionHelper) const;

    //! Returns if object is in the camera view.
    virtual bool IsInCameraView(const CCamera& camera);
    //! Returns vis ratio of object in camera
    virtual float GetCameraVisRatio(const CCamera& camera);

    // Do basic intersection tests
    virtual bool IntersectRectBounds(const AABB& bbox);
    virtual bool IntersectRayBounds(const Ray& ray);

    // Do hit testing on specified bounding box.
    // Function can be used by derived classes.
    bool HitTestRectBounds(HitContext& hc, const AABB& box);

    // Do helper hit testing as specific location.
    bool HitHelperAtTest(HitContext& hc, const Vec3& pos);

    // Do helper hit testing taking child objects into account (e.g. opened prefab)
    virtual bool HitHelperTestForChildObjects([[maybe_unused]] HitContext& hc) { return false; }

    CBaseObject* FindObject(REFGUID id) const;

    // Returns true if game objects should be created.
    bool IsCreateGameObjects() const;

    // Helper gizmo functions.
    void AddGizmo(CGizmo* gizmo);
    void RemoveGizmo(CGizmo* gizmo);

    //! Notify all listeners about event.
    void NotifyListeners(EObjectListenerEvent event);

    //! Only used by ObjectManager.
    bool IsPotentiallyVisible() const;

    //////////////////////////////////////////////////////////////////////////
    // May be overridden in derived classes to handle helpers scaling.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetHelperScale([[maybe_unused]] float scale) {};
    virtual float GetHelperScale() { return 1.0f; };

    void SetNameInternal(const QString& name) { m_name = name; }

    void SetDrawTextureIconProperties(DisplayContext& dc, const Vec3& pos, float alpha = 1.0f, int texIconFlags = 0);
    const Vec3& GetTextureIconDrawPos(){ return m_vDrawIconPos; };
    int GetTextureIconFlags(){ return m_nIconFlags; };

    Matrix33 GetWorldRotTM() const;
    Matrix33 GetWorldScaleTM() const;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //! World space object's position.
    Vec3 m_pos;
    //! Object's Rotation angles.
    Quat m_rotate;
    //! Object's scale value.
    Vec3 m_scale;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

private:
    friend class CUndoBaseObject;
    friend class CObjectArchive;
    friend class CSelectionGroup;

    void OnMenuShowInAssetBrowser();

    //! Set class description for this object,
    //! Only called once after creation by ObjectManager.
    void SetClassDesc(CObjectClassDesc* classDesc);

    EScaleWarningLevel GetScaleWarningLevel() const;
    ERotationWarningLevel GetRotationWarningLevel() const;

    // auto resolving
    void OnMtlResolved(uint32 id, bool success, const char* orgName, const char* newName);

    bool IsInSelectionBox() const { return m_bInSelectionBox; }

    void SetId(REFGUID guid) { m_guid = guid; }

    // Before translating, rotating or scaling, we ask our subclasses for whether
    // they want us to notify CGameEngine of the upcoming change of our AABB.
    virtual bool ShouldNotifyOfUpcomingAABBChanges() const { return false; }

    // Notifies the CGameEngine about an upcoming change of our AABB.
    void OnBeforeAreaChange();

    //////////////////////////////////////////////////////////////////////////
    // PRIVATE FIELDS
    //////////////////////////////////////////////////////////////////////////
private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //Default ObjType
    ObjectType m_objType;

    //! Unique object Id.
    GUID m_guid;

    // floor number of object if procedural object flag is set
    int m_floorNumber;

    //! Flags of this object.
    int m_flags;

    // Id of the texture icon for this object.
    int m_nTextureIcon;

    //! Display color.
    QColor m_color;

    //! World transformation matrix of this object.
    mutable Matrix34 m_worldTM;

    //////////////////////////////////////////////////////////////////////////
    //! Look At target entity.
    _smart_ptr<CBaseObject> m_lookat;
    //! If we are lookat target. this is pointer to source.
    CBaseObject* m_lookatSource;

    //////////////////////////////////////////////////////////////////////////
    //! Area radius around object, where terrain is flatten and static objects removed.
    float m_flattenArea;
    //! Object's name.
    QString m_name;
    //! Class description for this object.
    CObjectClassDesc*   m_classDesc;

    //! Number of reference to this object.
    //! When reference count reach zero, object will delete itself.
    int m_numRefs;

    int m_nIconFlags;

    //////////////////////////////////////////////////////////////////////////
    //! Child animation nodes.
    Childs m_childs;
    //! Pointer to parent node.
    mutable CBaseObject* m_parent;

    AABB m_worldBounds;

    // The transform delegate
    ITransformDelegate* m_pTransformDelegate;

    //////////////////////////////////////////////////////////////////////////
    // Listeners.
    std::vector<EventListener*> m_eventListeners;

    //////////////////////////////////////////////////////////////////////////
    // Flags and bit masks.
    //////////////////////////////////////////////////////////////////////////
    mutable uint32 m_bMatrixInWorldSpace : 1;
    mutable uint32 m_bMatrixValid : 1;
    mutable uint32 m_bWorldBoxValid : 1;
    uint32 m_bInSelectionBox : 1;
    uint32 m_nMaterialLayersMask : 8;
    uint32 m_nMinSpec : 8;

    Vec3 m_vDrawIconPos;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    uint64 m_hideOrder;
};

Q_DECLARE_METATYPE(CBaseObject*)

#endif // CRYINCLUDE_EDITOR_OBJECTS_BASEOBJECT_H
