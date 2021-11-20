/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>

namespace AZ::IO
{
    //! Implementation of a Path class for providing an abstraction of paths on the file system
    //! This class represents paths provides an abstraction for paths on a filesystem
    //! Only the syntactic parts of a path are handled. The actual paths stored may
    //! represent non-existing paths or paths that are not possible on the current OS/filesystem
    //! A path name is made up of the following segments
    //! # root-name - identifies the root of a filesystem(such as C: or "//myserver")
    //!   This is normally only set for paths on Windows OS
    //! # root-directory - A directory separator, which if present marks the path as absolute
    //!   if missing and the first-element other than the root-name is a filename, then this
    //!   represents a relative path
    //! * zero or more of the following:
    //! # filename - sequence of characters that aren't directory separators or preferred directory
    //!   separators. Filenames may identify a file, hard link, symbolic link, or directory
    //! # directory-separators - the forward slash character '/' or the preferred separator character
    //!   On windows the preferred separator is '\'
    //!   If this character is repeated it is treated as a single directory separator
    //!   i.e /usr///////lib is the same as /usr/lib
    //!
    //! Paths can be traversed elmeent wise using the begin()/end() functions
    //! This views the path in the generic format and iterators over the path in order of
    //! root-name -> root-directory -> file name(0 or more times)
    //! directory separators are skipped except for the one that represents the root directory
    //! A iterated path element is never empty
    class PathView
    {
    public:
        using string_view_type = AZStd::string_view;
        using value_type = char;
        using const_iterator = const PathIterator<PathView>;
        using iterator = const_iterator;
        friend PathIterator<PathView>;

        // constructors and destructor
        constexpr PathView() = default;
        constexpr PathView(const PathView& other) = default;

        //! Constructs a PathView by the storing the supplied string_view
        //! The preferred separator is to the OS default path separator
        constexpr PathView(AZStd::string_view pathView) noexcept;
        //! Constructs a PathView by the storing the supplied string_view
        //! The preferred separator it set to the parameter
        constexpr PathView(AZStd::string_view pathView, const char preferredSeparator) noexcept;
        //! Constructs a PathView by storing the value_type* into a string_view
        //! The preferred separator is to the OS default path separator
        constexpr PathView(const value_type* pathString) noexcept;
        //! Constructs a PathView by storing the value_type* into a string_view
        //! The preferred separator it set to the parameter
        constexpr PathView(const value_type* pathString, const char preferredSeparator) noexcept;
        // Constructs an empty PathView with the preferred separator set to the parameter value
        explicit constexpr PathView(const char preferredSeparator) noexcept;

        // assignments
        // copy assignment operators
        constexpr PathView& operator=(const PathView& other) = default;

        constexpr PathView& operator=(AZStd::string_view pathView) noexcept;
        constexpr PathView& operator=(const value_type* pathView) noexcept;

        // modifiers
        constexpr void swap(PathView& rhs) noexcept;

        // native format observers
        //! Returns string_view stored within the PathView
        constexpr AZStd::string_view Native() const noexcept;
        //! Conversion operator to retrieve string_view stored within the PathView
        constexpr explicit operator AZStd::string_view() const noexcept;

        // compare
        //! Performs a compare of each of the path parts for equivalence
        //! Each part of the path is compare using string comparison
        //! Ex: Comparing "test/foo" against "test/fop" returns -1;
        //! Path separators of the contained path string aren't compared
        //! Ex. Comparing "C:/test\foo" against C:\test/foo" returns 0;
        constexpr int Compare(const PathView& other) const noexcept;
        constexpr int Compare(AZStd::string_view pathString) const noexcept;
        constexpr int Compare(const value_type* pathString) const noexcept;

        // Extension for fixed strings
        //! extension: fixed string types with MaxPathLength capacity
        //! Returns a new instance of an AZStd::fixed_string with capacity of MaxPathLength
        //! made from the internal string
        constexpr AZStd::fixed_string<MaxPathLength> FixedMaxPathString() const noexcept;

        // as_posix
        //! Replicates the behavior of the Python pathlib as_posix method
        //! by replacing the Windows Path Separator with the Posix Path Seperator
        constexpr AZStd::fixed_string<MaxPathLength> FixedMaxPathStringAsPosix() const noexcept;

        // decomposition
        //! Given a windows path of "C:\O3DE\foo\bar\name.txt" and a posix path of
        //! "/O3DE/foo/bar/name.txt"
        //! The following functions return the following

