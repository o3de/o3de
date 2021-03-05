// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

/// @file  filesystem.h
///
/// @brief Utilities for dealing with file names and files portably.
///
/// Some helpful nomenclature:
///  -  "filename" - a file or directory name, relative or absolute
///  -  "searchpath" - a list of directories separated by ':' or ';'.
///


#pragma once

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include <OpenImageIO/span.h>
#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/string_view.h>

#if defined(_WIN32) && defined(__GLIBCXX__)
#    define OIIO_FILESYSTEM_USE_STDIO_FILEBUF 1
#    include <OpenImageIO/fstream_mingw.h>
#endif


// Define symbols that let client applications determine if newly added
// features are supported.
#define OIIO_FILESYSTEM_SUPPORTS_IOPROXY 1



OIIO_NAMESPACE_BEGIN

#if OIIO_FILESYSTEM_USE_STDIO_FILEBUF
// MingW uses GCC to build, but does not support having a wchar_t* passed as argument
// of ifstream::open or ofstream::open. To properly support UTF-8 encoding on MingW we must
// use the __gnu_cxx::stdio_filebuf GNU extension that can be used with _wfsopen and returned
// into a istream which share the same API as ifsteam. The same reasoning holds for ofstream.
typedef basic_ifstream<char> ifstream;
typedef basic_ofstream<char> ofstream;
#else
typedef std::ifstream ifstream;
typedef std::ofstream ofstream;
#endif

/// @namespace Filesystem
///
/// @brief Platform-independent utilities for manipulating file names,
/// files, directories, and other file system miscellany.

