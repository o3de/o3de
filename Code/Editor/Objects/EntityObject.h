/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_OBJECTS_ENTITYOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_ENTITYOBJECT_H
#pragma once


#if !defined(Q_MOC_RUN)
#include "BaseObject.h"

#include "IMovieSystem.h"
#include "Gizmo.h"

#include <QObject>
#endif

#define CLASS_LIGHT "Light"
#define CLASS_DESTROYABLE_LIGHT "DestroyableLight"
#define CLASS_RIGIDBODY_LIGHT "RigidBodyLight"
#define CLASS_ENVIRONMENT_LIGHT "EnvironmentLight"

class CEntityObject;
class QMenu;

/*!
 *  CEntityEventTarget is an Entity event target and type.
 */
struct CEntityEventTarget
{
    CBaseObject* target; //! Target object.
    _smart_ptr<CGizmo> pLineGizmo;
    QString event;
    QString sourceEvent;
};

//////////////////////////////////////////////////////////////////////////
// Named link from entity to entity.
//////////////////////////////////////////////////////////////////////////
struct CEntityLink
{
    GUID targetId;   // Target entity id.
    CEntityObject* target; // Target entity.
    QString name;    // Name of the link.
    _smart_ptr<CGizmo> pLineGizmo;
};

struct IPickEntitesOwner
{
    virtual void AddEntity(CBaseObject* pEntity) = 0;
    virtual CBaseObject* GetEntity(int nIdx) = 0;
    virtual int GetEntityCount() = 0;
    virtual void RemoveEntity(int nIdx) = 0;
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
/*!
 *  CEntity is an static object on terrain.
 *
 */
class CRYEDIT_API CEntityObject
    : public CBaseObject
    , public CBaseObject::EventListener
{
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT
public:
    ~CEntityObject();

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////

    bool Init(IEditor* ie, CBaseObject* prev, const QString& file) override;
    void InitVariables() override;
    void Done() override;

    void DrawExtraLightInfo (DisplayContext& disp);

    bool GetEntityPropertyBool(const char* name) const;
    int GetEntityPropertyInteger(const char* name) const;
    float GetEntityPropertyFloat(const char* name) const;
    QString GetEntityPropertyString(const char* name) const;
    void SetEntityPropertyBool(const char* name, bool value);
    void SetEntityPropertyInteger(const char* name, int value);
    void SetEntityPropertyFloat(const char* name, float value);
    void SetEntityPropertyString(const char* name, const QString& value);

    void SetName(const QString& name) override;
    void SetSelected(bool bSelect) override;

    void GetLocalBounds(AABB& box) override;

    bool HitTest(HitContext& hc) override;
    bool HitTestRect(HitContext& hc) override;
    void UpdateVisibility(bool bVisible) override;
    bool ConvertFromObject(CBaseObject* object) override;

    using CBaseObject::Serialize;
    void Serialize(CObjectArchive& ar) override;
    void PostLoad(CObjectArchive& ar) override;

    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) override;

    //////////////////////////////////////////////////////////////////////////
    void OnEvent(ObjectEvent event) override;

    void SetTransformDelegate(ITransformDelegate* pTransformDelegate) override;

    // Set attach flags and target
    enum EAttachmentType
    {
        eAT_Pivot,
        eAT_CharacterBone,
    };

    void SetAttachType(const EAttachmentType attachmentType) { m_attachmentType = attachmentType; }
    void SetAttachTarget(const char* target) { m_attachmentTarget = target; }
    EAttachmentType GetAttachType() const { return m_attachmentType; }
    QString GetAttachTarget() const { return m_attachmentTarget; }

    void GatherUsedResources(CUsedResources& resources) override;
    bool IsSimilarObject(CBaseObject* pObject) override;

    bool IsIsolated() const override { return false; }

    //////////////////////////////////////////////////////////////////////////
    // END CBaseObject
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // CEntity interface.
    //////////////////////////////////////////////////////////////////////////
    virtual void DeleteEntity() {}

    QString GetEntityClass() const { return m_entityClass; };
    int GetEntityId() const { return m_entityId; };

