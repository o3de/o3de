/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IConsole.h>
#include <ISystem.h>
#include <AzCore/std/function/function_template.h>

class CXConsole;

class CXConsoleVariableBase
    : public ICVar
{
public:
    //! constructor
    //! \param pConsole must not be 0
    CXConsoleVariableBase(CXConsole* pConsole, const char* sName, int nFlags, const char* help);
    //! destructor
    virtual ~CXConsoleVariableBase();

    // interface ICVar --------------------------------------------------------------------------------------

    void ClearFlags(int flags) override;
    int GetFlags() const override;
    int SetFlags(int flags) override;
    const char* GetName() const override;
    const char* GetHelp() override;
    void Release() override;
    void ForceSet(const char* s) override;
    void SetOnChangeCallback(ConsoleVarFunc pChangeFunc) override;
    bool AddOnChangeFunctor(AZ::Name functorName, const AZStd::function<void()>& pChangeFunctor) override;
    ConsoleVarFunc GetOnChangeCallback() const override;

    bool ShouldReset() const { return (m_nFlags & VF_RESETTABLE) != 0; }
    void Reset() override
    {
        if (ShouldReset())
        {
            ResetImpl();
        }
    }

    virtual void ResetImpl() = 0;

    void SetLimits(float min, float max) override;
    void GetLimits(float& min, float& max) override;
    bool HasCustomLimits() override;

    int GetRealIVal() const override { return GetIVal(); }
    bool IsConstCVar() const override {return (m_nFlags & VF_CONST_CVAR) != 0; }
    void SetDataProbeString(const char* pDataProbeString) override
    {
        CRY_ASSERT(m_pDataProbeString == NULL);
        m_pDataProbeString = new char[ strlen(pDataProbeString) + 1 ];
        azstrcpy(m_pDataProbeString, strlen(pDataProbeString) + 1, pDataProbeString);
    }
    const char* GetDataProbeString() const override;

protected: // ------------------------------------------------------------------------------------------

    virtual const char* GetOwnDataProbeString() const
    {
        return GetString();
    }

    void CallOnChangeFunctions();

    char*                      m_szName;                                            // if VF_COPYNAME then this data need to be deleteed, otherwise it's pointer to .dll/.exe

    char*                      m_psHelp;                                            // pointer to the help string, might be 0
    char*            m_pDataProbeString;            // value client is required to have for data probes
    int                             m_nFlags;                                           // e.g. VF_CHEAT, ...

    using ChangeFunctorContainer = AZStd::unordered_map<AZ::Name, AZStd::function<void ()>>;
    ChangeFunctorContainer m_changeFunctors;
    ConsoleVarFunc    m_pChangeFunc;                // Callback function that is called when this variable changes.
    CXConsole*             m_pConsole;                                      // used for the callback OnBeforeVarChange()

    float m_valueMin;
    float m_valueMax;
    bool m_hasCustomLimits;
};

//////////////////////////////////////////////////////////////////////////
class CXConsoleVariableString
    : public CXConsoleVariableBase
{
public:
    // constructor
    CXConsoleVariableString(CXConsole* pConsole, const char* sName, const char* szDefault, int nFlags, const char* help)
        : CXConsoleVariableBase(pConsole, sName, nFlags, help)
    {
        if (szDefault)
        {
            m_sValue = szDefault;
            m_sDefault = szDefault;
        }
    }

    // interface ICVar --------------------------------------------------------------------------------------

    int GetIVal() const override { return atoi(m_sValue.c_str()); }
    int64 GetI64Val() const override { return _atoi64(m_sValue.c_str()); }
    float GetFVal() const override { return (float)atof(m_sValue.c_str()); }
    const char* GetString() const override { return m_sValue.c_str(); }
    void ResetImpl() override
    {
        Set(m_sDefault.c_str());
    }
    void Set(const char* s) override;
    void Set(float f) override;
    void Set(int i) override;
    int GetType() override { return CVAR_STRING; }

private: // --------------------------------------------------------------------------------------------
    AZStd::string m_sValue;
    AZStd::string m_sDefault;                                                                              //!<
};



class CXConsoleVariableInt
    : public CXConsoleVariableBase
{
public:
    // constructor
    CXConsoleVariableInt(CXConsole* pConsole, const char* sName, const int iDefault, int nFlags, const char* help)
        : CXConsoleVariableBase(pConsole, sName, nFlags, help)
        , m_iValue(iDefault)
        , m_iDefault(iDefault)
    {
    }

    // interface ICVar --------------------------------------------------------------------------------------

    int GetIVal() const override { return m_iValue; }
    int64 GetI64Val() const override { return m_iValue; }
    float GetFVal() const override { return (float)GetIVal(); }
    const char* GetString() const override;
    void ResetImpl() override { Set(m_iDefault); }
    void Set(const char* s) override;
    void Set(float f) override;
    void Set(int i) override;
    int GetType() override { return CVAR_INT; }
protected: // --------------------------------------------------------------------------------------------

    int                             m_iValue;
    int                             m_iDefault;                                 //!<
};

