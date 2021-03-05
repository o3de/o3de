// Copyright 2014 The Crashpad Authors. All rights reserved.
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

#ifndef CRASHPAD_MINIDUMP_MINIDUMP_CONTEXT_WRITER_H_
#define CRASHPAD_MINIDUMP_MINIDUMP_CONTEXT_WRITER_H_

#include <sys/types.h>

#include <memory>

#include "base/macros.h"
#include "minidump/minidump_context.h"
#include "minidump/minidump_writable.h"

namespace crashpad {

struct CPUContext;
struct CPUContextX86;
struct CPUContextX86_64;

//! \brief The base class for writers of CPU context structures in minidump
//!     files.
class MinidumpContextWriter : public internal::MinidumpWritable {
 public:
  ~MinidumpContextWriter() override;

  //! \brief Creates a MinidumpContextWriter based on \a context_snapshot.
  //!
  //! \param[in] context_snapshot The context snapshot to use as source data.
  //!
  //! \return A MinidumpContextWriter subclass, such as MinidumpContextWriterX86
  //!     or MinidumpContextWriterAMD64, appropriate to the CPU type of \a
  //!     context_snapshot. The returned object is initialized using the source
  //!     data in \a context_snapshot. If \a context_snapshot is an unknown CPU
  //!     type’s context, logs a message and returns `nullptr`.
  static std::unique_ptr<MinidumpContextWriter> CreateFromSnapshot(
      const CPUContext* context_snapshot);

 protected:
  MinidumpContextWriter() : MinidumpWritable() {}

  //! \brief Returns the size of the context structure that this object will
  //!     write.
  //!
  //! \note This method will only be called in #kStateFrozen or a subsequent
  //!     state.
  virtual size_t ContextSize() const = 0;

  // MinidumpWritable:
  size_t SizeOfObject() final;

 private:
  DISALLOW_COPY_AND_ASSIGN(MinidumpContextWriter);
};

//! \brief The writer for a MinidumpContextX86 structure in a minidump file.
class MinidumpContextX86Writer final : public MinidumpContextWriter {
 public:
  MinidumpContextX86Writer();
  ~MinidumpContextX86Writer() override;

  //! \brief Initializes the MinidumpContextX86 based on \a context_snapshot.
  //!
  //! \param[in] context_snapshot The context snapshot to use as source data.
  //!
  //! \note Valid in #kStateMutable. No mutation of context() may be done before
  //!     calling this method, and it is not normally necessary to alter
  //!     context() after calling this method.
  void InitializeFromSnapshot(const CPUContextX86* context_snapshot);

  //! \brief Returns a pointer to the context structure that this object will
  //!     write.
  //!
  //! \attention This returns a non-`const` pointer to this object’s private
  //!     data so that a caller can populate the context structure directly.
  //!     This is done because providing setter interfaces to each field in the
  //!     context structure would be unwieldy and cumbersome. Care must be taken
  //!     to populate the context structure correctly. The context structure
  //!     must only be modified while this object is in the #kStateMutable
  //!     state.
  MinidumpContextX86* context() { return &context_; }

 protected:
  // MinidumpWritable:
  bool WriteObject(FileWriterInterface* file_writer) override;

  // MinidumpContextWriter:
  size_t ContextSize() const override;

 private:
  MinidumpContextX86 context_;

  DISALLOW_COPY_AND_ASSIGN(MinidumpContextX86Writer);
};

//! \brief The writer for a MinidumpContextAMD64 structure in a minidump file.
class MinidumpContextAMD64Writer final : public MinidumpContextWriter {
 public:
  MinidumpContextAMD64Writer();
  ~MinidumpContextAMD64Writer() override;

  // Ensure proper alignment of heap-allocated objects. This should not be
  // necessary in C++17.
  static void* operator new(size_t size);
  static void operator delete(void* ptr);

  // Prevent unaligned heap-allocated arrays. Provisions could be made to allow
  // these if necessary, but there is currently no use for them.
  static void* operator new[](size_t size) = delete;
  static void operator delete[](void* ptr) = delete;

  //! \brief Initializes the MinidumpContextAMD64 based on \a context_snapshot.
  //!
  //! \param[in] context_snapshot The context snapshot to use as source data.
  //!
  //! \note Valid in #kStateMutable. No mutation of context() may be done before
  //!     calling this method, and it is not normally necessary to alter
  //!     context() after calling this method.
  void InitializeFromSnapshot(const CPUContextX86_64* context_snapshot);

  //! \brief Returns a pointer to the context structure that this object will
  //!     write.
  //!
  //! \attention This returns a non-`const` pointer to this object’s private
  //!     data so that a caller can populate the context structure directly.
  //!     This is done because providing setter interfaces to each field in the
  //!     context structure would be unwieldy and cumbersome. Care must be taken
  //!     to populate the context structure correctly. The context structure
  //!     must only be modified while this object is in the #kStateMutable
  //!     state.
  MinidumpContextAMD64* context() { return &context_; }

 protected:
  // MinidumpWritable:
  size_t Alignment() override;
  bool WriteObject(FileWriterInterface* file_writer) override;

  // MinidumpContextWriter:
  size_t ContextSize() const override;

 private:
  MinidumpContextAMD64 context_;

  DISALLOW_COPY_AND_ASSIGN(MinidumpContextAMD64Writer);
};

}  // namespace crashpad

#endif  // CRASHPAD_MINIDUMP_MINIDUMP_CONTEXT_WRITER_H_