    //////////////////////////////////////////////////////////////////////////
    //! Return number of event targets of Script.
    int     GetEventTargetCount() const { return static_cast<int>(m_eventTargets.size()); };
    CEntityEventTarget& GetEventTarget(int index) { return m_eventTargets[index]; };
    //! Add new event target, returns index of created event target.
    //! Event targets are Always entities.
    int AddEventTarget(CBaseObject* target, const QString& event, const QString& sourceEvent, bool bUpdateScript = true);
    //! Remove existing event target by index.
    void RemoveEventTarget(int index, bool bUpdateScript = true);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Entity Links.
    //////////////////////////////////////////////////////////////////////////
    //! Return number of event targets of Script.
    int     GetEntityLinkCount() const { return static_cast<int>(m_links.size()); };
    CEntityLink& GetEntityLink(int index) { return m_links[index]; };
    virtual int AddEntityLink(const QString& name, GUID targetEntityId);
    virtual bool EntityLinkExists(const QString& name, GUID targetEntityId);
    void RenameEntityLink(int index, const QString& newName);
    void RemoveEntityLink(int index);
    void RemoveAllEntityLinks();
    virtual void EntityLinked([[maybe_unused]] const QString& name, [[maybe_unused]] GUID targetEntityId){}
    virtual void EntityUnlinked([[maybe_unused]] const QString& name, [[maybe_unused]] GUID targetEntityId) {}
    void LoadLink(XmlNodeRef xmlNode, CObjectArchive* pArchive = nullptr);
    void SaveLink(XmlNodeRef xmlNode);
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    int GetCastShadowMinSpec() const { return mv_castShadowMinSpec; }

    float GetRatioLod() const { return static_cast<float>(mv_ratioLOD); };
    float GetViewDistanceMultiplier() const { return mv_viewDistanceMultiplier; }

    CVarBlock* GetProperties() const { return m_pProperties; };
    CVarBlock* GetProperties2() const { return m_pProperties2; };

    bool IsLight()  const   {   return m_bLight;        }

    void Validate(IErrorReport* report) override;

    // Find CEntity from AZ::EntityId, which can also handle legacy game Ids stored as AZ::EntityIds
    static CEntityObject* FindFromEntityId(const AZ::EntityId& id);

    // Get the name of the light animation node assigned to this, if any.
    QString GetLightAnimation() const;

    IVariable* GetLightVariable(const char* name) const;

    void PreInitLightProperty();
    void UpdateLightProperty();

    void EnableReload(bool bEnable)
    {
        m_bEnableReload = bEnable;
    }

    static void StoreUndoEntityLink(CSelectionGroup* pGroup);

protected:
    template <typename T>
    void SetEntityProperty(const char* name, T value);
    template <typename T>
    T GetEntityProperty(const char* name, T defaultvalue) const;

    //! Draw default object items.
    void DrawProjectorPyramid(DisplayContext& dc, float dist);
    void DrawProjectorFrustum(DisplayContext& dc, Vec2 size, float dist);

    void OnLoadFailed();

    CVarBlock* CloneProperties(CVarBlock* srcProperties);

    //////////////////////////////////////////////////////////////////////////
    void OnObjectEvent(CBaseObject* target, int event) override;
    void ResolveEventTarget(CBaseObject* object, unsigned int index);
    void ReleaseEventTargets();

public:
    CEntityObject();

    static const GUID& GetClassID()
    {
        // {C80F8AEA-90EF-471f-82C7-D14FA80B9203}
        static const GUID guid = {
            0xc80f8aea, 0x90ef, 0x471f, { 0x82, 0xc7, 0xd1, 0x4f, 0xa8, 0xb, 0x92, 0x3 }
        };
        return guid;
    }

protected:
    void DeleteThis() override { delete this; };

    //////////////////////////////////////////////////////////////////////////
    // Radius callbacks.
    //////////////////////////////////////////////////////////////////////////
    void OnRadiusChange(IVariable* var);
    void OnInnerRadiusChange(IVariable* var);
    void OnOuterRadiusChange(IVariable* var);
    void OnBoxSizeXChange(IVariable* var);
    void OnBoxSizeYChange(IVariable* var);
    void OnBoxSizeZChange(IVariable* var);
    void OnProjectorFOVChange(IVariable* var);
    void OnProjectorTextureChange(IVariable* var);
    void OnProjectInAllDirsChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////
    // Area light callbacks.
    //////////////////////////////////////////////////////////////////////////
    void OnAreaLightChange(IVariable* var);
    void OnAreaWidthChange(IVariable* var);
    void OnAreaHeightChange(IVariable* var);
    void OnAreaFOVChange(IVariable* var);
    void OnAreaLightSizeChange(IVariable* var);
    void OnColorChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////
    // Box projection callbacks.
    //////////////////////////////////////////////////////////////////////////
    void OnBoxProjectionChange(IVariable* var);
    void OnBoxWidthChange(IVariable* var);
    void OnBoxHeightChange(IVariable* var);
    void OnBoxLengthChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////