namespace Filesystem {

/// Return the filename (excluding any directories, but including the
/// file extension, if any) of a filepath.
OIIO_API std::string filename (const std::string &filepath) noexcept;

/// Return the file extension (including the last '.' if
/// include_dot=true) of a filename or filepath.
OIIO_API std::string extension (const std::string &filepath,
                                bool include_dot=true) noexcept;

/// Return all but the last part of the path, for example,
/// parent_path("foo/bar") returns "foo", and parent_path("foo")
/// returns "".
OIIO_API std::string parent_path (const std::string &filepath) noexcept;

/// Replace the file extension of a filename or filepath. Does not alter
/// filepath, just returns a new string.  Note that the new_extension
/// should contain a leading '.' dot.
OIIO_API std::string replace_extension (const std::string &filepath, 
                                        const std::string &new_extension) noexcept;

/// Turn a searchpath (multiple directory paths separated by ':' or ';')
/// into a vector<string> containing each individual directory.  If
/// validonly is true, only existing and readable directories will end
/// up in the list.  N.B., the directory names will not have trailing
/// slashes.
OIIO_API void searchpath_split (const std::string &searchpath,
                                std::vector<std::string> &dirs,
                                bool validonly = false);

/// Find the first instance of a filename existing in a vector of
/// directories, returning the full path as a string.  If the file is
/// not found in any of the listed directories, return an empty string.
/// If the filename is absolute, the directory list will not be used.
/// If testcwd is true, "." will be tested before the searchpath;
/// otherwise, "." will only be tested if it's explicitly in dirs.  If
/// recursive is true, the directories will be searched recursively,
/// finding a matching file in any subdirectory of the directories
/// listed in dirs; otherwise.
OIIO_API std::string searchpath_find (const std::string &filename,
                                      const std::vector<std::string> &dirs,
                                      bool testcwd = true,
                                      bool recursive = false);

/// Fill a vector-of-strings with the names of all files contained by
/// directory dirname.  If recursive is true, it will return all files
/// below the directory (even in subdirectories), but if recursive is
/// false (the default)If filter_regex is supplied and non-empty, only
/// filenames matching the regular expression will be returned.  Return
/// true if ok, false if there was an error (such as dirname not being
/// found or not actually being a directory).
OIIO_API bool get_directory_entries (const std::string &dirname,
                               std::vector<std::string> &filenames,
                               bool recursive = false,
                               const std::string &filter_regex=std::string());

/// Return true if the path is an "absolute" (not relative) path.
/// If 'dot_is_absolute' is true, consider "./foo" absolute.
OIIO_API bool path_is_absolute (string_view path,
                                bool dot_is_absolute=false);

/// Return true if the file exists.
///
OIIO_API bool exists (string_view path) noexcept;


/// Return true if the file exists and is a directory.
///
OIIO_API bool is_directory (string_view path) noexcept;

/// Return true if the file exists and is a regular file.
///
OIIO_API bool is_regular (string_view path) noexcept;

/// Create the directory. Return true for success, false for failure and
/// place an error message in err.
OIIO_API bool create_directory (string_view path, std::string &err);
inline bool create_directory (string_view path) {
    std::string err;
    return create_directory (path, err);
}

/// Copy a file, directory, or link. It is an error if 'to' already exists.
/// Return true upon success, false upon failure and place an error message
/// in err.
OIIO_API bool copy (string_view from, string_view to, std::string &err);
inline bool copy (string_view from, string_view to) {
    std::string err;
    return copy (from, to, err);
}

/// Rename (or move) a file, directory, or link.  Return true upon success,
/// false upon failure and place an error message in err.
OIIO_API bool rename (string_view from, string_view to, std::string &err);
inline bool rename (string_view from, string_view to) {
    std::string err;
    return rename (from, to, err);
}

/// Remove the file or directory. Return true for success, false for
/// failure and place an error message in err.
OIIO_API bool remove (string_view path, std::string &err);
inline bool remove (string_view path) {
    std::string err;
    return remove (path, err);
}

/// Remove the file or directory, including any children (recursively).
/// Return the number of files removed.  Place an error message (if
/// applicable in err.
OIIO_API unsigned long long remove_all (string_view path, std::string &err);
inline unsigned long long remove_all (string_view path) {
    std::string err;
    return remove_all (path, err);
}

/// Return a directory path where temporary files can be made.
///
OIIO_API std::string temp_directory_path ();

/// Return a unique filename suitable for making a temporary file or
/// directory.
OIIO_API std::string unique_path (string_view model="%%%%-%%%%-%%%%-%%%%");

/// Version of fopen that can handle UTF-8 paths even on Windows
///
OIIO_API FILE *fopen (string_view path, string_view mode);

/// Version of fseek that works with 64 bit offsets on all systems.
OIIO_API int fseek (FILE *file, int64_t offset, int whence);

/// Version of ftell that works with 64 bit offsets on all systems.
OIIO_API int64_t ftell (FILE *file);

/// Return the current (".") directory path.
///
OIIO_API std::string current_path ();

/// Version of std::ifstream.open that can handle UTF-8 paths
///
OIIO_API void open (OIIO::ifstream &stream, string_view path,
                    std::ios_base::openmode mode = std::ios_base::in);

/// Version of std::ofstream.open that can handle UTF-8 paths
///
OIIO_API void open (OIIO::ofstream &stream, string_view path,
                    std::ios_base::openmode mode = std::ios_base::out);


/// Read the entire contents of the named text file and place it in str,
/// returning true on success, false on failure.
OIIO_API bool read_text_file (string_view filename, std::string &str);

/// Read a maximum of n bytes from the named file, starting at position pos
/// (which defaults to the start of the file), storing results in
/// buffer[0..n-1]. Return the number of bytes read, which will be n for
/// full success, less than n if the file was fewer than n+pos bytes long,
/// or 0 if the file did not exist or could not be read.
OIIO_API size_t read_bytes (string_view path, void *buffer, size_t n,
                            size_t pos=0);

/// Get last modified time of file
///
OIIO_API std::time_t last_write_time (string_view path) noexcept;

/// Set last modified time on file
///
OIIO_API void last_write_time (string_view path, std::time_t time) noexcept;

/// Return the size of the file (in bytes), or uint64_t(-1) if there is any
/// error.
OIIO_API uint64_t file_size (string_view path) noexcept;

/// Ensure command line arguments are UTF-8 everywhere
///
OIIO_API void convert_native_arguments (int argc, const char *argv[]);

/// Turn a sequence description string into a vector of integers.
/// The sequence description can be any of the following
///  * A value (e.g., "3")
///  * A value range ("1-10", "10-1", "1-10x3", "1-10y3"):
///     START-FINISH        A range, inclusive of start & finish
///     START-FINISHxSTEP   A range with step size
///     START-FINISHySTEP   The complement of a stepped range, that is,
///                           all numbers within the range that would
///                           NOT have been selected by 'x'.
///     Note that START may be > FINISH, or STEP may be negative.
///  * Multiple values or ranges, separated by a comma (e.g., "3,4,10-20x2")
/// Retrn true upon success, false if the description was too malformed
/// to generate a sequence.
OIIO_API bool enumerate_sequence (string_view desc,
                                  std::vector<int> &numbers);

/// Given a pattern (such as "foo.#.tif" or "bar.1-10#.exr"), return a
/// normalized pattern in printf format (such as "foo.%04d.tif") and a
/// framespec (such as "1-10").
///
/// If framepadding_override is > 0, it overrides any specific padding amount
/// in the original pattern.
///
/// Return true upon success, false if the description was too malformed
/// to generate a sequence.
OIIO_API bool parse_pattern (const char *pattern,
                             int framepadding_override,
                             std::string &normalized_pattern,
                             std::string &framespec);


/// Given a normalized pattern (such as "foo.%04d.tif") and a list of frame
/// numbers, generate a list of filenames.
///
/// Return true upon success, false if the description was too malformed
/// to generate a sequence.
OIIO_API bool enumerate_file_sequence (const std::string &pattern,
                                       const std::vector<int> &numbers,
                                       std::vector<std::string> &filenames);

/// Given a normalized pattern (such as "foo_%V.%04d.tif") and a list of frame
/// numbers, generate a list of filenames. "views" is list of per-frame
/// views, or empty. In each frame filename, "%V" is replaced with the view,
/// and "%v" is replaced with the first character of the view.
///
/// Return true upon success, false if the description was too malformed
/// to generate a sequence.
OIIO_API bool enumerate_file_sequence (const std::string &pattern,
                                       const std::vector<int> &numbers,
                                       const std::vector<string_view> &views,
                                       std::vector<std::string> &filenames);

/// Given a normalized pattern (such as "/path/to/foo.%04d.tif") scan the
/// containing directory (/path/to) for matching frame numbers, views and files.
/// "%V" in the pattern matches views, while "%v" matches the first character
/// of each entry in views.
///
/// Return true upon success, false if the directory doesn't exist or the
/// pattern can't be parsed.
OIIO_API bool scan_for_matching_filenames (const std::string &pattern,
                                           const std::vector<string_view> &views,
                                           std::vector<int> &frame_numbers,
                                           std::vector<string_view> &frame_views,
                                           std::vector<std::string> &filenames);

/// Given a normalized pattern (such as "/path/to/foo.%04d.tif") scan the
/// containing directory (/path/to) for matching frame numbers and files.
///
/// Return true upon success, false if the directory doesn't exist or the
/// pattern can't be parsed.
OIIO_API bool scan_for_matching_filenames (const std::string &pattern,
                                           std::vector<int> &numbers,
                                           std::vector<std::string> &filenames);



/// Proxy class for I/O. This provides a simplified interface for file I/O
/// that can have custom overrides.
class OIIO_API IOProxy {
public:
    enum Mode { Closed = 0, Read = 'r', Write = 'w' };
    IOProxy () {}
    IOProxy (string_view filename, Mode mode)
        : m_filename(filename), m_mode(mode) {}
    virtual ~IOProxy () { }
    virtual const char* proxytype () const = 0;
    virtual void close () { }
    virtual bool opened () const { return mode() != Closed; }
    virtual int64_t tell () { return m_pos; }
    virtual bool seek (int64_t offset) { m_pos = offset; return true; }
    virtual size_t read (void *buf, size_t size) { return 0; }
    virtual size_t write (const void *buf, size_t size) { return 0; }
    // pread(), pwrite() are stateless, do not alter the current file
    // position, and are thread-safe (against each other).
    virtual size_t pread (void *buf, size_t size, int64_t offset) { return 0; }
    virtual size_t pwrite (const void *buf, size_t size, int64_t offset) { return 0; }
    virtual size_t size () const { return 0; }
    virtual void flush () const { }

