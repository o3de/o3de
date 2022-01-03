/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Class that collects error reports to present them later.


#ifndef CRYINCLUDE_EDITOR_ERRORREPORT_H
#define CRYINCLUDE_EDITOR_ERRORREPORT_H
#pragma once

// forward declarations.
class CParticleItem;

#include "BaseLibraryItem.h"
#include "Objects/BaseObject.h"
#include "Include/IErrorReport.h"
#include "ErrorRecorder.h"

/*! Single error entry in error report.
 */
class CErrorRecord
{
public:
    enum ESeverity
    {
        ESEVERITY_ERROR,
        ESEVERITY_WARNING,
        ESEVERITY_COMMENT
    };
    enum EFlags
    {
        FLAG_NOFILE         = 0x0001,   // Indicate that required file was not found.
        FLAG_SCRIPT         = 0x0002,   // Error with scripts.
        FLAG_TEXTURE        = 0x0004,   // Error with scripts.
        FLAG_OBJECTID       = 0x0008,   // Error with object Ids, Unresolved/Duplicate etc...
        FLAG_AI                 = 0x0010,   // Error with AI.
    };
    //! Severety of this error.
    ESeverity severity;
    //! Module of error.
    EValidatorModule module;
    //! Error Text.
    QString error;
    //! File which is missing or causing problem.
    QString file;
    //! More detailed description for this error.
    QString description;
    // Asset dependencies
    QString assetScope;
    int count;
    //! Object that caused this error.
    _smart_ptr<CBaseObject> pObject;
    //! Library Item that caused this error.
    _smart_ptr<CBaseLibraryItem> pItem;
    int flags;

    CErrorRecord(CBaseObject* object, ESeverity _severity, const QString& _error, int _flags = 0, int _count = 0,
        CBaseLibraryItem* item = 0, EValidatorModule _module = VALIDATOR_MODULE_EDITOR)
        : severity(_severity)
        , module(_module)
        , pObject(object)
        , pItem(item)
        , flags(_flags)
        , count(_count)
        , error(_error)
    {
    }

    CErrorRecord()
    {
        severity = ESEVERITY_WARNING;
        module = VALIDATOR_MODULE_EDITOR;
        pObject = 0;
        pItem = 0;
        flags = 0;
        count = 0;
    }
    QString GetErrorText() const;
};

/*! Error report manages collection of errors occured duruing map analyzes or level load.
 */
class CErrorReport
    : public IErrorReport
{
public:
    CErrorReport();

    //! If enabled errors are reported immidiatly and not stored.
    void SetImmediateMode(bool bEnable);
    bool IsImmediateMode() const { return m_bImmediateMode; };

    void SetShowErrors(bool bShowErrors = true) { m_bShowErrors = bShowErrors; };

    //! Adds new error to report.
    void ReportError(CErrorRecord& err);

    //! Check if error report have any errors.
    bool IsEmpty() const;

    //! Get number of contained error records.
    int GetErrorCount() const { return static_cast<int>(m_errors.size()); };
    //! Get access to indexed error record.
    CErrorRecord& GetError(int i);
    //! Clear all error records.
    void Clear();

    //! Display dialog with all errors.
    void Display();

    //! Assign current Object to which new reported warnings are assigned.
    void SetCurrentValidatorObject(CBaseObject* pObject);
    //! Assign current Item to which new reported warnings are assigned.
    void SetCurrentValidatorItem(CBaseLibraryItem* pItem);
    //! Assign current filename.
    void SetCurrentFile(const QString& file);

private:
    //! Array of all error records added to report.
    std::vector<CErrorRecord> m_errors;
    bool m_bImmediateMode;
    bool m_bShowErrors;
    _smart_ptr<CBaseObject> m_pObject;
    _smart_ptr<CBaseLibraryItem> m_pItem;
    CParticleItem* m_pParticle;
    QString m_currentFilename;
};


#endif // CRYINCLUDE_EDITOR_ERRORREPORT_H
