/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

struct ICVar;
class XmlNodeRef;

namespace Config
{
    // Abstract configurable variable
    struct IConfigVar
    {
    public:
        enum EType
        {
            eType_BOOL,
            eType_INT,
            eType_FLOAT,
            eType_STRING,
        };

        enum EFlags
        {
            eFlag_NoUI = 1 << 0,
            eFlag_NoCVar = 1 << 1,
            eFlag_DoNotSave = 1 << 2,
        };

        IConfigVar(const char* szName, const char* szDescription, EType varType, AZ::u8 flags)
            : m_name(szName)
            , m_description(szDescription)
            , m_type(varType)
            , m_flags(flags)
            , m_ptr(nullptr)
        {};

        virtual ~IConfigVar() = default;

        AZ_FORCE_INLINE EType GetType() const
        {
            return m_type;
        }

        AZ_FORCE_INLINE const AZStd::string& GetName() const
        {
            return m_name;
        }

        AZ_FORCE_INLINE const AZStd::string& GetDescription() const
        {
            return m_description;
        }

        AZ_FORCE_INLINE bool IsFlagSet(EFlags flag) const
        {
            return 0 != (m_flags & flag);
        }

        virtual void Get(void* outPtr) const = 0;
        virtual void Set(const void* ptr) = 0;
        virtual bool IsDefault() const = 0;
        virtual void GetDefault(void* outPtr) const = 0;
        virtual void Reset() = 0;

        static constexpr EType TranslateType(const bool&) { return eType_BOOL; }
        static constexpr EType TranslateType(const int&) { return eType_INT; }
        static constexpr EType TranslateType(const float&) { return eType_FLOAT; }
        static constexpr EType TranslateType(const AZStd::string&) { return eType_STRING; }

    protected:
        EType m_type;
        AZ::u8 m_flags;
        AZStd::string m_name;
        AZStd::string m_description;
        void* m_ptr;
        ICVar* m_pCVar;
    };

    // Group of configuration variables with optional mapping to CVars
    class CConfigGroup
    {
    private:
        using TConfigVariables = AZStd::vector<IConfigVar*> ;
        TConfigVariables m_vars;

        using TConsoleVariables = AZStd::vector<ICVar*>;
        TConsoleVariables m_consoleVars;

    public:
        CConfigGroup();
        virtual ~CConfigGroup();

        void AddVar(IConfigVar* var);
        AZ::u32 GetVarCount();
        IConfigVar* GetVar(const char* szName);
        IConfigVar* GetVar(AZ::u32 index);
        const IConfigVar* GetVar(const char* szName) const;
        const IConfigVar* GetVar(AZ::u32 index) const;

        void SaveToXML(XmlNodeRef node);
        void LoadFromXML(XmlNodeRef node);
    };
};
