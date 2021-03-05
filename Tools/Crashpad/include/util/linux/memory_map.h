// Copyright 2017 The Crashpad Authors. All rights reserved.
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

#ifndef CRASHPAD_UTIL_LINUX_MEMORY_MAP_H_
#define CRASHPAD_UTIL_LINUX_MEMORY_MAP_H_

#include <sys/types.h>

#include <string>
#include <vector>

#include "util/linux/address_types.h"
#include "util/linux/checked_linux_address_range.h"
#include "util/misc/initialization_state_dcheck.h"

namespace crashpad {

//! \brief Accesses information about mapped memory in another process.
//!
//! The target process must be stopped to guarantee correct mappings. If the
//! target process is not stopped, mappings may be invalid after the return from
//! Initialize(), and even mappings existing at the time Initialize() was called
//! may not be found.
class MemoryMap {
 public:
  //! \brief Information about a mapped region of memory.
  struct Mapping {
    Mapping();
    bool Equals(const Mapping& other) const;

    std::string name;
    CheckedLinuxAddressRange range;
    off_t offset;
    dev_t device;
    ino_t inode;
    bool readable;
    bool writable;
    bool executable;
    bool shareable;
  };

  MemoryMap();
  ~MemoryMap();

  //! \brief Initializes this object with information about the mapped memory
  //!     regions in the process whose ID is \a pid.
  //!
  //! This method must be called successfully prior to calling any other method
  //! in this class. This method may only be called once.
  //!
  //! \param[in] pid The process ID to obtain information for.
  //!
  //! \return `true` on success, `false` on failure with a message logged.
  bool Initialize(pid_t pid);

  //! \return The Mapping containing \a address or `nullptr` if no match is
  //!     found. The caller does not take ownership of this object. It is scoped
  //!     to the lifetime of the MemoryMap object that it was obtained from.
  const Mapping* FindMapping(LinuxVMAddress address) const;

  //! \return The Mapping with the lowest base address whose name is \a name or
  //!     `nullptr` if no match is found. The caller does not take ownership of
  //!     this object. It is scoped to the lifetime of the MemoryMap object that
  //!     it was obtained from.
  const Mapping* FindMappingWithName(const std::string& name) const;

  //! \brief Find the first Mapping in a series of mappings for the same file.
  //!
  //! Executables and libaries are typically loaded into several mappings with
  //! varying permissions for different segments. This method searches for the
  //! mapping with the highest address at or below \a mapping, which maps the
  //! same file as \a mapping from file offset 0.
  //!
  //! If \a mapping is not found, `nullptr` is returned. If \a mapping is found
  //! but does not map a file, \a mapping is returned.
  //!
  //! \param[in] mapping A Mapping whose series to find the start of.
  //! \return The first Mapping in the series or `nullptr` on failure with a
  //!     message logged.
  const Mapping* FindFileMmapStart(const Mapping& mapping) const;

 private:
  std::vector<Mapping> mappings_;
  InitializationStateDcheck initialized_;
};

}  // namespace crashpad

#endif  // CRASHPAD_UTIL_LINUX_MEMORY_MAP_H_
