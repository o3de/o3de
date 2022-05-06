# Pre-Built Gem hook insutrctions

The following are instructions on how to reference pre-existing static libraries, shared libraries or executable files into the Pre-Build Gem CMake target

## How to Hook Pre-Built Binaries into the CMake targets

To get familiar with the list of targets that need to be updated, please examine the [Code/CMakeLists.txt](./Code/CMakeLists.txt) from the root of the Gem.
That CMake file contains the list of IMPORTED targets along with the list of per platform include files specified PLATFORM_INCLUDE_FILES argument to ly_add_target
```cmake
ly_add_target(
    NAME ${Name} IMPORTED ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/${LY_BUILD_PERMUTATION}/${NameLower}.cmake
```

The PLATFORM_INCLUDE_FILES are first segmented by OS platform first(Platform/Linux, Platform/Mac, Platform/Android, Platform/Windows, Platform/iOS) followed by having a build directory based on the build permutation that O3DE is using.
Those files are pre-configured to set the `IMPORTED_LOCATION` for an existing target

### What is a Build Permutation?

A build permutation is a different way to build O3DE engine targets based on the  value of the `LY_MONOLITHIC_GAME` cache variable.

When the `LY_MONOLITHIC_GAME=0`(or FALSE) which is the default, the Build Permutation is "Default" which represents non-monolithic.
In the "Default" permutation all gem modules are built as MODULE library targets which are dynamically loaded by the O3DE Gem System.
When the `LY_MONOLITHIC_GAME=1`(or TRUE), the Build Permutation is "Monolithic".
In the "Monolithic" permuation all gem modules are built as STATIC library targets and link directly into the GameLauncher and ServerLauncher applications.
The O3DE Gem System initializes the gem module through invoking the [CreateStaticModules](https://github.com/o3de/o3de/blob/development/Code/LauncherUnified/Launcher.cpp#L36-L38) function which is generated with the list of active gems when CMake [configures](https://github.com/o3de/o3de/blob/development/Code/LauncherUnified/launcher_generator.cmake#L228-L230) the GameLauncher and ServerLauncher targets

### Updating the IMPORTED targets IMPORTED_LOCATION to point to the pre-built binaries

First the `IMPORTED_LOCATION` for the IMPORTED target can be determined by opening the `Code/Platform/<platform>/<permutation>/${NameLower}*.cmake` file

#### Example: Updating the non-monolithic binaries for the Windows platform

To the update the non-monolithic binaries for the Windows platform first the `Code/Platform/Windows/Default` directory can be navigated to.
In that directory are several *.cmake files that contains the statements to set the target properties for the IMPORTED targets specified in the gems CMakeLists.txt
```cmake
get_property(${NameLower}_gem_root GLOBAL PROPERTY "@GEMROOT:${Name}@")
list(APPEND LY_TARGET_PROPERTIES
    IMPORTED_LOCATION ${${NameLower}_gem_root}/bin/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/release/${Name}.dll
    IMPORTED_LOCATION_DEBUG ${${NameLower}_gem_root}/bin/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/debug/${Name}.dll
)
```

The mapping of CMake module targets names to `${NameLower}*.cmake` file will follow.
1. `${NameLower}` -> `${NameLower}.cmake`
1. `${NameLower}.Builders` -> `${NameLower}_builders.cmake`
1. `${NameLower}.Tools` -> `${NameLower}_tools.cmake`
1. `${NameLower}.Tests` -> `${NameLower}_tests.cmake`
1. `${NameLower}.Tools.Tests` -> `${NameLower}_tools_tests.cmake`

For this example, placing a dll at the location of `<gem-root>/bin/Windows/Default/release/${Name}.dll` will allow it to be used by non-debug configurations and placing a dll `<gem-root>/bin/Windows/Default/debug/${Name}.dll` will allow it to be used by debug configurations.

The gem maintainer is free to change these locations by updating the `IMPORTED_LOCATION` or `IMPORTED_LOCATION_DEBUG` properties.
The gem maintainer can even consolidate all build configurations to use the same gem dll by not setting the `IMPORTED_LOCATION_DEBUG` property.

The following allows all build configurations(debug, profile, release) to use the same runtime gem dll
```cmake
get_property(${NameLower}_gem_root GLOBAL PROPERTY "@GEMROOT:${Name}@")
list(APPEND LY_TARGET_PROPERTIES
    IMPORTED_LOCATION ${${NameLower}_gem_root}/bin/${PAL_PLATFORM_NAME}/${LY_BUILD_PERMUTATION}/release/${Name}.dll
)
```

### Additional Notes

In order to configure a gem fully to be used for all platforms, the IMPORTED_LOCATION must be populated for the  CMake module targets above for each platform and for each build permutation.
As a side note Android and iOS only contains a `${NameLower}` cmake module target and do not have `.Builders`, .`Tools`, `.Tests` targets so they only need to set the IMPORTED location for the runtime gem module for the "Default" and "Monolithic" permutations
