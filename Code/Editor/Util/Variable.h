/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_VARIABLE_H
#define CRYINCLUDE_EDITOR_UTIL_VARIABLE_H
#pragma once


#include "RefCountBase.h"

#include "Include/EditorCoreAPI.h"

#include <QObject>
AZ_PUSH_DISABLE_WARNING(4458, "-Wunknown-warning-option")
#include <QtUtilWin.h>
AZ_POP_DISABLE_WARNING
#include <QVariant>

#include <StlUtils.h>

inline const char* to_c_str(const char* str) { return str; }
#define MAX_VAR_STRING_LENGTH 4096

struct IVarEnumList;
struct ISplineInterpolator;
class CUsedResources;
struct IVariable;

namespace
{
    template<typename T>
    T nextNiceNumberBelow(T number);

    template<typename T>
    T nextNiceNumberAbove(T number)
    {
        if (number == 0)
        {
            return 0;
        }
        if (number < 0)
        {
            return -nextNiceNumberBelow(-number);
        }

        // number = 8000
        auto l = log10(number);   // 3.90
        AZ::s64 i = aznumeric_cast<AZ::s64>(l); // 3
        int f = aznumeric_cast<int>(pow(10, l - i));   // 10^0.9 = 8
        f = f < 2 ? 2 : f < 5 ? 5 : 10; // f -> 10
        return aznumeric_cast<T>(pow(10, i) * f);
    }

    template<typename T>
    T nextNiceNumberBelow(T number)
    {
        if (number == 0)
        {
            return 0;
        }
        if (number < 0)
        {
            return -nextNiceNumberAbove(-number);
        }

        // number = 8000
        auto l = log10(number);   // 3.90
        AZ::s64 i = aznumeric_cast<AZ::s64>(l); // 3
        int f = aznumeric_cast<int>(pow(10, l - i));   // 10^0.9 = 8
        f = f > 5 ? 5 : f > 2 ? 2 : 1;  // f -> 5
        return aznumeric_cast<T>(pow(10, i) * f);
    }
}

/** IVariableContainer
 *  Interface for all classes that hold child variables
 */