        //! Returns the root name part of the path. if it has one.
        //! This is the part of the path before the '/' root directory part
        //! windows = "C:", posix = ""
        constexpr PathView RootName() const;
        //! Returns the root directory part of the path if it has one.
        //! This is the root directory separator
        //! windows = "\", posix = "/"
        constexpr PathView RootDirectory() const;
        //! Returns the full root path portion of the path made as if it joined
        //! the root_name() and root_directory()
        //! windows = "C:\", posix = "/"
        constexpr PathView RootPath() const;
        //! Returns the relative path portion of the path.
        //! This contains the path parts after the root path
        //! windows = "O3DE\foo\bar\name.txt", posix = "O3DE/foo/bar/name.txt"
        constexpr PathView RelativePath() const;
        //! Returns the parent directory of filename contained within the path
        //! windows = "C:\O3DE\foo\bar", posix = "/O3DE/foo/bar"
        //! NOTE: If the path ends with a trailing separator "test/foo/" it is treated as being a path of
        //! "test/foo" as if the filename of the path is "foo" and the parent directory is "test"
        constexpr PathView ParentPath() const;
        //! Returns the filename of the path
        //! windows = "name.txt", posix = "name.txt"
        constexpr PathView Filename() const;
        //! Returns the filename of the path stripped of it's extension
        //! windows = "name", posix = "name"
        //! NOTE: If the filename starts with a "." it is treated as part of the stem
        //! i.e "home/user/.vimrc"; The stem porttion is ".vimrc" in this case
        constexpr PathView Stem() const;
        //! Returns the extension of the filename
        //! (or equivalently, the filename of the path with the stem stripped)
        //! windows = ".txt", posix = ".txt"
        //! NOTE: If the filename is the special "." or ".." path then their is no extension in that case
        constexpr PathView Extension() const;

        // query
        //! Checks if the contained string type element is empty
        [[nodiscard]] constexpr bool empty() const noexcept;

        //! Checks if the path has a root name
        //! For posix paths this is always empty
        //! For windows paths this is the part of the path before the either the root directory
        //! or relative path part
        [[nodiscard]] constexpr bool HasRootName() const;
        //! Checks if the path has a root directory
        [[nodiscard]] constexpr bool HasRootDirectory() const;
        //! Checks whether the entire root path portion of the path is empty
        //! The root portion of the path is made up of root_name() / root_directory()
        [[nodiscard]] constexpr bool HasRootPath() const;
        //! checks whether the relative part of path is empty
        //! (C:\\     O3DE\dev\)
        //!    ^            ^
        //! root part    relative part
        [[nodiscard]] constexpr bool HasRelativePath() const;
        //! checks whether the path has a parent path that  empty
        [[nodiscard]] constexpr bool HasParentPath() const;
        //! Checks whether the filename is empty
        [[nodiscard]] constexpr bool HasFilename() const;
        //! Checks whether the stem of the filename is empty <stem>[.<ext>]
        [[nodiscard]] constexpr bool HasStem() const;
        //! Checks whether the extension of the filename is empty <stem>[.<ext>]
        [[nodiscard]] constexpr bool HasExtension() const;

        //! Checks whether the contained path represents an absolute path
        //! If a path starts with a '/', "<drive letter>:\", "\\<network_name>\",
        //! "\\?\", "\??\" or "\\.\"
        [[nodiscard]] constexpr bool IsAbsolute() const;
        //! Check whether the path is not absolute
        [[nodiscard]] constexpr bool IsRelative() const;
        //! Check whether the path is relative to the base path
        [[nodiscard]] constexpr bool IsRelativeTo(const PathView& base) const;

        //! Normalizes a path in a purely lexical manner.
        //! # Path separators are converted to their preferred path separator
        //! # Path parts of "." are collapsed to nothing empty
        //! # Paths parts of ".." are removed if there is a preceding directory
        //! The preceding directory is also removed
        //! # Runs of Two or more path separators are collapsed into one path separator
        //! unless the path begins with two path separators
        //! # Finally any trailing path separators are removed
        constexpr FixedMaxPath LexicallyNormal() const;

