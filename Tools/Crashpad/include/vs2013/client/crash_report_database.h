// Copyright 2015 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CRASHPAD_CLIENT_CRASH_REPORT_DATABASE_H_
#define CRASHPAD_CLIENT_CRASH_REPORT_DATABASE_H_

#include <time.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "util/file/file_io.h"
#include "util/misc/metrics.h"
#include "util/misc/uuid.h"

namespace crashpad {

class Settings;

//! \brief An interface for managing a collection of crash report files and
//!     metadata associated with the crash reports.
//!
//! All Report objects that are returned by this class are logically const.
//! They are snapshots of the database at the time the query was run, and the
//! data returned is liable to change after the query is executed.
//!
//! The lifecycle of a crash report has three stages:
//!
//!   1. New: A crash report is created with PrepareNewCrashReport(), the
//!      the client then writes the report, and then calls
//!      FinishedWritingCrashReport() to make the report Pending.
//!   2. Pending: The report has been written but has not been locally
//!      processed, or it was has been brought back from 'Completed' state by
//!      user request.
//!   3. Completed: The report has been locally processed, either by uploading
//!      it to a collection server and calling RecordUploadAttempt(), or by
//!      calling SkipReportUpload().
class CrashReportDatabase {
 public:
  //! \brief A crash report record.
  //!
  //! This represents the metadata for a crash report, as well as the location
  //! of the report itself. A CrashReportDatabase maintains at least this
  //! information.
  struct Report {
    Report();

    //! A unique identifier by which this report will always be known to the
    //! database.
    UUID uuid;

    //! The current location of the crash report on the client’s filesystem.
    //! The location of a crash report may change over time, so the UUID should
    //! be used as the canonical identifier.
    base::FilePath file_path;

    //! An identifier issued to this crash report by a collection server.
    std::string id;

    //! The time at which the report was generated.
    time_t creation_time;

    //! Whether this crash report was successfully uploaded to a collection
    //! server.
    bool uploaded;

    //! The last timestamp at which an attempt was made to submit this crash
    //! report to a collection server. If this is zero, then the report has
    //! never been uploaded. If #uploaded is true, then this timestamp is the
    //! time at which the report was uploaded, and no other attempts to upload
    //! this report will be made.
    time_t last_upload_attempt_time;

    //! The number of times an attempt was made to submit this report to
    //! a collection server. If this is more than zero, then
    //! #last_upload_attempt_time will be set to the timestamp of the most
    //! recent attempt.
    int upload_attempts;

    //! Whether this crash report was explicitly requested by user to be
    //! uploaded. This can be true only if report is in the 'pending' state.
    bool upload_explicitly_requested;
  };

  //! \brief A crash report that is in the process of being written.
  //!
  //! An instance of this struct should be created via PrepareNewCrashReport()
  //! and destroyed with FinishedWritingCrashReport().
  struct NewReport {
    //! The file handle to which the report should be written.
    FileHandle handle;

    //! A unique identifier by which this report will always be known to the
    //! database.
    UUID uuid;

    //! The path to the crash report being written.
    base::FilePath path;
  };

  //! \brief A scoper to cleanly handle the interface requirement imposed by
  //!     PrepareNewCrashReport().
  //!
  //! Calls ErrorWritingCrashReport() upon destruction unless disarmed by
  //! calling Disarm(). Armed upon construction.
  class CallErrorWritingCrashReport {
   public:
    //! \brief Arms the object to call ErrorWritingCrashReport() on \a database
    //!     with an argument of \a new_report on destruction.
    CallErrorWritingCrashReport(CrashReportDatabase* database,
                                NewReport* new_report);

    //! \brief Calls ErrorWritingCrashReport() if the object is armed.
    ~CallErrorWritingCrashReport();

    //! \brief Disarms the object so that CallErrorWritingCrashReport() will not
    //!     be called upon destruction.
    void Disarm();

   private:
    CrashReportDatabase* database_;  // weak
    NewReport* new_report_;  // weak

    DISALLOW_COPY_AND_ASSIGN(CallErrorWritingCrashReport);
  };