    Mode mode () const { return m_mode; }
    const std::string& filename () const { return m_filename; }
    template<class T> size_t read (span<T> buf) {
        return read (buf.data(), buf.size()*sizeof(T));
    }
    template<class T> size_t write (span<T> buf) {
        return write (buf.data(), buf.size()*sizeof(T));
    }
    size_t write (string_view buf) {
        return write (buf.data(), buf.size());
    }
    bool seek (int64_t offset, int origin) {
        return seek ((origin == SEEK_SET ? offset : 0) +
                     (origin == SEEK_CUR ? offset+tell() : 0) +
                     (origin == SEEK_END ? size() : 0));
    }

#if OIIO_VERSION >= 20101
    #define OIIO_IOPROXY_HAS_ERROR 1
    std::string error() const { return m_error; }
    void error(string_view e) { m_error = e; }
#else
    std::string error() const { return ""; }
    void error(string_view e) { }
#endif

protected:
    std::string m_filename;
    int64_t m_pos = 0;
    Mode m_mode   = Closed;
#ifdef OIIO_IOPROXY_HAS_ERROR
    std::string m_error;
#endif
};


/// IOProxy subclass for reading or writing (but not both) that wraps C
/// stdio 'FILE'.
class OIIO_API IOFile : public IOProxy {
public:
    // Construct from a filename, open, own the FILE*.
    IOFile(string_view filename, Mode mode);
    // Construct from an already-open FILE* that is owned by the caller.
    // Caller is responsible for closing the FILE* after the proxy is gone.
    IOFile(FILE* file, Mode mode);
    virtual ~IOFile();
    virtual const char* proxytype() const { return "file"; }
    virtual void close();
    virtual bool seek(int64_t offset);
    virtual size_t read(void* buf, size_t size);
    virtual size_t write(const void* buf, size_t size);
    virtual size_t pread(void* buf, size_t size, int64_t offset);
    virtual size_t pwrite(const void* buf, size_t size, int64_t offset);
    virtual size_t size() const;
    virtual void flush() const;