        //! Make the path relative to the supplied base PathView, by using the following rules.
        //! # If root_name() != base.root_name() is true, return a default constructed path
        //!  (Path can't relative if the root_names are different i.e C: vs F:)
        //! # If is_absolute() != base.is_absolute() is true, return a default constructed path
        //!   (A relative path can't be formed out of when one path is absolute and the other isn't)
        //! # If !has_root_directory() && base.has_root_directory() is true return a default constructed path
        //!   (Again if one path contains a root directory and the other doesn't a relative path can't be formed
        //! # if any filename in the relative_path() or the base.relative_path() can be interpreted as a root-name
        //!   return a default constructed path
        //!   (This is the occasion of having a "path" such as "foo/bar/C:/foo/bar". Such a path cannot be made relative
        //! # Otherwise determine the first mismatch of path parts between *this path and the base as if using
        //!   auto [a, b] = AZStd::mismatch(begin(), end(), base.begin(), base.end())
        //! ## Then if a == end() and b == base.end(), then the paths are equal, return a relative path of "."(represents the same path)
        //! ## Otherwise define N as the number of non-empty filename elements that are not "." or ".." in [b, base.end()] minus
        //!    the number of ".." elements. N is less than 0 then return a default constructed path
        //!    (This is the case of all the parent directories of base being consumed by the "..". Such as a path of "foo/../bar/../..")
        //! ## If N == 0 and a == end() or a->empty(), then the paths are equal and retruns a path of "."
        //!    (This is the case of a making "a/b/c" relative to a path such as "a/b/c/d/.."
        //! ## Finally if all the above conditions fail then a path is composed of a default constructed path followed by
        //!    N applications of the Path append operator on "..", followed by an application of the path append operator for each
        //!    element in the range of a[a, end()]
        //!    (Example of this case is making "/a/d" relative to "/a/b/c". The mismatch happens on "d" for *this and b for base
        //!    the number of path elements remaining from "b" to "base.end" is 2("b", "c"). Therefore two applications of ".." is formed
        //!    So the path at this point is "../..", then the remaining elements in the mismatched range for *this is then append
        //!    There is only 1 element in the range of [a, end()] and that is "d", so the final path becomes "../../d"
        constexpr FixedMaxPath LexicallyRelative(const PathView& base) const;

        //! Returns the lexically relative path if it is not an empty path, otherwise it returns the current *this path
        constexpr FixedMaxPath LexicallyProximate(const PathView& base) const;

        // match
        //! Returns true if given pattern matches against the path using glob-style pattern expansion
        //! The Match algorithm works as follows
        //! # If pattern when converted to a PathView results in an empty, return false
        //! If the pattern is relative, then the path can be either absolute or relative and matching
        //! is done from the end
        //! Ex. AZ::IO::PathView("a/b.ext").Match("*.ext") -> True
        //!     AZ::IO::PathView("a/b/c.ext").Match("b/*.ext") -> True
        //!     AZ::IO::PathView("a/b/c.ext").Match("a/*.ext") -> False
        //!
        //! # If the pattern is absolute, then the path must be absolute and the whole path must match
        //! is done from the end
        //! Ex. AZ::IO::PathView("/a.ext").Match("/*.ext") -> True
        //!     AZ::IO::PathView("a/b.ext").Match("/*.ext") -> False
        //!
        //! # a glob containing '*' will match a single component of a path
        //! Ex. AZ::IO::PathView("a/b/c.ext").Match("a/*/c.ext") -> True
        //! Ex. AZ::IO::PathView("a/d/e.ext").Match("a/*/*.ext") -> True
        //! Ex. AZ::IO::PathView("a/g/h.ext").Match("a/*/f.ext") -> False
        [[nodiscard]] constexpr bool Match(AZStd::string_view pathPattern) const;

        // iterators
        //! Returns an iterator to the beginning of the path that can be used to traverse the path
        //! according to the following
        //! 1. Root name - (0 or 1)
        //! 2. Root directory - (0 or 1)
        //! 3. Filename - (0 or more)
        //! 4. Trailing separator - (0 or 1)
        constexpr const_iterator begin() const;
        //! Returns an iterator to the end of the path element
        //! This iterator can be safely decremented to point to the last valid path element
        //! if it is not equal to the begin iterator
        constexpr const_iterator end() const;

    private:
        template <typename StringType>
        friend class BasicPath;
        friend struct AZStd::hash<PathView>;
        struct PathIterable;

        static constexpr void MakeRelativeTo(PathIterable& pathResult, const AZ::IO::PathView& path, const AZ::IO::PathView& base) noexcept;