  //! \brief The result code for operations performed on a database.
  enum OperationStatus {
    //! \brief No error occurred.
    kNoError = 0,

    //! \brief The report that was requested could not be located.
    kReportNotFound,

    //! \brief An error occured while performing a file operation on a crash
    //!     report.
    //!
    //! A database is responsible for managing both the metadata about a report
    //! and the actual crash report itself. This error is returned when an
    //! error occurred when managing the report file. Additional information
    //! will be logged.
    kFileSystemError,

    //! \brief An error occured while recording metadata for a crash report or
    //!     database-wide settings.
    //!
    //! A database is responsible for managing both the metadata about a report
    //! and the actual crash report itself. This error is returned when an
    //! error occurred when managing the metadata about a crash report or
    //! database-wide settings. Additional information will be logged.
    kDatabaseError,

    //! \brief The operation could not be completed because a concurrent
    //!     operation affecting the report is occurring.
    kBusyError,

    //! \brief The report cannot be uploaded by user request as it has already
    //!     been uploaded.
    kCannotRequestUpload,
  };

  virtual ~CrashReportDatabase() {}

  //! \brief Opens a database of crash reports, possibly creating it.
  //!
  //! \param[in] path A path to the database to be created or opened. If the
  //!     database does not yet exist, it will be created if possible. Note that
  //!     for databases implemented as directory structures, existence refers
  //!     solely to the outermost directory.
  //!
  //! \return A database object on success, `nullptr` on failure with an error
  //!     logged.
  //!
  //! \sa InitializeWithoutCreating
  static std::unique_ptr<CrashReportDatabase> Initialize(
      const base::FilePath& path);

  //! \brief Opens an existing database of crash reports.
  //!
  //! \param[in] path A path to the database to be opened. If the database does
  //!     not yet exist, it will not be created. Note that for databases
  //!     implemented as directory structures, existence refers solely to the
  //!     outermost directory. On such databases, as long as the outermost
  //!     directory is present, this method will create the inner structure.
  //!
  //! \return A database object on success, `nullptr` on failure with an error
  //!     logged.
  //!
  //! \sa Initialize
  static std::unique_ptr<CrashReportDatabase> InitializeWithoutCreating(
      const base::FilePath& path);

  //! \brief Returns the Settings object for this database.
  //!
  //! \return A weak pointer to the Settings object, which is owned by the
  //!     database.
  virtual Settings* GetSettings() = 0;

  //! \brief Creates a record of a new crash report.
  //!
  //! Callers can then write the crash report using the file handle provided.
  //! The caller does not own the new crash report record or its file handle,
  //! both of which must be explicitly disposed of by calling
  //! FinishedWritingCrashReport() or ErrorWritingCrashReport().
  //!
  //! To arrange to call ErrorWritingCrashReport() during any early return, use
  //! CallErrorWritingCrashReport.
  //!
  //! \param[out] report A NewReport object containing a file handle to which
  //!     the crash report data should be written. Only valid if this returns
  //!     #kNoError. The caller must not delete the NewReport object or close
  //!     the file handle within.
  //!
  //! \return The operation status code.
  virtual OperationStatus PrepareNewCrashReport(NewReport** report) = 0;

  //! \brief Informs the database that a crash report has been written.
  //!
  //! After calling this method, the database is permitted to move and rename
  //! the file at NewReport::path.
  //!
  //! \param[in] report A NewReport obtained with PrepareNewCrashReport(). The
  //!     NewReport object and file handle within will be invalidated as part of
  //!     this call.
  //! \param[out] uuid The UUID of this crash report.
  //!
  //! \return The operation status code.
  virtual OperationStatus FinishedWritingCrashReport(NewReport* report,
                                                     UUID* uuid) = 0;

  //! \brief Informs the database that an error occurred while attempting to
  //!     write a crash report, and that any resources associated with it should
  //!     be cleaned up.
  //!
  //! After calling this method, the database is permitted to remove the file at
  //! NewReport::path.
  //!
  //! \param[in] report A NewReport obtained with PrepareNewCrashReport(). The
  //!     NewReport object and file handle within will be invalidated as part of
  //!     this call.
  //!
  //! \return The operation status code.
  virtual OperationStatus ErrorWritingCrashReport(NewReport* report) = 0;