    void FreeGameData();

    void AdjustLightProperties(CVarBlockPtr& properties, const char* pSubBlock);
    IVariable* FindVariableInSubBlock(CVarBlockPtr& properties, IVariable* pSubBlockVar, const char* pVarName);

    unsigned int m_bLoadFailed : 1;
    unsigned int m_bCalcPhysics : 1;
    unsigned int m_bDisplayBBox : 1;
    unsigned int m_bDisplaySolidBBox : 1;
    unsigned int m_bDisplayAbsoluteRadius : 1;
    unsigned int m_bDisplayArrow : 1;
    unsigned int m_bIconOnTop : 1;
    unsigned int m_bVisible : 1;
    unsigned int m_bLight : 1;
    unsigned int m_bAreaLight : 1;
    unsigned int m_bProjectorHasTexture : 1;
    unsigned int m_bProjectInAllDirs : 1;
    unsigned int m_bBoxProjectedCM : 1;
    unsigned int m_bBBoxSelection : 1;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Vec3  m_lightColor;

    //! Entity class.
    QString m_entityClass;
    //! Id of spawned entity.
    int m_entityId;

    // Used for light entities
    float m_projectorFOV;

    AABB m_box;

    //////////////////////////////////////////////////////////////////////////
    // Main entity parameters.
    //////////////////////////////////////////////////////////////////////////
    CVariable<bool> mv_outdoor;
    CVariable<bool> mv_castShadow; // Legacy, required for backwards compatibility
    CSmartVariableEnum<int> mv_castShadowMinSpec;
    CVariable<int> mv_ratioLOD;
    CVariable<float> mv_viewDistanceMultiplier;
    CVariable<bool> mv_hiddenInGame; // Entity is hidden in game (on start).
    CVariable<bool> mv_recvWind;
    CVariable<bool> mv_renderNearest;
    CVariable<bool> mv_noDecals;
    CVariable<bool> mv_createdThroughPool;
    CVariable<float> mv_obstructionMultiplier;

    //////////////////////////////////////////////////////////////////////////
    // Temp variables (Not serializable) just to display radii from properties.
    //////////////////////////////////////////////////////////////////////////
    // Used for proximity entities.
    float m_proximityRadius;
    float m_innerRadius;
    float m_outerRadius;
    // Used for probes
    float m_boxSizeX;
    float m_boxSizeY;
    float m_boxSizeZ;
    // Used for area lights
    float m_fAreaWidth;
    float m_fAreaHeight;
    float m_fAreaLightSize;
    // Used for box projected cubemaps
    float m_fBoxWidth;
    float m_fBoxHeight;
    float m_fBoxLength;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Event Targets.
    //////////////////////////////////////////////////////////////////////////
    //! Array of event targets of this Entity.
    typedef std::vector<CEntityEventTarget> EventTargets;
    EventTargets m_eventTargets;

    //////////////////////////////////////////////////////////////////////////
    // Links
    typedef std::vector<CEntityLink> Links;
    Links m_links;

    //! Entity properties variables.
    CVarBlockPtr m_pProperties;

    //! Per instance entity properties variables
    CVarBlockPtr m_pProperties2;

    // Physics state, as a string.
    XmlNodeRef m_physicsState;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    EAttachmentType m_attachmentType;

    bool m_bEnableReload;

    QString m_attachmentTarget;

private:
    struct VariableCallbackIndex
    {
        enum : unsigned char
        {
            OnAreaHeightChange = 0,
            OnAreaLightChange,
            OnAreaLightSizeChange,
            OnAreaWidthChange,
            OnBoxHeightChange,
            OnBoxLengthChange,
            OnBoxProjectionChange,
            OnBoxSizeXChange,
            OnBoxSizeYChange,
            OnBoxSizeZChange,
            OnBoxWidthChange,
            OnColorChange,
            OnInnerRadiusChange,
            OnOuterRadiusChange,
            OnProjectInAllDirsChange,
            OnProjectorFOVChange,
            OnProjectorTextureChange,
            OnPropertyChange,
            OnRadiusChange,

            // must be at the end
            Count,
        };
    };

    void ResetCallbacks();
    void SetVariableCallback(IVariable* pVar, IVariable::OnSetCallback* func);
    void ClearCallbacks();

    void ForceVariableUpdate();

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    std::vector< std::pair<IVariable*, IVariable::OnSetCallback*> > m_callbacks;
    AZStd::fixed_vector< IVariable::OnSetCallback, VariableCallbackIndex::Count > m_onSetCallbacksCache;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_ENTITYOBJECT_H