        //! Returns a structure that provides a view of the path parts which can be used for iteration
        //! Only the path parts that correspond to creating an normalized path is returned
        //! This function is useful for returning a "view" into a normalized path without the need
        //! to allocate memory for the heap
        static constexpr PathIterable GetNormalPathParts(const AZ::IO::PathView& path) noexcept;
        //! joins the input path to the Path Iterable structure using similiar logic to Path::Append
        //! If the input path is absolute it will replace the current PathIterable otherwise
        //! the input path will be appended to the Path Iterable structure
        //! For example a PathIterable with parts = ['C:', '/', 'foo']
        //! If the path input = 'bar', then the new PathIterable parts = [C:', '/', 'foo', 'bar']
        //! If the path input = 'C:/bar', then the new PathIterable parts = [C:', '/', 'bar']
        //! If the path input = 'C:bar', then the new PathIterable parts = [C:', '/', 'foo', 'bar' ]
        //! If the path input = 'D:bar', then the new PathIterable parts = [D:, 'bar' ]
        static constexpr void AppendNormalPathParts(PathIterable& pathIterableResult, const AZ::IO::PathView& path) noexcept;

        constexpr int ComparePathView(const PathView& other) const;
        constexpr AZStd::string_view root_name_view() const;
        constexpr AZStd::string_view root_directory_view() const;
        constexpr AZStd::string_view root_path_raw_view() const;
        constexpr AZStd::string_view relative_path_view() const;
        constexpr AZStd::string_view parent_path_view() const;
        constexpr AZStd::string_view filename_view() const;
        constexpr AZStd::string_view stem_view() const;
        constexpr AZStd::string_view extension_view() const;

        AZStd::string_view m_path;
        value_type m_preferred_separator = AZ_TRAIT_OS_PATH_SEPARATOR;
    };

    constexpr void swap(PathView& lhs, PathView& rhs) noexcept;

    //! Hashes a value in a manner that if two paths are equal,
    //! then their hash values are also equal
    //! For example : path "a//b" equals  "a/b", the
    //! hash value of "a//b" would also equal the hash value of "a/b"
    size_t hash_value(const PathView& pathToHash) noexcept;

    // path.comparison
    constexpr bool operator==(const PathView& lhs, const PathView& rhs) noexcept;
    constexpr bool operator!=(const PathView& lhs, const PathView& rhs) noexcept;
    constexpr bool operator<(const PathView& lhs, const PathView& rhs) noexcept;
    constexpr bool operator<=(const PathView& lhs, const PathView& rhs) noexcept;
    constexpr bool operator>(const PathView& lhs, const PathView& rhs) noexcept;
    constexpr bool operator>=(const PathView& lhs, const PathView& rhs) noexcept;
}

namespace AZ::IO
{
    template <typename StringType>
    class BasicPath
    {
    public:
        using string_type = StringType;
        using value_type = typename StringType::value_type;
        using traits_type = typename StringType::traits_type;
        using string_view_type = AZStd::string_view;
        using const_iterator = const PathIterator<BasicPath>;
        using iterator = const_iterator;
        friend PathIterator<BasicPath>;
        friend struct PathReflection;

        // constructors and destructor
        constexpr BasicPath() = default;
        constexpr BasicPath(const BasicPath& other) = default;
        constexpr BasicPath(BasicPath&& other) = default;

        // Conversion constructor for other types of BasicPath instantiations
        constexpr BasicPath(const PathView& other) noexcept;

        // String constructors
        //! Constructs a Path by copying the pathString to its internal string
        //! The preferred separator is to the OS default path separator
        constexpr BasicPath(const string_type& pathString) noexcept;
        //! Constructs a Path by copying the pathString to its internal string
        //! The preferred separator is set to the parameter
        constexpr BasicPath(const string_type& pathString, const char preferredSeparator) noexcept;
        //! Constructs a Path by moving the pathString to its internal string
        //! The preferred separator is to the OS default path separator
        constexpr BasicPath(string_type&& pathString) noexcept;
        //! Constructs a Path by copying the pathString to its internal string
        //! The preferred separator is set to the parameter
        constexpr BasicPath(string_type&& pathString, const char preferredSeparator) noexcept;
        //! Constructs a Path by constructing it's internal out of a string_view
        //! The preferred separator is to the OS default path separator
        constexpr BasicPath(AZStd::string_view src) noexcept;
        //! Constructs a Path by constructing it's internal out of a string_view
        //! The preferred separator is set to the parameter
        constexpr BasicPath(AZStd::string_view src, const char preferredSeparator) noexcept;
        //! Constructs a Path by constructing it's internal out of a value_type*
        //! The preferred separator is to the OS default path separator
        constexpr BasicPath(const value_type* pathString) noexcept;
        //! Constructs a Path by constructing it's internal out of a value_type*
        //! The preferred separator is set to the parameter
        constexpr BasicPath(const value_type* pathString, const char preferredSeparator) noexcept;
        //! Constructs a empty Path with the preferred separator set to the parameter
        explicit constexpr BasicPath(const char preferredSeparator) noexcept;