  //! \brief Returns the crash report record for the unique identifier.
  //!
  //! \param[in] uuid The crash report record unique identifier.
  //! \param[out] report A crash report record. Only valid if this returns
  //!     #kNoError.
  //!
  //! \return The operation status code.
  virtual OperationStatus LookUpCrashReport(const UUID& uuid,
                                            Report* report) = 0;

  //! \brief Returns a list of crash report records that have not been uploaded.
  //!
  //! \param[out] reports A list of crash report record objects. This must be
  //!     empty on entry. Only valid if this returns #kNoError.
  //!
  //! \return The operation status code.
  virtual OperationStatus GetPendingReports(std::vector<Report>* reports) = 0;

  //! \brief Returns a list of crash report records that have been completed,
  //!     either by being uploaded or by skipping upload.
  //!
  //! \param[out] reports A list of crash report record objects. This must be
  //!     empty on entry. Only valid if this returns #kNoError.
  //!
  //! \return The operation status code.
  virtual OperationStatus GetCompletedReports(std::vector<Report>* reports) = 0;

  //! \brief Obtains a report object for uploading to a collection server.
  //!
  //! The file at Report::file_path should be uploaded by the caller, and then
  //! the returned Report object must be disposed of via a call to
  //! RecordUploadAttempt().
  //!
  //! A subsequent call to this method with the same \a uuid is illegal until
  //! RecordUploadAttempt() has been called.
  //!
  //! \param[in] uuid The unique identifier for the crash report record.
  //! \param[out] report A crash report record for the report to be uploaded.
  //!     The caller does not own this object. Only valid if this returns
  //!     #kNoError.
  //!
  //! \return The operation status code.
  virtual OperationStatus GetReportForUploading(const UUID& uuid,
                                                const Report** report) = 0;

  //! \brief Adjusts a crash report record’s metadata to account for an upload
  //!     attempt, and updates the last upload attempt time as returned by
  //!     Settings::GetLastUploadAttemptTime().
  //!
  //! After calling this method, the database is permitted to move and rename
  //! the file at Report::file_path.
  //!
  //! \param[in] report The report object obtained from
  //!     GetReportForUploading(). This object is invalidated after this call.
  //! \param[in] successful Whether the upload attempt was successful.
  //! \param[in] id The identifier assigned to this crash report by the
  //!     collection server. Must be empty if \a successful is `false`; may be
  //!     empty if it is `true`.
  //!
  //! \return The operation status code.
  virtual OperationStatus RecordUploadAttempt(const Report* report,
                                              bool successful,
                                              const std::string& id) = 0;

  //! \brief Moves a report from the pending state to the completed state, but
  //!     without the report being uploaded.
  //!
  //! This can be used if the user has disabled crash report collection, but
  //! crash generation is still enabled in the product.
  //!
  //! \param[in] uuid The unique identifier for the crash report record.
  //! \param[in] reason The reason the report upload is being skipped for
  //!     metrics tracking purposes.
  //!
  //! \return The operation status code.
  virtual OperationStatus SkipReportUpload(
      const UUID& uuid,
      Metrics::CrashSkippedReason reason) = 0;

  //! \brief Deletes a crash report file and its associated metadata.
  //!
  //! \param[in] uuid The UUID of the report to delete.
  //!
  //! \return The operation status code.
  virtual OperationStatus DeleteReport(const UUID& uuid) = 0;

  //! \brief Marks a crash report as explicitly requested to be uploaded by the
  //!     user and moves it to 'pending' state.
  //!
  //! \param[in] uuid The unique identifier for the crash report record.
  //!
  //! \return The operation status code.
  virtual OperationStatus RequestUpload(const UUID& uuid) = 0;

 protected:
  CrashReportDatabase() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CrashReportDatabase);
};

}  // namespace crashpad

#endif  // CRASHPAD_CLIENT_CRASH_REPORT_DATABASE_H_