struct IVariableContainer
    : public CRefCountBase
{
    //! Add child variable
    virtual void AddVariable(IVariable* var) = 0;

    //! Delete specific variable
    virtual bool DeleteVariable(IVariable* var, bool recursive = false) = 0;

    //! Delete all variables
    virtual void DeleteAllVariables() = 0;

    //! Gets number of variables
    virtual int GetNumVariables() const = 0;

    //! Get variable at index
    virtual IVariable* GetVariable(int index) const = 0;

    //! Returns true if var block contains specified variable.
    virtual bool IsContainsVariable(IVariable* pVar, bool bRecursive = false) const = 0;

    //! Find variable by name.
    virtual IVariable* FindVariable(const char* name, bool bRecursive = false, bool bHumanName = false) const = 0;

    //! Return true if variable block is empty (Does not have any vars).
    virtual bool IsEmpty() const = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** IVariable is the variant variable interface.
 */
struct IVariable
    : public IVariableContainer
{
    /** Type of data stored in variable.
    */
    enum EType
    {
        UNKNOWN,            //!< Unknown parameter type.
        INT,                //!< Integer property.
        BOOL,               //!< Boolean property.
        FLOAT,              //!< Float property.
        VECTOR2,            //!< Vector property.
        VECTOR,             //!< Vector property.
        VECTOR4,            //!< Vector property.
        QUAT,               //!< Quaternion property.
        STRING,             //!< String property.
        ARRAY,              //!< Array of parameters.
        FLOW_CUSTOM_DATA,   //!< FLow graph custom data
        DOUBLE              //!< Double property.
    };

    //! Type of data hold by variable.
    enum EDataType
    {
        DT_SIMPLE = 0, //!< Standard param type.
        DT_BOOLEAN,
        DT_PERCENT,     //!< Percent data type, (Same as simple but value is from 0-1 and UI will be from 0-100).
        DT_COLOR,
        DT_ANGLE,
        DT_TEXTURE,
        DT_SHADER,
        DT_LOCAL_STRING,
        DT_EQUIP,
        DT_MATERIALLOOKUP,
        DT_EXTARRAY,    // Extendable Array
        DT_SEQUENCE,    // Movie Sequence (DEPRECATED, use DT_SEQUENCE_ID, instead.)
        DT_MISSIONOBJ,  // Mission Objective
        DT_USERITEMCB,  // Use a callback GetItemsCallback in user data of variable
        DT_UIENUM,          // Edit as enum, uses CUIEnumsDatabase to lookup the enum to value pairs and combobox in GUI.
        DT_SEQUENCE_ID, // Movie Sequence
        DT_LIGHT_ANIMATION, // Light Animation Node in the global Light Animation Set
        DT_PARTICLE_EFFECT,
        DT_DEPRECATED, // formerly DT_FLARE
        DT_AUDIO_TRIGGER,
        DT_AUDIO_SWITCH,
        DT_AUDIO_SWITCH_STATE,
        DT_AUDIO_RTPC,
        DT_AUDIO_ENVIRONMENT,
        DT_AUDIO_PRELOAD_REQUEST,
        DT_UI_ELEMENT,
        DT_COLORA,      // DT_COLOR with alpha channel
        DT_MOTION,      // Motion animation asset
        DT_CURVE = BIT(7),  // Combined with other types
    };

    // Flags that can used with variables.
    enum EFlags
    {
        // User interface related flags.
        UI_DISABLED          = BIT(0),  //!< This variable will be disabled in UI.
        UI_BOLD              = BIT(1),  //!< Variable name in properties will be bold.
        UI_SHOW_CHILDREN     = BIT(2),  //!< Display children in parent field.
        UI_USE_GLOBAL_ENUMS  = BIT(3),  //!< Use CUIEnumsDatabase to fetch enums for this variable name.
        UI_INVISIBLE         = BIT(4),  //!< This variable will not be displayed in the UI
        UI_ROLLUP2           = BIT(5),  //!< Prefer right roll-up bar for extended property control.
        UI_COLLAPSED         = BIT(6),  //!< Category collapsed by default.
        UI_UNSORTED          = BIT(7),  //!< Do not sort list-box alphabetically.
        UI_EXPLICIT_STEP     = BIT(8),  //!< Use the step size set in the variable for the UI directly.
        UI_NOT_DISPLAY_VALUE = BIT(9),  //!< The UI will display an empty string where it would normally draw the variable value.
        UI_HIGHLIGHT_EDITED  = BIT(10), //!< Edited (non-default value) properties show highlight color.
        UI_AUTO_EXPAND       = BIT(11), //!< Category expanded by default. to auto-expand child in ReflectedPropertyCtrl.
        UI_CREATE_SPLINE     = BIT(12), //!< To indicate the spline need to be re-created. This is usually because the data was changed directly through the data address
    };

    typedef AZStd::function<void(IVariable*)> OnSetCallback;
    using OnSetEnumCallback = OnSetCallback;

    // Store IGetCustomItems into IVariable's UserData and set datatype to
    // DT_USERITEMCB
    // RefCounting is NOT handled by IVariable!
    struct IGetCustomItems
        : public CRefCountBase
    {
        struct SItem
        {
            SItem() {}
            SItem(const QString& name, const QString& desc = QString())
                : name(name)
                , desc(desc) {}
            SItem(const char* name, const char* desc = "")
                : name(name)
                , desc(desc) {}
            QString name;
            QString desc;
        };
        virtual bool GetItems (IVariable* /* pVar */, std::vector<SItem>& /* items */, QString& /* outDialogTitle */) = 0;
        virtual bool UseTree() = 0;
        virtual const char* GetTreeSeparator() = 0;
    };

    //! Get name of parameter.
    virtual QString GetName() const = 0;
    //! Set name of parameter.
    virtual void SetName(const QString& name) = 0;

    //! Get human readable name of parameter (Normally same as name).
    virtual QString GetHumanName() const = 0;
    //! Set human readable name of parameter (name without prefix).
    virtual void SetHumanName(const QString& name) = 0;

    //! Get variable description.
    virtual QString GetDescription() const = 0;
    //! Set variable description.
    virtual void SetDescription(const char* desc) = 0;
    //! Set variable description.
    virtual void SetDescription(const QString& desc) = 0;

    //! Get parameter type.
    virtual EType   GetType() const = 0;
    //! Get size of parameter.
    virtual int GetSize() const = 0;

    //! Type of data stored in this variable.
    virtual unsigned char   GetDataType() const = 0;
    virtual void SetDataType(unsigned char dataType) = 0;

    virtual void SetUserData(const QVariant &data) = 0;
    virtual QVariant GetUserData() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Flags
    //////////////////////////////////////////////////////////////////////////
    //! Set variable flags, (Limited to 16 flags).
    virtual void SetFlags(int flags) = 0;
    virtual int  GetFlags() const = 0;
    virtual void SetFlagRecursive(EFlags flag) = 0;

    /////////////////////////////////////////////////////////////////////////////
    // Set methods.
    /////////////////////////////////////////////////////////////////////////////
    virtual void Set(int value) = 0;
    virtual void Set(bool value) = 0;
    virtual void Set(float value) = 0;
    virtual void Set(double value) = 0;
    virtual void Set(const Vec2& value) = 0;
    virtual void Set(const Vec3& value) = 0;
    virtual void Set(const Vec4& value) = 0;
    virtual void Set(const Ang3& value) = 0;
    virtual void Set(const Quat& value) = 0;
    virtual void Set(const QString& value) = 0;
    virtual void Set(const char* value) = 0;
    virtual void SetDisplayValue(const QString& value) = 0;

    // Called when value updated by any means (including internally).
    virtual void OnSetValue(bool bRecursive) = 0;

    /////////////////////////////////////////////////////////////////////////////
    // Get methods.
    /////////////////////////////////////////////////////////////////////////////
    virtual void Get(int& value) const  = 0;
    virtual void Get(bool& value) const  = 0;
    virtual void Get(float& value) const  = 0;
    virtual void Get(double& value) const = 0;
    virtual void Get(Vec2& value) const  = 0;
    virtual void Get(Vec3& value) const  = 0;
    virtual void Get(Vec4& value) const  = 0;
    virtual void Get(Ang3& value) const  = 0;
    virtual void Get(Quat& value) const  = 0;
    virtual void Get(QString& value) const = 0;
    virtual QString GetDisplayValue() const = 0;
    virtual bool HasDefaultValue() const = 0;

    //! reset value to default value
    virtual void ResetToDefault() = 0;

    //! Return cloned value of variable.
    virtual IVariable* Clone(bool bRecursive) const = 0;

    //! Copy variable value from specified variable.
    //! This method executed always recursively on all sub hierarchy of variables,
    //! In Array vars, will never create new variables, only copy values of corresponding childs.
    //! @param fromVar Source variable to copy value from.
    virtual void CopyValue(IVariable* fromVar) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Value Limits.
    //////////////////////////////////////////////////////////////////////////
    //! Set value limits.
    virtual void SetLimits([[maybe_unused]] float fMin, [[maybe_unused]] float fMax, [[maybe_unused]] float fStep = 0.f, [[maybe_unused]] bool bHardMin = true, [[maybe_unused]] bool bHardMax = true) {}
    //! Get value limits.
    virtual void GetLimits([[maybe_unused]] float& fMin, [[maybe_unused]] float& fMax, [[maybe_unused]] float& fStep, [[maybe_unused]] bool& bHardMin, [[maybe_unused]] bool& bHardMax) {}
    void GetLimits(float& fMin, float& fMax)
    {
        float f;
        bool b;
        GetLimits(fMin, fMax, f, b, b);
    }
    virtual bool HasCustomLimits() { return false; }

    virtual void EnableNotifyWithoutValueChange([[maybe_unused]] bool bFlag){}


    //////////////////////////////////////////////////////////////////////////
    // Wire/Unwire variables.
    //////////////////////////////////////////////////////////////////////////
    //! Wire variable, wired variable will be changed when this var changes.
    virtual void Wire(IVariable* targetVar) = 0;
    //! Unwire variable.
    virtual void Unwire(IVariable* targetVar) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Assign on set callback.
    //////////////////////////////////////////////////////////////////////////
    virtual void AddOnSetCallback(OnSetCallback* func) = 0;
    virtual void RemoveOnSetCallback(OnSetCallback* func) = 0;
    virtual void ClearOnSetCallbacks() {}

    //////////////////////////////////////////////////////////////////////////
    // Assign callback triggered when enums change.
    //////////////////////////////////////////////////////////////////////////
    virtual void AddOnSetEnumCallback(OnSetEnumCallback* func) = 0;
    virtual void RemoveOnSetEnumCallback(OnSetCallback* func) = 0;
    virtual void ClearOnSetEnumCallbacks() {}

    //////////////////////////////////////////////////////////////////////////
    //! Retrieve pointer to selection list used by variable.
    virtual IVarEnumList* GetEnumList() const { return 0; }
    virtual ISplineInterpolator* GetSpline() { return 0; }

    //////////////////////////////////////////////////////////////////////////
    //! Serialize variable to XML.
    //////////////////////////////////////////////////////////////////////////
    virtual void Serialize(XmlNodeRef node, bool load) = 0;

    // From CObject, (not implemented)
    virtual void Serialize([[maybe_unused]] CArchive& ar) {}

    //////////////////////////////////////////////////////////////////////////
    // Disables the update callbacks for certain operations in order to avoid
    // too many function calls when not needed.
    //////////////////////////////////////////////////////////////////////////
    virtual void EnableUpdateCallbacks(bool boEnable) = 0;

    // Setup to true to force save Undo in case the value the same.
    virtual void SetForceModified([[maybe_unused]] bool bForceModified) {}
};

// Smart pointer to this parameter.
typedef _smart_ptr<IVariable> IVariablePtr;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/**
 **************************************************************************************
 * CVariableBase implements IVariable interface and provide default implementation
 * for basic IVariable functionality (Name, Flags, etc...)
 * CVariableBase cannot be instantiated directly and should be used as the base class for
 * actual Variant implementation classes.
 ***************************************************************************************
 */
class EDITOR_CORE_API CVariableBase
    : public IVariable
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    virtual ~CVariableBase() {}

    void SetName(const QString& name) override { m_name = name; };
    //! Get name of parameter.
    QString GetName() const override { return m_name; };

    QString GetHumanName() const override
    {
        if (!m_humanName.isEmpty())
        {
            return m_humanName;
        }
        return m_name;
    }
    void SetHumanName(const QString& name) override { m_humanName = name; }

    void SetDescription(const char* desc) override { m_description = desc; };
    void SetDescription(const QString& desc) override { m_description = desc; };

    //! Get name of parameter.
    QString GetDescription() const override { return m_description; };

    EType   GetType() const override { return IVariable::UNKNOWN; };
    int GetSize() const override { return sizeof(*this); };

    unsigned char   GetDataType() const override { return m_dataType; };
    void SetDataType(unsigned char dataType) override { m_dataType = dataType; }

    void SetFlags(int flags) override { m_flags = static_cast<uint16>(flags); }
    int  GetFlags() const override { return m_flags; }
    void  SetFlagRecursive(EFlags flag) override { m_flags |= flag; }

    void SetUserData(const QVariant &data) override { m_userData = data; };
    QVariant GetUserData() const override { return m_userData; }

    //////////////////////////////////////////////////////////////////////////
    // Set methods.
    //////////////////////////////////////////////////////////////////////////
    void Set([[maybe_unused]] int value) override                   { assert(0); }
    void Set([[maybe_unused]] bool value) override                  { assert(0); }
    void Set([[maybe_unused]] float value) override                 { assert(0); }
    void Set([[maybe_unused]] double value) override                { assert(0); }
    void Set([[maybe_unused]] const Vec2& value) override           { assert(0); }
    void Set([[maybe_unused]] const Vec3& value) override           { assert(0); }
    void Set([[maybe_unused]] const Vec4& value) override           { assert(0); }
    void Set([[maybe_unused]] const Ang3& value) override           { assert(0); }
    void Set([[maybe_unused]] const Quat& value) override           { assert(0); }
    void Set([[maybe_unused]] const QString& value) override        { assert(0); }
    void Set([[maybe_unused]] const char* value) override            { assert(0); }
    void SetDisplayValue(const QString& value) override    { Set(value); }

    //////////////////////////////////////////////////////////////////////////
    // Get methods.
    //////////////////////////////////////////////////////////////////////////
    void Get([[maybe_unused]] int& value) const override        { assert(0); }
    void Get([[maybe_unused]] bool& value) const override       { assert(0); }
    void Get([[maybe_unused]] float& value) const override      { assert(0); }
    void Get([[maybe_unused]] double& value) const override     { assert(0); }
    void Get([[maybe_unused]] Vec2& value) const override       { assert(0); }
    void Get([[maybe_unused]] Vec3& value) const override       { assert(0); }
    void Get([[maybe_unused]] Vec4& value) const override       { assert(0); }
    void Get([[maybe_unused]] Ang3& value) const override       { assert(0); }
    void Get([[maybe_unused]] Quat& value) const override       { assert(0); }
    void Get([[maybe_unused]] QString& value) const override    { assert(0); }
    QString GetDisplayValue() const override     { QString val; Get(val); return val; }

    //////////////////////////////////////////////////////////////////////////
    // IVariableContainer functions
    //////////////////////////////////////////////////////////////////////////
    void AddVariable([[maybe_unused]] IVariable* var) override { assert(0); }

    bool DeleteVariable([[maybe_unused]] IVariable* var, [[maybe_unused]] bool recursive = false) override { return false; }
    void DeleteAllVariables() override {}

    int GetNumVariables() const override { return 0; }
    IVariable* GetVariable([[maybe_unused]] int index) const override { return nullptr; }

    bool IsContainsVariable([[maybe_unused]] IVariable* pVar, [[maybe_unused]] bool bRecursive = false) const override { return false; }

    IVariable* FindVariable([[maybe_unused]] const char* name, [[maybe_unused]] bool bRecursive = false, [[maybe_unused]] bool bHumanName = false) const override { return nullptr; }

    bool IsEmpty() const override { return true; }

    //////////////////////////////////////////////////////////////////////////
    void Wire(IVariable* var) override
    {
        m_wiredVars.push_back(var);
    }
    //////////////////////////////////////////////////////////////////////////
    void Unwire(IVariable* var) override
    {
        if (!var)
        {
            // Unwire all.
            m_wiredVars.clear();
        }
        else
        {
            stl::find_and_erase(m_wiredVars, var);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void AddOnSetCallback(OnSetCallback* func) override
    {
        if (!stl::find(m_onSetFuncs, func))
        {
            m_onSetFuncs.push_back(func);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void RemoveOnSetCallback(OnSetCallback* func) override
    {
        stl::find_and_erase(m_onSetFuncs, func);
    }

    //////////////////////////////////////////////////////////////////////////
    void ClearOnSetCallbacks() override
    {
        m_onSetFuncs.clear();
    }

    void AddOnSetEnumCallback(OnSetEnumCallback* func) override
    {
        if (!stl::find(m_onSetEnumFuncs, func))
        {
            m_onSetEnumFuncs.push_back(func);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void RemoveOnSetEnumCallback(OnSetCallback* func) override
    {
        stl::find_and_erase(m_onSetEnumFuncs, func);
    }

    void ClearOnSetEnumCallbacks() override
    {
        m_onSetEnumFuncs.clear();
    }


    void OnSetValue([[maybe_unused]] bool bRecursive) override
    {
        // If have wired variables or OnSet callback, process them.
        // Send value to wired variable.
        for (CVariableBase::WiredList::iterator it = m_wiredVars.begin(); it != m_wiredVars.end(); ++it)
        {
            if (m_bForceModified)
            {
                (*it)->SetForceModified(true);
            }

            // Copy value to wired vars.
            (*it)->CopyValue(this);
        }

        if (!m_boUpdateCallbacksEnabled)
        {
            return;
        }

        // Call on set callback.
        for (auto it = m_onSetFuncs.begin(); it != m_onSetFuncs.end(); ++it)
        {
            // Call on set callback.
            (*it)->operator()(this);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    using IVariable::Serialize;
    void Serialize(XmlNodeRef node, bool load) override
    {
        if (load)
        {
            if (node->haveAttr(m_name.toUtf8().data()))
            {
                Set(node->getAttr(m_name.toUtf8().data()));
            }
        }
        else
        {
            // Saving.
            QString str;
            Get(str);
            node->setAttr(m_name.toUtf8().data(), str.toUtf8().data());
        }
    }

    void EnableUpdateCallbacks(bool boEnable) override{m_boUpdateCallbacksEnabled = boEnable; };
    void SetForceModified(bool bForceModified) override { m_bForceModified = bForceModified; }
protected:
    // Constructor.
    CVariableBase()
        : m_dataType(DT_SIMPLE)
        , m_flags(0)
        , m_boUpdateCallbacksEnabled(true)
        , m_bForceModified(false)
    {
    }

    // Copy constructor.
    CVariableBase(const CVariableBase& var)
    {
        m_name = var.m_name;
        m_humanName = var.m_humanName;
        m_description = var.m_description;
        m_flags = var.m_flags;
        m_dataType = var.m_dataType;
        m_userData = var.m_userData;
        m_boUpdateCallbacksEnabled = true;
        m_bForceModified = var.m_bForceModified;
        // Never copy callback function or wired variables they are private to specific variable,
    }

    // Not allow.
    CVariableBase& operator=([[maybe_unused]] const CVariableBase& var)
    {
        return *this;
    }

    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////
    typedef std::vector<OnSetCallback*> OnSetCallbackList;
    typedef std::vector<IVariablePtr> WiredList;
    using OnSetEnumCallbackList = std::vector<OnSetEnumCallback*>;

    QString m_name;
    QString m_humanName;
    QString m_description;

    //! Optional userdata pointer
    QVariant m_userData;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //! Extended data (Extended data is never copied, it's always private to this variable).
    WiredList m_wiredVars;
    OnSetCallbackList m_onSetFuncs;
    OnSetEnumCallbackList m_onSetEnumFuncs;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    uint16 m_flags;
    //! Limited to 8 flags.
    unsigned char m_dataType;

    bool m_boUpdateCallbacksEnabled;

    bool m_bForceModified;
};

/**
 **************************************************************************************
 * CVariableArray implements variable of type array of IVariables.
 ***************************************************************************************
 */
class EDITOR_CORE_API CVariableArray
    : public CVariableBase
{
public:
    CVariableArray(){}

    //! Get name of parameter.
    EType   GetType() const override { return IVariable::ARRAY; };
    int     GetSize() const override { return sizeof(CVariableArray); };

    //////////////////////////////////////////////////////////////////////////
    // Set methods.
    //////////////////////////////////////////////////////////////////////////
    void Set(const QString& value) override
    {
        if (m_strValue != value)
        {
            m_strValue = value;
            OnSetValue(false);
        }
    }
    void OnSetValue(bool bRecursive) override
    {
        CVariableBase::OnSetValue(bRecursive);
        if (bRecursive)
        {
            for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            {
                (*it)->OnSetValue(true);
            }
        }
    }
    void SetFlagRecursive(EFlags flag) override
    {
        CVariableBase::SetFlagRecursive(flag);
        for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            (*it)->SetFlagRecursive(flag);
        }
    }
    //////////////////////////////////////////////////////////////////////////
    // Get methods.
    //////////////////////////////////////////////////////////////////////////
    void Get(QString& value) const override { value = m_strValue; }

    bool HasDefaultValue() const override
    {
        for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            if (!(*it)->HasDefaultValue())
            {
                return false;
            }
        }
        return true;
    }

    void ResetToDefault() override
    {
        for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            (*it)->ResetToDefault();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    IVariable* Clone(bool bRecursive) const override
    {
        CVariableArray* var = new CVariableArray(*this);

        // m_vars was shallow-duplicated, clone elements.
        for (int i = 0; i < m_vars.size(); i++)
        {
            var->m_vars[i] = m_vars[i]->Clone(bRecursive);
        }
        return var;
    }

    //////////////////////////////////////////////////////////////////////////
    void CopyValue(IVariable* fromVar) override
    {
        assert(fromVar);
        if (fromVar->GetType() != IVariable::ARRAY)
        {
            return;
        }
        int numSrc = fromVar->GetNumVariables();
        int numTrg = static_cast<int>(m_vars.size());
        for (int i = 0; i < numSrc && i < numTrg; i++)
        {
            // Copy Every child variable.
            m_vars[i]->CopyValue(fromVar->GetVariable(i));
        }
        QString strValue;
        fromVar->Get(strValue);
        Set(strValue);
    }

    //////////////////////////////////////////////////////////////////////////
    int GetNumVariables() const override { return static_cast<int>(m_vars.size()); }

    IVariable* GetVariable(int index) const override
    {
        assert(index >= 0 && index < (int)m_vars.size());
        return m_vars[index];
    }

    void AddVariable(IVariable* var) override
    {
        m_vars.push_back(var);
    }

    bool DeleteVariable(IVariable* var, bool recursive /*=false*/) override
    {
        bool found = stl::find_and_erase(m_vars, var);
        if (!found && recursive)
        {
            for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            {
                if ((*it)->DeleteVariable(var, recursive))
                {
                    return true;
                }
            }
        }
        return found;
    }

    void DeleteAllVariables() override
    {
        m_vars.clear();
    }

    bool IsContainsVariable(IVariable* pVar, bool bRecursive) const override
    {
        for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            if (*it == pVar)
            {
                return true;
            }
        }

        // If not found search childs.
        if (bRecursive)
        {
            // Search all top level variables.
            for (Variables::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            {
                if ((*it)->IsContainsVariable(pVar))
                {
                    return true;
                }
            }
        }

        return false;
    }

    IVariable* FindVariable(const char* name, bool bRecursive, bool bHumanName) const override;

    bool IsEmpty() const override
    {
        return m_vars.empty();
    }

    using IVariable::Serialize;
    void Serialize(XmlNodeRef node, bool load) override
    {
        if (load)
        {
            // Loading.
            QString name;
            for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            {
                IVariable* var = *it;
                if (var->GetNumVariables())
                {
                    XmlNodeRef child = node->findChild(var->GetName().toUtf8().data());
                    if (child)
                    {
                        var->Serialize(child, load);
                    }
                }
                else
                {
                    var->Serialize(node, load);
                }
            }
        }
        else
        {
            // Saving.
            for (Variables::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            {
                IVariable* var = *it;
                if (var->GetNumVariables())
                {
                    XmlNodeRef child = node->newChild(var->GetName().toUtf8().data());
                    var->Serialize(child, load);
                }
                else
                {
                    var->Serialize(node, load);
                }
            }
        }
    }

protected:
    typedef std::vector<IVariablePtr> Variables;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Variables m_vars;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //! Any string value displayed in properties.
    QString m_strValue;
};

/** var_type namespace includes type definitions needed for CVariable implementaton.
 */
namespace var_type
{
    //////////////////////////////////////////////////////////////////////////
    template <int TypeID, bool IsStandart, bool IsInteger, bool IsSigned, bool SupportsRange>
    struct type_traits_base
    {
        static int type() { return TypeID; };
        //! Return true if standard C++ type.
        static bool is_standart() { return IsStandart; };
        static bool is_integer() { return IsInteger; };
        static bool is_signed() { return IsSigned; };
        static bool supports_range() { return SupportsRange; };
    };

    template <class Type>
    struct type_traits
        : public type_traits_base<IVariable::UNKNOWN, false, false, false, false> {};

    // Types specialization.
    template<>
    struct type_traits<int>
        : public type_traits_base<IVariable::INT, true, true, true, true> {};
    template<>
    struct type_traits<bool>
        : public type_traits_base<IVariable::BOOL, true, true, false, true> {};
    template<>
    struct type_traits<float>
        : public type_traits_base<IVariable::FLOAT, true, false, false, true> {};
    template<>
    struct type_traits<double>
        : public type_traits_base<IVariable::DOUBLE, true, false, false, true>{};
    template<>
    struct type_traits<Vec2>
        : public type_traits_base<IVariable::VECTOR2, false, false, false, false> {};
    template<>
    struct type_traits<Vec3>
        : public type_traits_base<IVariable::VECTOR, false, false, false, false> {};
    template<>
    struct type_traits<Vec4>
        : public type_traits_base<IVariable::VECTOR4, false, false, false, false> {};
    template<>
    struct type_traits<Quat>
        : public type_traits_base<IVariable::QUAT, false, false, false, false> {};
    template<>
    struct type_traits<QString>
        : public type_traits_base<IVariable::STRING, false, false, false, false> {};
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // General one type to another type convertor class.
    //////////////////////////////////////////////////////////////////////////
    struct type_convertor
    {
        template <class From, class To>
        void operator()([[maybe_unused]] const From& from, [[maybe_unused]] To& to) const { assert(0); }

        void operator()(const int& from, int& to) const { to = from; }
        void operator()(const int& from, bool& to) const { to = from != 0; }
        void operator()(const int& from, float& to) const { to = (float)from; }
        //////////////////////////////////////////////////////////////////////////
        void operator()(const bool& from, int& to) const { to = from; }
        void operator()(const bool& from, bool& to) const { to = from; }
        void operator()(const bool& from, float& to) const { to = from; }
        //////////////////////////////////////////////////////////////////////////
        void operator()(const float& from, int& to) const { to = (int)from; }
        void operator()(const float& from, bool& to) const { to = from != 0; }
        void operator()(const float& from, float& to) const { to = from; }

        void operator()(const double& from, int& to) const { to = (int)from; }
        void operator()(const double& from, bool& to) const { to = from != 0; }
        void operator()(const double& from, float& to) const { to = aznumeric_cast<float>(from); }

        void operator()(const Vec2& from, Vec2& to) const { to = from; }
        void operator()(const Vec3& from, Vec3& to) const { to = from; }
        void operator()(const Vec4& from, Vec4& to) const { to = from; }
        void operator()(const Quat& from, Quat& to) const { to = from; }
        void operator()(const QString& from, QString& to) const { to = from; }

        void operator()(bool value, QString& to) const { to = QString::number((value) ? (int)1 : (int)0); }
        void operator()(int value, QString& to) const { to = QString::number(value); }
        void operator()(float value, QString& to) const { to = QString::number(value); };
        void operator()(double value, QString& to) const { to = QString::number(value); };
        void operator()(const Vec2& value, QString& to) const { to = QString::fromLatin1("%1,%2").arg(value.x).arg(value.y); }
        void operator()(const Vec3& value, QString& to) const { to = QString::fromLatin1("%1,%2,%3").arg(value.x).arg(value.y).arg(value.z); }
        void operator()(const Vec4& value, QString& to) const { to = QString::fromLatin1("%1,%2,%3,%4").arg(value.x).arg(value.y).arg(value.z).arg(value.w); }
        void operator()(const Ang3& value, QString& to) const { to = QString::fromLatin1("%1,%2,%3").arg(value.x).arg(value.y).arg(value.z); }
        void operator()(const Quat& value, QString& to) const { to = QString::fromLatin1("%1,%2,%3,%4").arg(value.w).arg(value.v.x).arg(value.v.y).arg(value.v.z); }

        void operator()(const QString& from, int& value) const { value = from.toInt(); }
        void operator()(const QString& from, bool& value) const { value = from.toInt() != 0; }
        void operator()(const QString& from, float& value) const { value = from.toFloat(); }
        void operator()(const QString& from, Vec2& value) const
        {
            QStringList parts = from.split(QStringLiteral(","));
            while (parts.size() < 2)
            {
                parts.push_back(QString());
            }
            value.x = parts[0].toFloat();
            value.y = parts[1].toFloat();
        };
        void operator()(const QString& from, Vec3& value) const
        {
            QStringList parts = from.split(QStringLiteral(","));
            while (parts.size() < 3)
            {
                parts.push_back(QString());
            }
            value.x = parts[0].toFloat();
            value.y = parts[1].toFloat();
            value.z = parts[2].toFloat();
        };
        void operator()(const QString& from, Vec4& value) const
        {
            QStringList parts = from.split(QStringLiteral(","));
            while (parts.size() < 4)
            {
                parts.push_back(QString());
            }
            value.x = parts[0].toFloat();
            value.y = parts[1].toFloat();
            value.z = parts[2].toFloat();
            value.w = parts[3].toFloat();
        };
        void operator()(const QString& from, Ang3& value) const
        {
            QStringList parts = from.split(QStringLiteral(","));
            while (parts.size() < 3)
            {
                parts.push_back(QString());
            }
            value.x = parts[0].toFloat();
            value.y = parts[1].toFloat();
            value.z = parts[2].toFloat();
        };
        void operator()(const QString& from, Quat& value) const
        {
            QStringList parts = from.split(QStringLiteral(","));
            while (parts.size() < 4)
            {
                parts.push_back(QString());
            }
            value.w = parts[0].toFloat();
            value.v.x = parts[1].toFloat();
            value.v.y = parts[2].toFloat();
            value.v.z = parts[3].toFloat();
        };
    };

    //////////////////////////////////////////////////////////////////////////
    // Custom comparison functions for different variable type's values,.
    //////////////////////////////////////////////////////////////////////////
    template <class Type>
    inline bool compare(const Type& arg1, const Type& arg2)
    {
        return arg1 == arg2;
    };
    inline bool compare(const Vec2& v1, const Vec2& v2)
    {
        return v1.x == v2.x && v1.y == v2.y;
    }
    inline bool compare(const Vec3& v1, const Vec3& v2)
    {
        return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
    }
    inline bool compare(const Vec4& v1, const Vec4& v2)
    {
        return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
    }
    inline bool compare(const Ang3& v1, const Ang3& v2)
    {
        return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
    }
    inline bool compare(const Quat& q1, const Quat& q2)
    {
        return q1.v.x == q2.v.x && q1.v.y == q2.v.y && q1.v.z == q2.v.z && q1.w == q2.w;
    }
    inline bool compare(const char* s1, const char* s2)
    {
        return strcmp(s1, s2) == 0;
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Custom Initialization functions for different variable type's values,.
    //////////////////////////////////////////////////////////////////////////
    template <class Type>
    inline void init(Type& val)
    {
        val = 0;
    };
    inline void init(Vec2& val)   {   val.x = 0; val.y = 0;   };
    inline void init(Vec3& val)   {   val.x = 0; val.y = 0; val.z = 0; };
    inline void init(Vec4& val)   {   val.x = 0; val.y = 0; val.z = 0; val.w = 0;  };
    inline void init(Ang3& val)   {   val.x = 0; val.y = 0; val.z = 0; };
    inline void init(Quat& val)
    {
        val.v.x = 0;
        val.v.y = 0;
        val.v.z = 0;
        val.w = 0;
    };
    inline void init(const char*& val)
    {
        val = "";
    };
    inline void init([[maybe_unused]] QString& val)
    {
        // self initializing.
    }
    //////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
// Void variable does not contain any value.
//////////////////////////////////////////////////////////////////////////
class CVariableVoid
    : public CVariableBase
{
public:
    CVariableVoid(){};
    EType GetType() const override { return IVariable::UNKNOWN; };
    IVariable* Clone([[maybe_unused]] bool bRecursive) const override { return new CVariableVoid(*this); }
    void CopyValue([[maybe_unused]] IVariable* fromVar) override {};
    bool HasDefaultValue() const override { return true; }
    void ResetToDefault() override {};
protected:
    CVariableVoid(const CVariableVoid& v)
        : CVariableBase(v) {};
};

//////////////////////////////////////////////////////////////////////////
template <class T>
class CVariable
    : public CVariableBase
{
    typedef CVariable<T> Self;
public:
    // Constructor.
    CVariable()
        : m_valueMin(0.0f)
        , m_valueMax(100.0f)
        , m_valueStep(0.0f)
        , m_bHardMin(false)
        , m_bHardMax(false)
        , m_customLimits(false)
    {
        // Initialize value to zero or empty string.
        var_type::init(m_valueDef);
    }

    explicit CVariable(const T& set)
        : CVariable()
    {
        var_type::init(m_valueDef);   // Update F32NAN values in Debud mode
        SetValue(set);
    }

    //! Get name of parameter.
    EType   GetType() const override { return (EType)var_type::type_traits<T>::type(); };
    int     GetSize() const override { return sizeof(T); };

    //////////////////////////////////////////////////////////////////////////
    // Set methods.
    //////////////////////////////////////////////////////////////////////////
    void Set(int value) override                           { SetValue(value); }
    void Set(bool value) override                      { SetValue(value); }
    void Set(float value) override                     { SetValue(value); }
    void Set(double value) override                { SetValue(value); }
    void Set(const Vec2& value) override           { SetValue(value); }
    void Set(const Vec3& value) override           { SetValue(value); }
    void Set(const Vec4& value) override           { SetValue(value); }
    void Set(const Ang3& value) override           { SetValue(value); }
    void Set(const Quat& value) override           { SetValue(value); }
    void Set(const QString& value) override    { SetValue(value); }
    void Set(const char* value) override      { SetValue(QString(value)); }

    //////////////////////////////////////////////////////////////////////////
    // Get methods.
    //////////////////////////////////////////////////////////////////////////
    void Get(int& value) const override                        { GetValue(value); }
    void Get(bool& value) const override                   { GetValue(value); }
    void Get(float& value) const override                  { GetValue(value); }
    void Get(double& value) const override                  { GetValue(value); }
    void Get(Vec2& value) const override                   { GetValue(value); }
    void Get(Vec3& value) const override                   { GetValue(value); }
    void Get(Vec4& value) const override                   { GetValue(value); }
    void Get(Quat& value) const override                   { GetValue(value); }
    void Get(QString& value) const override                { GetValue(value); }
    bool HasDefaultValue() const override
    {
        T defval;
        var_type::init(defval);
        return m_valueDef == defval;
    }

    void ResetToDefault() override
    {
        T defval;
        var_type::init(defval);
        SetValue(defval);
    }

    //////////////////////////////////////////////////////////////////////////
    // Limits.
    //////////////////////////////////////////////////////////////////////////
    void SetLimits(float fMin, float fMax, float fStep = 0.f, bool bHardMin = true, bool bHardMax = true) override
    {
        m_valueMin = fMin;
        m_valueMax = fMax;
        m_valueStep = fStep;
        m_bHardMin = bHardMin;
        m_bHardMax = bHardMax;

        // Flag to determine when this variable has custom limits set
        m_customLimits = true;
    }

    void GetLimits(float& fMin, float& fMax, float& fStep, bool& bHardMin, bool& bHardMax) override
    {
        if (!m_customLimits && var_type::type_traits<T>::supports_range())
        {
            float value;
            GetValue(value);
            if (value > m_valueMax && !m_bHardMax)
            {
                SetLimits(m_valueMin, nextNiceNumberAbove(value), 0.0, false, false);
            }
            if (value < m_valueMin && !m_bHardMin)
            {
                SetLimits(nextNiceNumberBelow(value), m_valueMax, 0.0, false, false);
            }
        }

        fMin = m_valueMin;
        fMax = m_valueMax;
        fStep = m_valueStep;
        bHardMin = m_bHardMin;
        bHardMax = m_bHardMax;
    }
    void ClearLimits()
    {
        SetLimits(0.0f, 0.0f, 0.0f, false, false);
        m_customLimits = false;
    }

    bool HasCustomLimits() override
    {
        return m_customLimits;
    }

    //////////////////////////////////////////////////////////////////////////
    // Access operators.
    //////////////////////////////////////////////////////////////////////////

    //! Cast to held type.
    operator T const& () const {
        return m_valueDef;
    }

    //! Assign operator for variable.
    void operator=(const T& value) { SetValue(value); }

    //////////////////////////////////////////////////////////////////////////
    IVariable* Clone([[maybe_unused]] bool bRecursive) const override
    {
        Self* var = new Self(*this);
        return var;
    }

    //////////////////////////////////////////////////////////////////////////
    void CopyValue(IVariable* fromVar) override
    {
        assert(fromVar);
        T val;
        fromVar->Get(val);
        SetValue(val);
    }

protected:

    //////////////////////////////////////////////////////////////////////////
    template <class P>
    void SetValue(const P& value)
    {
        T newValue = T();
        //var_type::type_convertor<P,T> convertor;
        var_type::type_convertor convertor;
        convertor(value, newValue);

        // compare old and new values.
        if (m_bForceModified || !var_type::compare(m_valueDef, newValue))
        {
            m_valueDef = newValue;
            m_bForceModified = false;
            OnSetValue(false);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template <class P>
    void GetValue(P& value) const
    {
        var_type::type_convertor convertor;
        convertor(m_valueDef, value);
    }

protected:
    T m_valueDef;
    bool m_customLimits;
    bool m_bResolving;
    unsigned char m_bHardMin : 1;
    unsigned char m_bHardMax : 1;
    // Min/Max value.
    float m_valueMin, m_valueMax, m_valueStep;
};

//////////////////////////////////////////////////////////////////////////

/**
**************************************************************************************
* CVariableArray implements variable of type array of IVariables.
***************************************************************************************
*/
template <class T>
class TVariableArray
    : public CVariable<T>
{
    typedef TVariableArray<T> Self;
public:
    using CVariable<T>::GetType;
    TVariableArray()
        : CVariable<T>() {};
    // Copy Constructor.
    TVariableArray(const Self& var)
        : CVariable<T>(var)
    {}

    //! Get name of parameter.
    virtual int     GetSize() const { return sizeof(Self); };

    virtual bool HasDefaultValue() const
    {
        for (Vars::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            if (!(*it)->HasDefaultValue())
            {
                return false;
            }
        }
        return true;
    }

    virtual void ResetToDefault()
    {
        for (Vars::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            (*it)->ResetToDefault();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    IVariable* Clone(bool bRecursive) const
    {
        Self* var = new Self(*this);
        for (Vars::const_iterator it = m_vars.begin(); it != m_vars.end(); ++it)
        {
            var->m_vars.push_back((*it)->Clone(bRecursive));
        }
        return var;
    }
    //////////////////////////////////////////////////////////////////////////
    void CopyValue(IVariable* fromVar)
    {
        assert(fromVar);
        if (fromVar->GetType() != GetType())
        {
            return;
        }
        CVariable<T>::CopyValue(fromVar);
        int numSrc = fromVar->GetNumVariables();
        int numTrg = m_vars.size();
        for (int i = 0; i < numSrc && i < numTrg; i++)
        {
            // Copy Every child variable.
            m_vars[i]->CopyValue(fromVar->GetVariable(i));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    int GetNumVariables() const { return m_vars.size(); }
    IVariable* GetVariable(int index) const
    {
        assert(index >= 0 && index < (int)m_vars.size());
        return m_vars[index];
    }
    void AddVariable(IVariable* var) { m_vars.push_back(var); }
    void DeleteAllVariables() { m_vars.clear(); }

    //////////////////////////////////////////////////////////////////////////
    void Serialize(XmlNodeRef node, bool load)
    {
        CVariable<T>::Serialize(node, load);
        if (load)
        {
            // Loading.
            QString name;
            for (Vars::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            {
                IVariable* var = *it;
                if (var->GetNumVariables())
                {
                    XmlNodeRef child = node->findChild(var->GetName().toUtf8().data());
                    if (child)
                    {
                        var->Serialize(child, load);
                    }
                }
                else
                {
                    var->Serialize(node, load);
                }
            }
        }
        else
        {
            // Saving.
            for (Vars::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            {
                IVariable* var = *it;
                if (var->GetNumVariables())
                {
                    XmlNodeRef child = node->newChild(var->GetName().toUtf8().data());
                    var->Serialize(child, load);
                }
                else
                {
                    var->Serialize(node, load);
                }
            }
        }
    }

protected:
    typedef std::vector<IVariablePtr> Vars;
    Vars m_vars;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Selection list shown in combo box, for enumerated variable.
struct IVarEnumList
    : public CRefCountBase
{
    //! Get the name of specified value in enumeration, or empty string if out of range.
    virtual QString GetItemName(uint index) = 0;
};
typedef _smart_ptr<IVarEnumList> IVarEnumListPtr;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING;
//! Selection list shown in combo box, for enumerated variable.
template <class T>
class CVarEnumListBase
    : public IVarEnumList
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CVarEnumListBase(){}

    //////////////////////////////////////////////////////////////////////////
    virtual T NameToValue(const QString& name) = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual QString ValueToName(T const& value) = 0;

    //! Add new item to the selection.
    virtual void AddItem(const QString& name, const T& value) = 0;

    template <class TVal>
    static bool IsValueEqual(const TVal& v1, const TVal& v2)
    {
        return v1 == v2;
    }
    static bool IsValueEqual(const QString& v1, const QString& v2)
    {
        // Case insensitive compare.
        return QString::compare(v1, v2, Qt::CaseInsensitive) == 0;
    }

protected:
    virtual ~CVarEnumListBase() {};
    friend class _smart_ptr<CVarEnumListBase<T> >;
};

struct CUIEnumsDatabase_SEnum;

//////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class EDITOR_CORE_API CVarGlobalEnumList
    : public CVarEnumListBase<QString>
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CVarGlobalEnumList(CUIEnumsDatabase_SEnum* pEnum);
    CVarGlobalEnumList(const QString& enumName);

    //! Get the name of specified value in enumeration.
    virtual QString GetItemName(uint index);

    virtual QString NameToValue(const QString& name);
    virtual QString ValueToName(const QString& value);

    //! Don't add anything to a global enum database
    virtual void AddItem([[maybe_unused]] const QString& name, [[maybe_unused]] const QString& value) {}

private:
    CUIEnumsDatabase_SEnum* m_pEnum;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Selection list shown in combo box, for enumerated variable.
template <class T>
class CVarEnumList
    : public CVarEnumListBase<T>
{
public:
    using CVarEnumListBase<T>::IsValueEqual;
    CVarEnumList(){}
    struct Item
    {
        QString name;
        T value;
    };
    QString GetItemName(uint index)
    {
        if (index >= m_items.size())
        {
            return QString();
        }
        return m_items[index].name;
    };

    //////////////////////////////////////////////////////////////////////////
    T NameToValue(const QString& name)
    {
        for (int i = 0; i < m_items.size(); i++)
        {
            if (name == m_items[i].name)
            {
                return m_items[i].value;
            }
        }
        return T();
    }

    //////////////////////////////////////////////////////////////////////////
    QString ValueToName(T const& value)
    {
        for (int i = 0; i < m_items.size(); i++)
        {
            if (CVarEnumListBase<T>::IsValueEqual(value, m_items[i].value))
            {
                return m_items[i].name;
            }
        }
        return "";
    }

    //! Add new item to the selection.
    void AddItem(const QString& name, const T& value)
    {
        Item item;
        item.name = name;
        item.value = value;
        m_items.push_back(item);
    };

protected:
    ~CVarEnumList() {};

private:
    std::vector<Item> m_items;
};

//////////////////////////////////////////////////////////////////////////////////
// CVariableEnum is the same as CVariable but it display enumerated values in UI
//////////////////////////////////////////////////////////////////////////////////
template <class T>
class CVariableEnum
    : public CVariable<T>
{
public:
    using CVariable<T>::SetValue;
    using CVariable<T>::GetFlags;
    using CVariable<T>::OnSetValue;
    using CVariable<T>::Set;
    using CVariable<T>::m_valueDef;
    using CVariable<T>::m_onSetEnumFuncs;
    //////////////////////////////////////////////////////////////////////////
    CVariableEnum(){}

    //! Assign operator for variable.
    void operator=(const T& value) { CVariable<T>::SetValue(value); }

    //! Add new item to the enumeration.
    void AddEnumItem(const char* name, const T& value)
    {
        AddEnumItem(QString(name), value);
    };
    //! Add new item to the enumeration.
    void AddEnumItem(const QString& name, const T& value)
    {
        if (GetFlags() & IVariable::UI_USE_GLOBAL_ENUMS)  // don't think adding makes sense
        {
            return;
        }
        if (!m_enum)
        {
            m_enum = new CVarEnumList<T>();
        }
        m_enum->AddItem(name, value);
        OnEnumsChanged();
    };
    void SetEnumList(CVarEnumListBase<T>* enumList)
    {
        m_enum = enumList;
        OnSetValue(false);
        OnEnumsChanged();
    }
    IVarEnumList* GetEnumList() const
    {
        return m_enum;
    }
    //////////////////////////////////////////////////////////////////////////
    IVariable* Clone([[maybe_unused]] bool bRecursive) const
    {
        CVariableEnum<T>* var = new CVariableEnum<T>(*this);
        return var;
    }

    virtual QString GetDisplayValue() const
    {
        if (m_enum)
        {
            return m_enum->ValueToName(m_valueDef);
        }
        else
        {
            return CVariable<T>::GetDisplayValue();
        }
    }

    virtual void SetDisplayValue(const QString& value)
    {
        if (m_enum)
        {
            Set(m_enum->NameToValue(value));
        }
        else
        {
            Set(value);
        }
    }

protected:
    void OnEnumsChanged()
    {
        // Call on set callback.
        for (auto it = m_onSetEnumFuncs.begin(); it != m_onSetEnumFuncs.end(); ++it)
        {
            // Call on set callback.
            (*it)->operator()(this);
        }
    }

    // Copy Constructor.
    CVariableEnum(const CVariableEnum<T>& var)
        : CVariable<T>(var)
    {
        m_enum = var.m_enum;
    }
private:
    _smart_ptr<CVarEnumListBase<T> > m_enum;
};

//////////////////////////////////////////////////////////////////////////
// Smart pointers to variables.
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
template <class T, class TVar>
struct CSmartVariableBase
{
    typedef TVar VarType;

    CSmartVariableBase() { pVar = new VarType(); }

    operator T const& () const {
        VarType* pV = pVar;
        return *pV;
    }
    //void operator=( const T& value ) { VarType *pV = pVar; *pV = value; }

    // Compare with pointer to variable.
    bool operator==(IVariable* pV) { return pVar == pV; }
    bool operator!=(IVariable* pV) { return pVar != pV; }

    operator VarType&() {
        VarType* pV = pVar;
        return *pV;
    }                                                        // Cast to CVariableBase&
    VarType& operator*() const { return *pVar; }
    VarType* operator->() const { return pVar; }

    VarType* GetVar() const { return pVar; };

protected:
    _smart_ptr<VarType> pVar;
};

//////////////////////////////////////////////////////////////////////////
template <class T>
struct CSmartVariable
    : public CSmartVariableBase<T, CVariable<T> >
{
    using CSmartVariableBase<T, CVariable<T> >::pVar;
    void operator=(const T& value) { CVariable<T>* pV = pVar; *pV = value; }
};

//////////////////////////////////////////////////////////////////////////
template <class T>
struct CSmartVariableArrayT
    : public CSmartVariableBase<T, TVariableArray<T> >
{
    using CSmartVariableBase<T, TVariableArray<T> >::pVar;
    void operator=(const T& value) { TVariableArray<T>* pV = pVar; *pV = value; }
};

//////////////////////////////////////////////////////////////////////////
template <class T>
struct CSmartVariableEnum
    : public CSmartVariableBase<T, CVariableEnum<T> >
{
    using CSmartVariableBase<T, CVariableEnum<T> >::pVar;
    CSmartVariableEnum(){}
    void operator=(const T& value) { CVariableEnum<T>* pV = pVar; *pV = value; }
    void AddEnumItem(const QString& name, const T& value)
    {
        pVar->AddEnumItem(name, value);
    };
    void SetEnumList(CVarEnumListBase<T>* enumList)
    {
        pVar->EnableUpdateCallbacks(false);
        pVar->SetEnumList(enumList);
        pVar->EnableUpdateCallbacks(true);
    }
};

//////////////////////////////////////////////////////////////////////////
struct CSmartVariableArray
{
    typedef CVariableArray VarType;

    CSmartVariableArray() { pVar = new VarType(); }

    //////////////////////////////////////////////////////////////////////////
    // Access operators.
    //////////////////////////////////////////////////////////////////////////
    operator VarType&() {
        VarType* pV = pVar;
        return *pV;
    }

    VarType& operator*() const { return *pVar; }
    VarType* operator->() const { return pVar; }

    VarType* GetVar() const { return pVar; };

private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    _smart_ptr<VarType> pVar;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class EDITOR_CORE_API CVarBlock
    : public IVariableContainer
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    // Dtor.
    virtual ~CVarBlock() {}
    //! Add variable to block.
    void AddVariable(IVariable* var) override;
    //! Remove variable from block
    bool DeleteVariable(IVariable* var, bool bRecursive = false) override;

    void AddVariable(IVariable* pVar, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE);
    // This used from smart variable pointer.
    void AddVariable(CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE);

    //! Returns number of variables in block.
    int GetNumVariables() const override { return static_cast<int>(m_vars.size()); }

    //! Get pointer to stored variable by index.
    IVariable* GetVariable(int index) const override
    {
        assert(index >= 0 && index < m_vars.size());
        return m_vars[index];
    }

    // Clear all vars from VarBlock.
    void DeleteAllVariables() override { m_vars.clear(); };

    //! Return true if variable block is empty (Does not have any vars).
    bool IsEmpty() const override { return m_vars.empty(); }

    // Returns true if var block contains specified variable.
    bool IsContainsVariable(IVariable* pVar, bool bRecursive = true) const override;

    //! Find variable by name.
    IVariable* FindVariable(const char* name, bool bRecursive = true, bool bHumanName = false) const override;

    //////////////////////////////////////////////////////////////////////////
    //! Clone var block.
    CVarBlock* Clone(bool bRecursive) const;

    //////////////////////////////////////////////////////////////////////////
    //! Copy variable values from specified var block.
    //! Do not create new variables, only copy values of existing ones.
    //! Should only be used to copy identical var blocks (eg. not Array type var copied to String type var)
    //! @param fromVarBlock Source variable block that contain copied values, must be identical to this var block.
    void CopyValues(const CVarBlock* fromVarBlock);

    //////////////////////////////////////////////////////////////////////////
    //! Copy variable values from specified var block.
    //! Do not create new variables, only copy values of existing ones.
    //! Can be used to copy slightly different var blocks, matching performed by variable name.
    //! @param fromVarBlock Source variable block that contain copied values.
    void CopyValuesByName(CVarBlock* fromVarBlock);

    void OnSetValues();


    //////////////////////////////////////////////////////////////////////////
    //! Set UI_CREATE_SPLINE flags for all the varialbes in this Var Block.
    //! This is because when the data was changed for a spline variable, the spline need to be recreated to reflect the change.
    //! Set the flag to notify the spline variable.
    void SetRecreateSplines();

    //////////////////////////////////////////////////////////////////////////
    // Wire/Unwire other variable blocks.
    //////////////////////////////////////////////////////////////////////////
    //! Wire to other variable block.
    //! Only equivalent VarBlocks can be wired (same number of variables with same type).
    //! Recursive wiring of array variables is supported.
    void Wire(CVarBlock* toVarBlock);
    //! Unwire var block.
    void Unwire(CVarBlock* varBlock);

    //! Add this callback to every variable in block (recursively).
    void AddOnSetCallback(IVariable::OnSetCallback* func);
    //! Remove this callback from every variable in block (recursively).
    void RemoveOnSetCallback(IVariable::OnSetCallback* func);

    //////////////////////////////////////////////////////////////////////////
    void Serialize(XmlNodeRef vbNode, bool load);

    void ReserveNumVariables(int numVars);

    //////////////////////////////////////////////////////////////////////////
    //! Gather resources in this variable block.
    virtual void GatherUsedResources(CUsedResources& resources);

    void EnableUpdateCallbacks(bool boEnable);
    IVariable* FindChildVar(const char* name, IVariable* pParentVar) const;

    void Sort();

protected:
    void SetCallbackToVar(IVariable::OnSetCallback* func, IVariable* pVar, bool bAdd);
    void WireVar(IVariable* src, IVariable* trg, bool bWire);
    void GatherUsedResourcesInVar(IVariable* pVar, CUsedResources& resources);

    typedef std::vector<IVariablePtr> Variables;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Variables m_vars;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

typedef _smart_ptr<CVarBlock> CVarBlockPtr;

//////////////////////////////////////////////////////////////////////////
class EDITOR_CORE_API CVarObject
    : public QObject
    , public CRefCountBase
{
public:
    typedef IVariable::OnSetCallback VarOnSetCallback;

    CVarObject();
    ~CVarObject();

    void Serialize(XmlNodeRef node, bool load);
    CVarBlock* GetVarBlock() const { return m_vars; };

    void AddVariable(CVariableBase& var, const QString& varName, VarOnSetCallback* cb = nullptr, unsigned char dataType = IVariable::DT_SIMPLE);
    void AddVariable(CVariableBase& var, const QString& varName, const QString& varHumanName, VarOnSetCallback* cb = nullptr, unsigned char dataType = IVariable::DT_SIMPLE);
    void AddVariable(CVariableArray& table, CVariableBase& var, const QString& varName, const QString& varHumanName, VarOnSetCallback* cb = nullptr, unsigned char dataType = IVariable::DT_SIMPLE);
    void ReserveNumVariables(int numVars);
    void RemoveVariable(IVariable* var);

    void EnableUpdateCallbacks(bool boEnable);
    void OnSetValues();
protected:
    //! Copy values of variables from other VarObject.
    //! Source object must be of same type.
    void CopyVariableValues(CVarObject* sourceObject);

private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    CVarBlockPtr m_vars;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

Q_DECLARE_METATYPE(IVariable *);

#endif // CRYINCLUDE_EDITOR_UTIL_VARIABLE_H