        template <typename InputIt>
        constexpr BasicPath(InputIt first, InputIt last);
        template <typename InputIt>
        constexpr BasicPath(InputIt first, InputIt last, const char preferredSeparator);

        // Conversion operator to PathView
        constexpr operator PathView() const noexcept;

        // assignments
        // copy assignment operators
        constexpr BasicPath& operator=(const BasicPath& other) = default;

         // move assignment operator
        constexpr BasicPath& operator=(BasicPath&& other) = default;

        // conversion assignment operator
        constexpr BasicPath& operator=(const PathView& pathView) noexcept;
        constexpr BasicPath& operator=(const string_type& str) noexcept;
        constexpr BasicPath& operator=(string_type&& str) noexcept;
        constexpr BasicPath& operator=(AZStd::string_view str) noexcept;
        constexpr BasicPath& operator=(const value_type* src) noexcept;
        constexpr BasicPath& operator=(value_type src) noexcept;

        //! Replaces the contents of the path using the character sequence
        //! from the supplied parameters. No special conversions take place.
        //! It is either a string copy or move
        constexpr BasicPath& Assign(const PathView& pathView) noexcept;
        constexpr BasicPath& Assign(const string_type& pathView) noexcept;
        constexpr BasicPath& Assign(string_type&& pathView) noexcept;
        constexpr BasicPath& Assign(AZStd::string_view pathView) noexcept;
        constexpr BasicPath& Assign(const value_type* src) noexcept;
        constexpr BasicPath& Assign(value_type src) noexcept;
        template <typename InputIt>
        constexpr BasicPath& Assign(InputIt first, InputIt last);


        // appends
        //! Joins a path together using the following rule set
        //! let p = other PathView
        //! # If p.is_absolute() || (p.has_root_name() && p.root.name() != root_name()
        //! Then replaces the current *this path with p
        //! NOTE: This is the case of trying to append something such as
        //! "C:\Foo" / "F:\Foo" = "F:\Foo"
        //! IMPORTANT NOTE: this also applies for the root directory as well
        //! So appending "test/foo" "/bar" results in "/bar", not "test/foo/bar"
        //! If "test/foo/bar" is desired then either operator+=/concat can be used with "/bar"
        //! or leading path separator in bar is skipped over so that only "bar" is passed to operator+=
        //!
        //! # Otherwise If p.has_root_directory(), then removes any root directory and the entire relative path
        //!   from *this
        //! These address the the following cases:
        //! 1. "C:foo" / "/bar" = "C:/bar"
        //! 2. "C:/foo" / "/charlie" = "C:/charlie", not "C:/foo/charlie"
        //! If "C:/foo/charlie" is desired then the leading path separator
        //! in "/charlie" can be skipped over before being passed to operator/=
        //!
        //! # Else if for *this path has_filename() || (!has_root_directory() && is_absolute()), appends
        //! the preferred path separator followed by the p using string operations
        //! This function absolute path behavior is consistent with the Python pathlib PurePath module
        //! https://docs.python.org/3/library/pathlib.html#pathlib.PurePath
        constexpr BasicPath& operator/=(const PathView& other);
        constexpr BasicPath& operator/=(const string_type& src);
        constexpr BasicPath& operator/=(AZStd::string_view src);
        constexpr BasicPath& operator/=(const value_type* src);
        constexpr BasicPath& operator/=(value_type src);

        //! Sames as the operator/= functions above
        constexpr BasicPath& Append(const PathView& src);
        constexpr BasicPath& Append(const string_type& pathView);
        constexpr BasicPath& Append(AZStd::string_view src);
        constexpr BasicPath& Append(const value_type* src);
        constexpr BasicPath& Append(value_type src);
        template <typename InputIt>
        constexpr BasicPath& Append(InputIt first, InputIt last);

        // modifiers
        //! Invokes clean on the underlying path string type
        constexpr void clear() noexcept;

        //! Replaces all path separators with the preferred separator value
        constexpr BasicPath& MakePreferred();

