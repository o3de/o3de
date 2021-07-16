/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZStd
{
    /**
    * \mainpage
    * Welcome to the AZStd (Amazon STD) library.
    *
    * The first and very important step if you never used AZStd is to run: \b \e "azstd\debugger tools\smartdebug.exe". To read more
    * why you need to run this tool, go to \ref Debugging "Debugging".
    *
    * If you are integrating AZStd to your project, go to \ref Setup.
    *
    * Check the latest \ref ReleaseNotes "release notes" for this version of AZSTD.
    *
    * You can read design and tips on AZStd components \ref AZStd "AZStd Overview"
    *
    * Or if you can't wait, jump to the \ref AZStdExamples "code examples" to see AZStd in action.
    *
    */

    /**
     * \page AZStd AZStd Overview
     *
     * AZStd (Double Helix STD) is designed to solve most of problems that STL/STD has in the game development world. The library is 100% conforming ( except \ref Allocators ) with
     * the \ref CStd and \ref CTR1. In addition it introduces useful extensions.
     * \note At the time of reading not all STD/TR1 features might be available in AZStd, it is work in progress.
     *
     * AZStd to STD major differences and extensions:
     * \li We use difference \ref Allocators specification. It is simpler, faster, better for debug (named allocators) and handles aligned allocations.
     * \li Temporary and static \ref Allocators allow fast temp container processing, data is never freed (and destroyed for POD).
     * \li Introduces net sets of fixed and intrusive \ref Containers.
     * \li \ref Containers handle alignment properly. Each container has "node_type" which you can use with sizeof(node_type) and alignment_of<node_type>::value.
     * \li \ref Containers never allocate memory for if they are empty. All containers with dynamic allocation a leak_and_reset functionality.
     * \li All \ref Containers iterators had checked versions \ref CheckedIterators which provide great error checking.
     * \li \ref Containers have variety of extensions like: lazy insert, lazy find, insert with source copy, etc. Knowing of them can improve the performance significantly.
     * \li Debug: AZStd do not use deep function calls. This improves performance in debug build, solve problems with compilers not that good with inlining. Of course
     * this has the downside that the AZStd code is harder to maintain, but this should not happen often at all.
     * \li Debug: AZStd is designed as much as possible for easy \ref Debugging.
     *
     * \anchor ADL
     * We write our code to be readable and consistent as possible. AZStd is ADL (Argument dependent lookup / Koenig lookup) safe, this allows mixing AZStd with other
     * STD implementations (which can be somewhat challenging with some implementations - \ref ADLHelperMacros), we do offer solutions for most issues that will arise.
     * All code is compiled without any warnings on the highest warning level for each compiler.
     *
     * We do some platform dependent optimizations.
     *
     * Here is the different AZStd components:
     *
     * \li \subpage Allocators
     *
     * \li \subpage Containers
     *
     * \li \subpage Algorithms
     *
     * \li \subpage TypeTraits
     *
     * \li \subpage Functors
     *
     * \li \subpage Parallel
     *
     * \li \subpage Debugging
     *
     * \li \ref AZStdExamples "Code examples"
     *
     * \section References
     * \li EASTL: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html#cpp_standard
     * \anchor C14IDEAS
     * \li 14 crazy ideas for the standard library in C++0x: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1870.html
     *
     *
     * \subsection CStd C++ standard
     * C++ standard ISO/IEC 14882 2003 update : \n
     *   \li local: \verbatim \\oc-chateau\matrix\programming\c++\C++ standard 2003.pdf \endverbatim
     *   \li i-net: http://www.open-std.org/jtc1/sc22/wg21/
     *
     * \subsection CTR1 TR1
     * C++ library extentions (TR1): http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1836.pdf \n
     * \subsection C++0x C0x
     * Working Draft, Standard for Programming Language C++
     * \li local: \verbatim \\oc-chateau\matrix\programming\c++\C++0x n2914.pdf \endverbatim
     * \li i-net: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2914.pdf
     */

    /**
     * \page Setup AZSTD Setup
     *
     * After you have the AZStd code, you need to make sure you project has add the include path to the folder where AZStd was installed. All AZStd include files are using based on
     * AZStd parent folder. For instance all includes are like this \e AZStd/base.h,"AzCore/std/containers/vector,h",etc. We use use the AZStd to avoid name collisions with other
     * stl implementations and make it obvious where the included file comes from.
     * If you decide to use the default allocator (AZStd::allocator) you will need to implement AZStd::Default_Alloc and AZStd::Default_Free functions otherwise you should
     * use your own allocator.
     * Due to some spefics of the CStd we need in an internal allocators (AZStd::functions and AZStd::threads)... we need to use system memory. So you are required to Create the AZ::SystemAllocator
     * if you use them.
     *
     * In many games and memory managed evironments, you do whant to have faster debug build (for STD usage), different default allocators and so on. This what this Setup section is mainly about.
     * You can used different macros and policies to control the AZStd behavior. So if you need to tweak AZStd, I suggest you create a config file in your project (ex. AZStd_Config.h) and make
     * sure this file is always included before using AZStd (precompiler header).
     * \n\n \b IMPORTANT: All configuration defines you do should happen before you include \e AZStd/base.h
     *
     * Defines you can use to configure AZStd:
     *
     * \li \ref AZ_Assert
     * \li \ref AZSTD_CONTAINER_ASSERT
     * \li \ref AZStd::allocator
     * \li \ref AZSTD_CHECKED_ITERATORS
     * \li \ref AZSTD_NO_DEFAULT_STD_INCLUDES
     * \li \ref AZSTD_NO_PARALLEL_SUPPORT
     * \li \ref AZSTD_NO_CHECKED_ITERATORS_IN_MULTI_THREADS
     *
     * Example configuration file (for a library project):
     * \code
     * // Since we are in a lib and we manage memory, we should not use the default allocator. This is a privilege of the
     * // final application. If the final application has not provided default allocator, use no_default_allocator which
     * // will cause compile error, if we try to use containers without providing valid allocator.
     * #ifndef AZStd::allocator
     *   #define AZStd::allocator AZStd::no_default_allocator
     * #endif
     *
     * // Not we can include the AZStd base defines.
     * // NOTE: for the control settings above to this should the first include of these files. If this is a lib, it is
     * // ok that they are already included by the application, since they have the right to configure AZStd as they like.
     * #include <AzCore/std/base.h>
     * #include <AzCore/std/typetraits/typetraits.h>
     * \endcode
     */
    /**
    * \file base.h
    * \brief Contains AZSTD configuration defines.
    */

    /**
     * \page Debugging
     *
     * AZStd is designed as much as possible for easy debugging directly with any compilers. For example the
     * list has types pointers to the prev and next elements, not void as in the most STL implementations.
     * This allows you traverse the list without any special casts. Sadly this is not the case for all containers.\n
     * This is why you should run \b \e "azstd\debugger tools\smartdebug.exe". This tool will add the following functionality
     * to your debugger environment:
     * \li Do not step in AZStd code and functions. They are assumed to be tested and safe, they
     * should not the focus of your work. Of course you can enable stepping in AZStd with one check-mark. The default if off (do not step in AZStd).
     * \li Containers debugging will be greatly simplified with some \ref DebugExeptions. For instance the vector/fixed_vector/list/fixed_list will just look like regular
     * C arrays. All iterators will show just their value (and pointer to it where applicable). This feature can be controlled with a check mark too.
     * Default is don't debug AZStd container, use smart visualizing.
     *
     * You can reference the smartdebug too, for updated list of supported debuggers. So far we support Visual Studio 2005/2008 and ProDG.
     *
     * \section DebugExeptions Exceptions
     * Due to some limitations in the debuggers for container visualization, at the moment we can not improve much the visualization of some
     * types of intrusive containers. For instance if you use intrusive_list and you use list_base_hook you will have smart visualization, but
     * if you use list_member_hook, only the root node will be displayed and you will need to traverse the list yourself.
     *
     *
     */

    /**
     * \page Containers
     *
     * AZStd covers all \ref CStd and \ref CTR1 containers, plus it introduces a new set of dynamic, fixed, intrusive and parallel containers.
     * All AZStd containers have the following functionality:
     *
     * \li All containers have typedefs for their allocation type node_type. This allows you to know at
     * compile time the allocation sizes and alignment (using sizeof(node_type) and alignment_of<node_type>::value)
     * \li All containers have support for \ref CheckedIterators, which are enabled by default in debug.
     * \li All containers have useful extensions (check each container).
     * \li Dynamic containers don't allocate any memory until you explicitly say so (by adding elements).
     * \li Dynamic containers have leak_and_reset functionality that allows quick tear down, for temporary containers. In the common case this is controlled
     * by the \ref Allocators.
     *
     * \note While AZStd is in development if you see a container without a link (class). This means that we plan to do it, but it's either not implemented or not ready to be used.
     *
     * \section ContainerComplexity Basic container operations complexity.
     * \subsection ContainerComplexityBase Base containers
     * <TABLE BORDER CELLSPACING="1" CELLPADDING="7">
     *  <TR>
     *    <TD>Container</TD>
     *    <TD>Stores</TD>
     *    <TD>Overhead</TD>
     *    <TD>Iterators</TD>
     *    <TD>Insert</TD>
     *    <TD>PushFront</TD>
     *    <TD>PushBack</TD>
     *    <TD>Erase</TD>
     *    <TD>PopFront</TD>
     *    <TD>PopBack</TD>
     *    <TD>Find</TD>
     *    <TD>Sort</TD>
     *   </TR>
     *   <TR>
     *    <TD>vector</TD>
     *    <TD>T</TD>
     *    <TD>0*</TD>
     *    <TD>Continuous</TD>
     *    <TD>N</TD>
     *    <TD>N</TD>
     *    <TD>C</TD>
     *    <TD>N</TD>
     *    <TD>N</TD>
     *    <TD>C</TD>
     *    <TD>N</TD>
     *    <TD>N log N</TD>
     *   </TR>
     *   <TR>
     *    <TD>list</TD>
     *    <TD>T</TD>
     *    <TD>8</TD>
     *    <TD>Bidirectional</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>N</TD>
     *    <TD>N log N</TD>
     *   </TR>
     *   <TR>
     *    <TD>forward_list</TD>
     *    <TD>T</TD>
     *    <TD>4</TD>
     *    <TD>Forward</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>N</TD>
     *    <TD>N log N</TD>
     *   </TR>
     *   <TR>
     *    <TD>deque</TD>
     *    <TD>T</TD>
     *    <TD>12*</TD>
     *    <TD>Random</TD>
     *    <TD>N/2</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>N</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>N</TD>
     *    <TD>N log N</TD>
     *   </TR>
     *   <TR>
     *    <TD>basic_string</TD>
     *    <TD>T</TD>
     *    <TD>0*</TD>
     *    <TD>Continous</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *   </TR>
     *   <TR>
     *    <TD>rope</TD>
     *    <TD>T</TD>
     *    <TD>0*</TD>
     *    <TD>Continous</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *    <TD>n/a</TD>
     *   </TR>
     *   <TR>
     *    <TD>set</TD>
     *    <TD>Key</TD>
     *    <TD>12</TD>
     *    <TD>Bidirectional</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>C</TD>
     *   </TR>
     *   <TR>
     *    <TD>multiset</TD>
     *    <TD>Key</TD>
     *    <TD>12</TD>
     *    <TD>Bidirectional</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>d log (N+d)</TD>
     *    <TD>d log (N+d)</TD>
     *    <TD>d log (N+d)</TD>
     *    <TD>log N</TD>
     *    <TD>C</TD>
     *   </TR>
     *   <TR>
     *    <TD>map</TD>
     *    <TD>Pair<Key,Value></TD>
     *    <TD>16</TD>
     *    <TD>Bidirectional</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>C</TD>
     *   </TR>
     *   <TR>
     *    <TD>multimap</TD>
     *    <TD>Pair<Key,Value></TD>
     *    <TD>16</TD>
     *    <TD>Bidirectional</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>log N</TD>
     *    <TD>d log (N+d)</TD>
     *    <TD>d log (N+d)</TD>
     *    <TD>d log (N+d)</TD>
     *    <TD>log N</TD>
     *    <TD>C</TD>
     *   </TR>
     *   <TR>
     *    <TD>unordered_map<br>unordered_multimap<br>unordered_set<br>unordered_multiset<br></TD>
     *    <TD>Key for sets<br>Pair<Key,Value> for maps</TD>
     *    <TD>8/4**</TD>
     *    <TD>Bidirectional/Forward<br>Depends if we use list/forward_list sub container.</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>C</TD>
     *    <TD>n/a</TD>
     *   </TR>
     * </TABLE>
     * \subsection ContainerComplexityAdapters Container adapters
     * <TABLE BORDER CELLSPACING="1" CELLPADDING="7">
     *  <TR>
     *   <TD>Container</TD>
     *   <TD>Stores</TD>
     *   <TD>Insert</TD>
     *   <TD>PushFront</TD>
     *   <TD>PushBack</TD>
     *   <TD>Erase</TD>
     *   <TD>PopFront</TD>
     *   <TD>PopBack</TD>
     *  </TR>
     *  <TR>
     *   <TD>stack</TD>
     *   <TD>T</TD>
     *   <TD>n/a</TD>
     *   <TD>n/a</TD>
     *   <TD>C</TD>
     *   <TD>n/a</TD>
     *   <TD>n/a</TD>
     *   <TD>C</TD>
     *  </TR>
     *  <TR>
     *   <TD>queue</TD>
     *   <TD>T</TD>
     *   <TD>n/a</TD>
     *   <TD>n/a</TD>
     *   <TD>C</TD>
     *   <TD>n/a</TD>
     *   <TD>C</TD>
     *   <TD>n/a</TD>
     *  </TR>
     *  <TR>
     *   <TD>priority_queue</TD>
     *   <TD>T</TD>
     *   <TD>n/a</TD>
     *   <TD>n/a</TD>
     *   <TD>log N</TD>
     *   <TD>n/a</TD>
     *   <TD>n/a</TD>
     *   <TD>log N</TD>
     *  </TR>
     *  <TR>
     *   <TD>ring_buffer</TD>
     *   <TD>T</TD>
     *   <TD>n/a</TD>
     *   <TD>n/a</TD>
     *   <TD>C</TD>
     *   <TD>n/a</TD>
     *   <TD>C</TD>
     *   <TD>n/a</TD>
     *  </TR>
     * </TABLE>
     *
     * Legend:
     *  \li * Overhead does not include pre-allocation. For instance when you push_back in the vector if there is not enought capacity
     *  we will preallocate memory, to avoid too many allocations.
     *  \li ** The overhead is as of list/forward_list container, in addition we have bucket vector container too. Which size is determined by the load factor.
     *  \li All time complexities are amortized. For instace vector push_bach is const (O(1)), until we run out of capacity then we need to
     *  reallocate the vector, this is O(N) operation + the allocation time. Still the amortized time is constaint.
     *  \li All fixed and intrusive containers have the same complexities as the primary dynamic containers. Just with memory allocation and for intrusive
     *  containers without construstion/destruction.
     *
     * \section DynamicContainers Dynamic Containers
     * Dynamic containers are container that will allocated memory to accommodate any number of elements. Sometimes this is all we need, sometimes this
     * cause memory fragmentation, bad performance because of slow allocations, cache misses, etc. That is why in addition to them we provide fixed (no allocations)
     * and intrusive (no allocation, better for cache) containers. None of the containers except \ref ParallelContainers are thread safe.
     *
     * \section Standard
     * \li vector
     * \li list
     * \li deque
     * \li basic_string
     * \li set
     * \li multiset
     * \li map
     * \li multimap
     * \li bitset
     * \subsection StandardAdapters As a container adapters
     * \li queue
     * \li priority_queue
     * \li stack
     *
     * \section TR1Containers TR1 containers
     * As part of \ref CTR1 :
     * \li array
     * \li unordered_set
     * \li unordered_multiset
     * \li unordered_map
     * \li unordered_multimap
     *
     * \section AZStdExtensionContainers AZStd extensions
     * \subsection DynamicContatinersList Dynamic containers:
     * \li forward_list
     * \li rope
     * \li ring_buffer
     *
     * \subsection FixedContainers Fixed containers
     * \li fixed_vector
     * \li fixed_list
     * \li fixed_forward_list
     * \li fixed_string
     * \li fixed_rope
     * \li fixed_set
     * \li fixed_multiset
     * \li fixed_map
     * \li fixed_multimap
     * \li fixed_unordered_set
     * \li fixed_unordered_multiset
     * \li fixed_unordered_map
     * \li fixed_unordered_multimap
     *
     * \subsection IntrusiveContainers Intrusive containers
     * Intrusive containers never allocate any memory, destroy or create any objects. Just uses the provided nodes via the hook
     * to store it's information. In addition intrusive data types are NOT required to be copyable as all other containers do.\n
     * Generally speaking intrusive containers are better for the cache, thus leading to better performance.
     *
     * \li intrusive_list
     * \li intrusive_slist
     * \li intrusive_set
     * \li intrusive_multiset
     * \li intrusive_map
     * \li intrusive_multimap
     *
     * \subsection SemiIntrusiveContainers Semi-intrusive containers
     * Semi intrusive are containers that in their behavior are like \ref IntrusiveContainers, but they need some extra data to work.
     * For instance unordered (hash) containers, in addition to the list node they container we need a bucket array (vector) to store the offsets
     * for each hashed value. This requiring the container to allocate this vector.
     *
     * \li intrusive_unordered_set
     * \li intrusive_unordered_multiset
     * \li intrusive_unordered_map
     * \li intrusive_unordered_multimap
     *
     * \subsection ParallelContainers Parallel containers
     * As it is coming from they name, these are containers that are save to be use from multiple processed or threads at the same time. In general they
     * are limited in their functionality and impose some restrictions.
     *
     * \li lock_free_stack
     * \li lock_free_stamped_stack
     * \li lock_free_intrusive_stack
     * \li lock_free_intrusive_stamped_stack
     * \li lock_free_queue
     * \li lock_free_stamped_queue
     * \li work_stealing_queue
     * \li concurrent_unordered_set
     * \li concurrent_unordered_multiset
     * \li concurrent_unordered_map
     * \li concurrent_unordered_multimap
     * \li concurrent_fixed_unordered_set
     * \li concurrent_fixed_unordered_multiset
     * \li concurrent_fixed_unordered_map
     * \li concurrent_fixed_unordered_multimap
     *
     */

    /**
     * \page AZStdExamples Examples
     *
     * \li \subpage AllocatorExamples "Allocators."
     * \li \subpage ArrayExamples "Array container."
     * \li \subpage VectorExamples "Vector and fixed_vector container."
     * \li \subpage ListExamples "List,fixed_list,intrusive_list and forward_list container."
     * \li \subpage DequeExamples "Deque container."
     * \li \subpage QueueAndPriQueueExamples "Queue and priority queue container."
     * \li \subpage StackExamples "Stack container."
     * \li \subpage HashExamples "Unordered containers."
     */

    /**
    * \page AllocatorExamples
    * You can see how to write allocator if you look at the \ref Allocators.
    * We will gust give some examples of practical their practical application.
    *
    * \dontinclude examples.cpp
    *
    * \skip // AllocatorExamples-Begin
    * \until // AllocatorExamples-End
    */

    /**
     * \page ArrayExamples
     * Array container usage.
     * We use test class \ref UnitTestInternal::MyClass in this examples.
     *
     * \dontinclude examples.cpp
     *
     * \skip // ArrayExamples-Begin
     * \until // ArrayExamples-End
     *
     * \section ArrayUnitTest Test code
     * In addition you can check our unit test code for the array container. We are trying to test
     * every function and each path inside. As an example this demonstrates the use of every aspect of the
     * container functionality.
     *
     * \dontinclude vector_and_array.cpp
     *
     * \skip // ArrayContainerTest-Begin
     * \until // ArrayContainerTest-End
     */

    /**
    * \page VectorExamples
    * Vector container usage. It this examples we will use vector, but except for the
    * custom allocators, it all applies to fixed_vector.\n
    * In the examples we usually show some tips on some of the important container features.
    * To see every function in use check the \ref VectorUnitTest and \ref FixedVectorUnitTest.
    *
    * We use test class \ref UnitTestInternal::MyClass in some examples.
    *
    * \dontinclude examples.cpp
    *
    * \skip // VectorExamples-Begin
    * \until // VectorExamples-End
    *
    * \section VectorUnitTest Vector test code
    * Here is the unit test code for the vector container. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude vector_and_array.cpp
    *
    * \skip // VectorContainerTest-Begin
    * \until // VectorContainerTest-End
    *
    * \section FixedVectorUnitTest Fixed vector test code
    * Here is the unit test code for the vector container. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude vector_and_array.cpp
    *
    * \skip // FixedVectorContainerTest-Begin
    * \until // FixedVectorContainerTest-End
    *
    */

    /**
    * \page ListExamples
    * All of the list demos apply to list, intrusive_list, fixed_list. Both intrusive and fixed_list don't have allocators and never allocate memory.
    * Intrusive list as all \ref IntrusiveContainers doesn't call constructor or destructor. Allows elements be be non-copyable. In addition all of this
    * applies to the \ref forward_list, which a single list version.
    *
    * There are not that many specific features and tips for the list container as for the vector. We will here just demonstrate a few of extensions in use,
    * otherwise you can check the test code to see every function in action.
    * \li \ref ListUnitTest
    * \li \ref FixedListUnitTest
    * \li \ref IntrusiveListUnitTest
    *
    * \dontinclude examples.cpp
    *
    * \skip // ListExamples-Begin
    * \until // ListExamples-End
    *
    * \section ListUnitTest List test code
    * Here is the unit test code for the list container. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude lists.cpp
    *
    * \skip // ListContainerTest-Begin
    * \until // ListContainerTest-End
    *
    * \section FixedListUnitTest Fixed list test code
    * Here is the unit test code for the fixed_list container. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude lists.cpp
    *
    * \skip // FixedListContainerTest-Begin
    * \until // FixedListContainerTest-End
    *
    * \section IntrusiveListUnitTest Intrusive list test code
    * Here is the unit test code for the intrusive_list container. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.\n
    * You can see at the start how we define our list class (MyListClass) with the different types of hooks. And their use later.
    * This is the only specific intrusive list feature, other than the general benefits of using \ref IntrusiveContainers.
    *
    * \dontinclude lists.cpp
    *
    * \skip // IntrusiveListContainerTest-Begin
    * \until // IntrusiveListContainerTest-End
    *
    */

    /**
    * \page DequeExamples
    *
    * \dontinclude examples.cpp
    *
    * \skip // DequeExamples-Begin
    * \until // DequeExamples-End
    *
    * \section DequeUnitTest Deque test code
    * Here is the unit test code for the deque container. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude deque_and_similar.cpp
    *
    * \skip // DequeContainerTest-Begin
    * \until // DequeContainerTest-End
    */

    /**
    * \page QueueAndPriQueueExamples
    * Since the queue and priority queue are just adapters, there are not any special examples. You just push and pop elements.
    * Just check the test code how each function is used.
    *
    * \dontinclude examples.cpp
    *
    * \section QueueUnitTest Queue test code
    * Here is the unit test code for the queue container. This is a container adapter
    * so there is not much to tell or explain. We just provide Queue like interface.
    * Check the underlaying container for more info.
    *
    * \dontinclude deque_and_similar.cpp
    *
    * \skip // QueueContainerTest-Begin
    * \until // QueueContainerTest-End
    *
    * \section PriQueueUnitTest Priority queue test code
    * Here is the unit test code for the priority_queue container. This is a container adapter
    * so there is not much to explain. We just provide Queue like interface.
    * Check the underlaying container for more info.
    *
    * \dontinclude deque_and_similar.cpp
    *
    * \skip // PriorityQueueContainerTest-Begin
    * \until // PriorityQueueContainerTest-End
    */

    /**
    * \page StackExamples
    *
    * Since the stack is just an adapters, there are not any special examples. You just push and pop elements.
    * Just check the test code how each function is used.
    *
    * \dontinclude examples.cpp
    *
    * \section StackUnitTest Stack test code
    * Here is the unit test code for the stack container. This is a container adapter
    * so there is not much to examplain. We just provide Stack like interface.
    * Check the underlaying container for more info.
    *
    * \dontinclude deque_and_similar.cpp
    *
    * \skip // StackContainerTest-Begin
    * \until // StackContainerTest-End
    */

    /**
    * \page HashExamples
    * All hash (unordered) containers are based on the hash_table. So any example
    * applies to all of them.
    *
    * For ref on all functions in action check the test code:
    * \li \ref HashFunctionsUnitTest
    * \li \ref HashTableUnitTest
    * \li \ref UnorderedSetUnitTest
    * \li \ref UnorderedMultisetUnitTest
    * \li \ref UnorderedMapUnitTest
    * \li \ref UnorderedMultimapUnitTest
    *
    * \dontinclude examples.cpp
    *
    * \skip // UnorderedExamples-Begin
    * \until // UnorderedExamples-End
    *
    * \section HashFunctionsUnitTest Hash functions test code
    *
    * Here is the unit test code for the hash functions.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // HashFunctionsTest-Begin
    * \until // HashFunctionsTest-End
    *
    * \section HashTableUnitTest Hash table test code
    *
    * Here is the unit test code for the hash_table. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // HashTableTest-Begin
    * \until // HashTableTest-End
    *
    * \section UnorderedSetUnitTest Unordered set test code
    *
    * Here is the unit test code for the unordered_set. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // UnorderedSetContainerTest-Begin
    * \until // UnorderedSetContainerTest-End
    *
    * \section UnorderedMultisetUnitTest Unordered multiset test code
    *
    * Here is the unit test code for the unordered_multiset. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // UnorderedMultiSetContainerTest-Begin
    * \until // UnorderedMultiSetContainerTest-End
    *
    * \section UnorderedMapUnitTest Unordered map test code
    *
    * Here is the unit test code for the unordered_map. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // UnorderedMapContainerTest-Begin
    * \until // UnorderedMapContainerTest-End
    *
    * \section UnorderedMultimapUnitTest Unordered multimap test code
    *
    * Here is the unit test code for the unordered_multimap. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // UnorderedMultiMapContainerTest-Begin
    * \until // UnorderedMultiMapContainerTest-End
    *
    * \section FixedUnorderedSetUnitTest Fixed unordered set test code
    *
    * Here is the unit test code for the unordered_set. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // FixedUnorderedSetContainerTest-Begin
    * \until // FixedUnorderedSetContainerTest-End
    *
    * \section FixedUnorderedMultisetUnitTest Fixed unordered multiset test code
    *
    * Here is the unit test code for the unordered_multiset. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // FixedUnorderedMultiSetContainerTest-Begin
    * \until // FixedUnorderedMultiSetContainerTest-End
    *
    * \section FixedUnorderedMapUnitTest Fixed unordered map test code
    *
    * Here is the unit test code for the unordered_map. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // FixedUnorderedMapContainerTest-Begin
    * \until // FixedUnorderedMapContainerTest-End
    *
    * \section FixedUnorderedMultimapUnitTest Fixed unordered multimap test code
    *
    * Here is the unit test code for the unordered_multimap. We are trying to test
    * every function and each path inside. As an example this demonstrates the use of every aspect of the
    * container functionality.
    *
    * \dontinclude hashed.cpp
    *
    * \skip // FixedUnorderedMultiMapContainerTest-Begin
    * \until // FixedUnorderedMultiMapContainerTest-End
    */
}
