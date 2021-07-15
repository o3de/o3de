/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



// Description : Class that collects error reports to present them later.

#ifndef CRYINCLUDE_EDITOR_INTERFACE_ERRORREPORT_H
#define CRYINCLUDE_EDITOR_INTERFACE_ERRORREPORT_H
#pragma once

#include <IValidator.h>

// forward declarations.
class CParticleItem;
class CBaseObject;
class CBaseLibraryItem;
class CErrorRecord;

/*! Error report manages collection of errors occurred during map analyzes or level load.
 */
struct IErrorReport
    : public IValidator
{
    virtual ~IErrorReport(){}

    //! If enabled errors are reported immediately and not stored.
    virtual void SetImmediateMode(bool bEnable) = 0;

    virtual bool IsImmediateMode() const = 0;

    virtual void SetShowErrors(bool bShowErrors = true) = 0;

    //! Adds new error to report.
    virtual void ReportError(CErrorRecord& err) = 0;

    //! Check if error report have any errors.
    virtual bool IsEmpty() const = 0;

    //! Get number of contained error records.
    virtual int GetErrorCount() const = 0;

    //! Get access to indexed error record.
    virtual CErrorRecord& GetError(int i) = 0;

    //! Clear all error records.
    virtual void Clear() = 0;

    //! Display dialog with all errors.
    virtual void Display() = 0;

    //! Assign current Object to which new reported warnings are assigned.
    virtual void SetCurrentValidatorObject(CBaseObject* pObject) = 0;

    //! Assign current Item to which new reported warnings are assigned.
    virtual void SetCurrentValidatorItem(CBaseLibraryItem* pItem) = 0;

    //! Assign current filename.
    virtual void SetCurrentFile(const QString& file) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Implement IValidator interface.
    //////////////////////////////////////////////////////////////////////////
    virtual void Report(SValidatorRecord& record) = 0;
};


#endif // CRYINCLUDE_EDITOR_ERRORREPORT_H