        //! Removes the filename portion of the Path
        //! NOTE: the filename portion of the path is the part of after last path separator
        //! What this means is that calling remove filename on "foo/bar" returns "foo/"
        //! and calling remove filename on "foo/" returns "foo/"
        //! (Treat the path of being made of two components "foo" and "")
        //! What this means is that remove filename does not change the number of path parts
        constexpr BasicPath& RemoveFilename();
        //! Replaces the filename portion of the Path as if invoking remove filename on the path
        //! followed by invoking the append operator/= with the replacement filename
        constexpr BasicPath& ReplaceFilename(const PathView& replacementFilename);
        //! Replaces the extension of the filename with the supplied parameter.
        //! This is done as if by first removing the extension from the *this path
        //! appending a "." character to the filename if the replacement extension doesn't begin with one
        //! followed by concatenating the extension via operator+=
        constexpr BasicPath& ReplaceExtension(const PathView& replacementExtension = {});

        constexpr void swap(BasicPath& rhs) noexcept;

        // native format observers
        constexpr const string_type& Native() const& noexcept;
        constexpr const string_type&& Native() const&& noexcept;
        constexpr const value_type* c_str() const noexcept;
        constexpr explicit operator string_type() const;

        // Adds support for retrieving a modifiable copy of the underlying string
        // Any modifications to the string invalidates existing PathIterators
        constexpr string_type& Native() & noexcept;
        constexpr string_type&& Native() && noexcept;

        //! The string and wstring functions cannot be constexpr until AZStd::basic_string is made constexpr.
        //! This cannot occur until C++20 as operator new/delete cannot be used within constexpr functions
        //! Returns a new instance of an AZStd::string made from the internal string
        AZStd::string String() const;

        // Extension for fixed strings
        //! extension: fixed string types with MaxPathLength capacity
        //! Returns a new instance of an AZStd::fixed_string with capacity of MaxPathLength
        //! made from the internal string
        constexpr AZStd::fixed_string<MaxPathLength> FixedMaxPathString() const;

        // as_posix
        //! Replicates the behavior of the Python pathlib as_posix method
        //! by replacing the Windows Path Separator with the Posix Path Seperator
        AZStd::string StringAsPosix() const;
        constexpr AZStd::fixed_string<MaxPathLength> FixedMaxPathStringAsPosix() const noexcept;

        // compare
        //! Performs a compare of each of the path parts for equivalence
        //! Each part of the path is compare using string comparison
        //! If both *this path and the input path uses the WindowsPathSeparator
        //! then a non-case sensitive compare is performed
        //! Ex: Comparing "test/foo" against "test/fop" returns -1;
        //! Path separators of the contained path string aren't compared
        //! Ex. Comparing "C:/test\foo" against C:\test/foo" returns 0;
        constexpr int Compare(const PathView& other) const noexcept;
        constexpr int Compare(const string_type& pathString) const;
        constexpr int Compare(AZStd::string_view pathString) const noexcept;
        constexpr int Compare(const value_type* pathString) const noexcept;

        // decomposition
        //! Returns the root name part of the path. if it has one.
        //! This is the part of the path before the '/' root directory part
        //! windows = "C:", posix = ""
        constexpr PathView RootName() const;
        //! Returns the root directory part of the path if it has one.
        //! This is the root directory separator
        //! windows = "\", posix = "/"
        constexpr PathView RootDirectory() const;
        //! Returns the full root path portion of the path made as if it joined
        //! the root_name() and root_directory()
        //! windows = "C:\", posix = "/"
        constexpr PathView RootPath() const;
        //! Returns the relative path portion of the path.
        //! This contains the path parts after the root path
        //! windows = "O3DE\foo\bar\name.txt", posix = "O3DE/foo/bar/name.txt"
        constexpr PathView RelativePath() const;
        //! Returns the parent directory of filename contained within the path
        //! windows = "C:\O3DE\foo\bar", posix = "/O3DE/foo/bar"
        //! NOTE: If the path ends with a trailing separator "test/foo/" it is treated as being a path of
        //! "test/foo" as if the filename of the path is "foo" and the parent directory is "test"
        constexpr PathView ParentPath() const;
        //! Returns the filename of the path
        //! windows = "name.txt", posix = "name.txt"
        constexpr PathView Filename() const;
        //! Returns the filename of the path stripped of it's extension
        //! windows = "name", posix = "name"
        //! NOTE: If the filename starts with a "." it is treated as part of the stem
        //! i.e "home/user/.vimrc"; The stem portion is ".vimrc" in this case
        constexpr PathView Stem() const;
        //! Returns the extension of the filename
        //! (or equivalently, the filename of the path with the stem stripped)
        //! windows = ".txt", posix = ".txt"
        //! NOTE: If the filename is the special "." or ".." path then their is no extension in that case
        constexpr PathView Extension() const;