    // Access the FILE*
    FILE* handle() const { return m_file; }

protected:
    FILE* m_file      = nullptr;
    size_t m_size     = 0;
    bool m_auto_close = false;
    std::mutex m_mutex;
};


/// IOProxy subclass for writing that wraps a std::vector<char> that will
/// grow as we write.
class OIIO_API IOVecOutput : public IOProxy {
public:
    // Construct, IOVecOutput owns its own vector.
    IOVecOutput()
        : IOProxy("", IOProxy::Write)
        , m_buf(m_local_buf)
    {
    }
    // Construct to wrap an existing vector.
    IOVecOutput(std::vector<unsigned char>& buf)
        : IOProxy("", Write)
        , m_buf(buf)
    {
    }
    virtual const char* proxytype() const { return "vecoutput"; }
    virtual size_t write(const void* buf, size_t size);
    virtual size_t pwrite(const void* buf, size_t size, int64_t offset);
    virtual size_t size() const { return m_buf.size(); }

    // Access the buffer
    std::vector<unsigned char>& buffer() const { return m_buf; }

protected:
    std::vector<unsigned char>& m_buf;       // reference to buffer
    std::vector<unsigned char> m_local_buf;  // our own buffer
    std::mutex m_mutex;                      // protect the buffer
};



/// IOProxy subclass for reading that wraps an cspan<char>.
class OIIO_API IOMemReader : public IOProxy {
public:
    IOMemReader(void* buf, size_t size)
        : IOProxy("", Read)
        , m_buf((const unsigned char*)buf, size)
    {
    }
    IOMemReader(cspan<unsigned char> buf)
        : IOProxy("", Read)
        , m_buf(buf.data(), buf.size())
    {
    }
    virtual const char* proxytype() const { return "memreader"; }
    virtual bool seek(int64_t offset)
    {
        m_pos = offset;
        return true;
    }
    virtual size_t read(void* buf, size_t size);
    virtual size_t pread(void* buf, size_t size, int64_t offset);
    virtual size_t size() const { return m_buf.size(); }

    // Access the buffer (caveat emptor)
    cspan<unsigned char> buffer() const noexcept { return m_buf; }

protected:
    cspan<unsigned char> m_buf;
};

};  // namespace Filesystem

OIIO_NAMESPACE_END
