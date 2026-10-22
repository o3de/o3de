# Validation scope

## Executed

- Six Python integration tests covering custom fields/BOM/CRLF preservation, backups, idempotence, creation of missing project SRGs, refusal of edited/unsupported blocks, migration from the legacy Gem include, and self-contained declaration installation.
- Standalone C++17 CaptureSize assertions for full/half/quarter sizing, odd dimensions, small/zero dimensions, invalid divisors and overflow-safe maximum dimensions.
- Isolated C++ argument-dependent lookup regression for enum type registration in its namespace. This does not replace an engine serialization test.
- Standalone preprocessing fixture: legacy include fails without the Gem include path, while self-contained project declarations preprocess identically with and without it. This tests include independence, not AZSL language compilation.
- User-reported Windows O3DE 2.8.0 runtime/Editor builds, Material Canvas scene-color rendering, resolution switching and no errors on disable/rebuild with external standalone Gem storage. The final in-engine Gem catalog packaging still needs validation.

## Remaining

- Full O3DE source-engine configure/build and Asset Processor validation of the final PR package.
- Vulkan/Linux, other desktop pipelines, standalone preview scenes, lifecycle/teardown, viewport resizes, multiple Views/pipelines and GPU synchronization validation.
- GPU time, bandwidth, persistent-memory and disabled-bookkeeping measurements.
- Materials/components explicitly depending on the Gem intentionally require migration before removal. The self-contained project layout remains installed after disable; exact restoration of the pre-install layout is not claimed.