        // query
        //! Checks if the contained string type element is empty
        [[nodiscard]] constexpr bool empty() const noexcept;

        //! Checks if the path has a root name
        //! For posix paths this is always empty
        //! For windows paths this is the part of the path before the either the root directory
        //! or relative path part
        [[nodiscard]] constexpr bool HasRootName() const;
        //! Checks if the path has a root directory
        [[nodiscard]] constexpr bool HasRootDirectory() const;
        //! Checks whether the entire root path portion of the path is empty
        //! The root portion of the path is made up of root_name() / root_directory()
        [[nodiscard]] constexpr bool HasRootPath() const;
        //! checks whether the relative part of path is empty
        //! (C:\\     O3DE\dev\)
        //!    ^          ^
        //! root part    relative part
        [[nodiscard]] constexpr bool HasRelativePath() const;
        //! checks whether the path has a parent path that  empty
        [[nodiscard]] constexpr bool HasParentPath() const;
        //! Checks whether the filename is empty
        [[nodiscard]] constexpr bool HasFilename() const;
        //! Checks whether the stem of the filename is empty <stem>[.<ext>]
        [[nodiscard]] constexpr bool HasStem() const;
        //! Checks whether the extension of the filename is empty <stem>[.<ext>]
        [[nodiscard]] constexpr bool HasExtension() const;

        //! Checks whether the contained path represents an absolute path
        //! If a path starts with a '/', "<drive letter>:\", "\\<network_name>\",
        //! "\\?\", "\??\" or "\\.\"
        [[nodiscard]] constexpr bool IsAbsolute() const;
        //! Check whether the path is not absolute
        [[nodiscard]] constexpr bool IsRelative() const;
        //! Check whether the path is relative to the base path
        [[nodiscard]] constexpr bool IsRelativeTo(const PathView& base) const;

        // decomposition
        //! Given a windows path of "C:\O3DE\foo\bar\name.txt" and a posix path of
        //! "/O3DE/foo/bar/name.txt"
        //! The following functions return the following

        // query

        // relative paths
        //! Normalizes a path in a purely lexical manner.
        //! # Path separators are converted to their preferred path separator
        //! # Path parts of "." are collapsed to nothing empty
        //! # Paths parts of ".." are removed if there is a preceding directory
        //! The preceding directory is also removed
        //! # Runs of Two or more path separators are collapsed into one path separator
        //! unless the path begins with two path separators
        //! # Finally any trailing path separators are removed
        constexpr BasicPath LexicallyNormal() const;

        //! Make the path relative to the supplied base PathView, by using the following rules.
        //! # If root_name() != base.root_name() is true, return a default constructed path
        //!  (Path can't relative if the root_names are different i.e C: vs F:)
        //! # If is_absolute() != base.is_absolute() is true, return a default constructed path
        //!   (A relative path can't be formed out of when one path is absolute and the other isn't)
        //! # If !has_root_directory() && base.has_root_directory() is true return a default constructed path
        //!   (Again if one path contains a root directory and the other doesn't a relative path can't be formed
        //! # if any filename in the relative_path() or the base.relative_path() can be interpreted as a root-name
        //!   return a default constructed path
        //!   (This is the occasion of having a "path" such as "foo/bar/C:/foo/bar". Such a path cannot be made relative
        //! # Otherwise determine the first mismatch of path parts between *this path and the base as if using
        //!   auto [a, b] = AZStd::mismatch(begin(), end(), base.begin(), base.end())
        //! ## Then if a == end() and b == base.end(), then the paths are equal, return a relative path of "."(represents the same path)
        //! ## Otherwise define N as the number of non-empty filename elements that are not "." or ".." in [b, base.end()] minus
        //!    the number of ".." elements. N is less than 0 then return a default constructed path
        //!    (This is the case of all the parent directories of base being consumed by the "..". Such as a path of "foo/../bar/../..")
        //! ## If N == 0 and a == end() or a->empty(), then the paths are equal and retruns a path of "."
        //!    (This is the case of a making "a/b/c" relative to a path such as "a/b/c/d/.."
        //! ## Finally if all the above conditions fail then a path is composed of a default constructed path followed by
        //!    N applications of the Path append operator on "..", followed by an application of the path append operator for each
        //!    element in the range of a[a, end()]
        //!    (Example of this case is making "/a/d" relative to "/a/b/c". The mismatch happens on "d" for *this and b for base
        //!    the number of path elements remaining from "b" to "base.end" is 2("b", "c"). Therefore two applications of ".." is formed
        //!    So the path at this point is "../..", then the remaining elements in the mismatched range for *this is then append
        //!    There is only 1 element in the range of [a, end()] and that is "d", so the final path becomes "../../d"
        constexpr BasicPath LexicallyRelative(const PathView& base) const;

