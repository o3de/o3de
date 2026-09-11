#pragma once

// Force-included (via /FI) into every target in the vendored sentry-native dependency tree
// (sentry-native itself, plus the crashpad/mini_chromium/zlib it bundles as its own crash backend).
// This engine promotes a set of warnings to hard errors engine-wide via
// cmake/Platform/Common/MSVC/Configurations_msvc.cmake's /we<code> flags -- correct for our own
// code, but this vendored tree isn't ours to fix line-by-line as each warning surfaces on a
// different file. A target-level /wd<code> can't win against that /we<code> on the actual cl.exe
// command line (CMake's generator places TreatSpecificWarningsAsErrors-derived flags after
// DisableSpecificWarnings-derived ones regardless of which CMake scope contributed each -- see
// the analogous C4774/BoxPhysics fix), so command-line suppression can't win here -- a pragma can,
// since it's evaluated by the compiler frontend rather than the command line.
#if defined(_MSC_VER)
#pragma warning(disable : 4263 4264 4265 4266 4296 4426 4437 4774 4777 4855 5031 5032 5233)
#endif