//////////////////////////////////////////////////////////////////////////
class CXConsoleVariableFloat
    : public CXConsoleVariableBase
{
public:
    // constructor
    CXConsoleVariableFloat(CXConsole* pConsole, const char* sName, const float fDefault, int nFlags, const char* help)
        : CXConsoleVariableBase(pConsole, sName, nFlags, help)
        , m_fValue(fDefault)
        , m_fDefault(fDefault)
    {
    }

    // interface ICVar --------------------------------------------------------------------------------------

    int GetIVal() const override { return (int)m_fValue; }
    int64 GetI64Val() const override { return (int64)m_fValue; }
    float GetFVal() const override { return m_fValue; }
    const char* GetString() const override;
    void ResetImpl() override { Set(m_fDefault); }
    void Set(const char* s) override;
    void Set(float f) override;
    void Set(int i) override;
    int GetType() override { return CVAR_FLOAT; }

protected:

    const char* GetOwnDataProbeString() const override
    {
        static char szReturnString[8];

        sprintf_s(szReturnString, "%.1g", m_fValue);
        return szReturnString;
    }

private: // --------------------------------------------------------------------------------------------

    float                           m_fValue;
    float                           m_fDefault;                             //!<
};


class CXConsoleVariableIntRef
    : public CXConsoleVariableBase
{
public:
    //! constructor
    //!\param pVar must not be 0
    CXConsoleVariableIntRef(CXConsole* pConsole, const char* sName, int32* pVar, int nFlags, const char* help)
        : CXConsoleVariableBase(pConsole, sName, nFlags, help)
        , m_iValue(*pVar)
        , m_iDefault(*pVar)
    {
        assert(pVar);
    }

    // interface ICVar --------------------------------------------------------------------------------------

    int GetIVal() const override { return m_iValue; }
    int64 GetI64Val() const override { return m_iValue; }
    float GetFVal() const override { return (float)m_iValue; }
    const char* GetString() const override;
    void ResetImpl() override { Set(m_iDefault); }
    void Set(const char* s) override;
    void Set(float f) override;
    void Set(int i) override;
    int GetType() override { return CVAR_INT; }

private: // --------------------------------------------------------------------------------------------

    int&                           m_iValue;
    int                            m_iDefault;                                  //!<
};

class CXConsoleVariableStringRef
    : public CXConsoleVariableBase
{
public:
    //! constructor
    //!\param userBuf must not be 0
    CXConsoleVariableStringRef(CXConsole* pConsole, const char* sName, const char** userBuf, const char* defaultValue, int nFlags, const char* help)
        : CXConsoleVariableBase(pConsole, sName, nFlags, help)
        , m_sValue(defaultValue)
        , m_sDefault(defaultValue)
        , m_userPtr(*userBuf)
    {
        m_userPtr = m_sValue.c_str();
        assert(userBuf);
    }

    // interface ICVar --------------------------------------------------------------------------------------

    int GetIVal() const override { return atoi(m_sValue.c_str()); }
    int64 GetI64Val() const override { return _atoi64(m_sValue.c_str()); }
    float GetFVal() const override { return (float)atof(m_sValue.c_str()); }
    const char* GetString() const override
    {
        return m_sValue.c_str();
    }
    void ResetImpl() override { Set(m_sDefault.c_str()); }
    void Set(const char* s) override;
    void Set(float f) override
    {
        AZStd::fixed_string<32> s = AZStd::fixed_string<32>::format("%g", f);
        Set(s.c_str());
    }
    void Set(int i) override
    {
        AZStd::fixed_string<32> s = AZStd::fixed_string<32>::format("%d", i);
        Set(s.c_str());
    }
    int GetType() override { return CVAR_STRING; }

private: // --------------------------------------------------------------------------------------------

    AZStd::string m_sValue;
    AZStd::string m_sDefault;
    const char*& m_userPtr;                                         //!<
};

class CXConsoleVariableFloatRef
    : public CXConsoleVariableBase
{
public:
    //! constructor
    //!\param pVar must not be 0
    CXConsoleVariableFloatRef(CXConsole* pConsole, const char* sName, float* pVar, int nFlags, const char* help)
        : CXConsoleVariableBase(pConsole, sName, nFlags, help)
        , m_fValue(*pVar)
        , m_fDefault(*pVar)
    {
        assert(pVar);
    }

    // interface ICVar --------------------------------------------------------------------------------------

    int GetIVal() const override { return (int)m_fValue; }
    int64 GetI64Val() const override { return (int64)m_fValue; }
    float GetFVal() const override { return m_fValue; }
    const char* GetString() const override;
    void ResetImpl() override { Set(m_fDefault); }
    void Set(const char* s) override;
    void Set(float f) override;
    void Set(int i) override;
    int GetType() override { return CVAR_FLOAT; }

protected:

    const char *GetOwnDataProbeString() const override
    {
        static char szReturnString[8];

        sprintf_s(szReturnString, "%.1g", m_fValue);
        return szReturnString;
    }

private: // --------------------------------------------------------------------------------------------

    float&                         m_fValue;
    float                          m_fDefault;                                  //!<
};