        //! Returns the lexically relative path if it is not an empty path, otherwise it returns the current *this path
        constexpr BasicPath LexicallyProximate(const PathView& base) const;

        //! See PathView::Match for documentation on how to use
        [[nodiscard]] constexpr bool Match(AZStd::string_view pathPattern) const;

        // iterators
        //! Returns an iterator to the beginning of the path that can be used to traverse the path
        //! according to the following
        //! 1. Root name - (0 or 1)
        //! 2. Root directory - (0 or 1)
        //! 3. Filename - (0 or more)
        constexpr const_iterator begin() const;
        //! Returns an iterator to the end of the path element
        //! This iterator can be safely decremented to point to the last valid path element
        //! if it is not equal to the begin iterator
        constexpr const_iterator end() const;

    private:
        friend class PathView;

        string_type m_path;
        value_type m_preferred_separator = AZ_TRAIT_OS_PATH_SEPARATOR;
    };

    template <typename StringType>
    constexpr void swap(BasicPath<StringType>& lhs, BasicPath<StringType>& rhs) noexcept;

    //! Hashes a value in a manner that if two paths are equal,
    //! then their hash values are also equal
    //! For example : path "a//b" equals  "a/b", the
    //! hash value of "a//b" would also equal the hash value of "a/b"
    template <typename StringType>
    size_t hash_value(const BasicPath<StringType>& pathToHash);

    // path.append
    template <typename StringType>
    constexpr BasicPath<StringType> operator/(const BasicPath<StringType>& lhs, const PathView& rhs);
    template <typename StringType>
    constexpr BasicPath<StringType> operator/(const BasicPath<StringType>& lhs, AZStd::string_view rhs);
    template <typename StringType>
    constexpr BasicPath<StringType> operator/(const BasicPath<StringType>& lhs, const typename BasicPath<StringType>::value_type* rhs);
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AZ::IO::Path, "{88E0A40F-3085-4CAB-8B11-EF5A2659C71A}");
}

namespace AZ::IO
{
    //! Path iterator that allows traversal of the path elements
    //! Example iterations of path can be seen in the PathIteratorFixture in the PathTests.
    //! For example the following path with a R"(\\server\share\users\abcdef\AppData\Local\Temp)", with a path separator of '\\',
    //! Iterates the following elements {R"(\\server)", R"(\)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
    template <typename PathType>
    class PathIterator
    {
    public:
        enum ParserState : uint8_t
        {
            Singular,
            BeforeBegin,
            InRootName,
            InRootDir,
            InFilenames,
            AtEnd
        };

        friend PathType;

        using iterator_category = AZStd::bidirectional_iterator_tag;
        using value_type = PathType;
        using difference_type = ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        using stashing_iterator_tag = void; // A reverse_iterator cannot be used with a stashing iterator
        // As PathView::iterator stores the current path view within it, undefined behavior occurs
        // when a reverse iterator is used

        constexpr PathIterator() = default;
        constexpr PathIterator(const PathIterator&) = default;

        constexpr PathIterator& operator=(const PathIterator&) = default;

        constexpr reference operator*() const;

        constexpr pointer operator->() const;

        constexpr PathIterator& operator++();

        constexpr PathIterator operator++(int);

        constexpr PathIterator& operator--();

        constexpr PathIterator operator--(int);

    private:
        template <typename PathType1>
        friend constexpr bool operator==(const PathIterator<PathType1>& lhs, const PathIterator<PathType1>& rhs);
        template <typename PathType1>
        friend constexpr bool operator!=(const PathIterator<PathType1>& lhs, const PathIterator<PathType1>& rhs);

        constexpr PathIterator& increment();
        constexpr PathIterator& decrement();

        value_type m_stashed_elem;
        const value_type* m_path_ref{};
        AZStd::string_view m_path_entry_view;
        ParserState m_state{ Singular };
    };

    template <typename PathType1>
    constexpr bool operator==(const PathIterator<PathType1>& lhs, const PathIterator<PathType1>& rhs);
    template <typename PathType1>
    constexpr bool operator!=(const PathIterator<PathType1>& lhs, const PathIterator<PathType1>& rhs);
}

#include <AzCore/IO/Path/Path.inl>
