# Material Canvas preview compile — measurement record

What was tested to make the Material Canvas preview compile faster, what the numbers were, and which
obvious-looking changes were measured and rejected.

Everything here was measured on this machine against this project. Where a number is inferred rather
than measured it says so. The point of the document is that nobody has to run these again.

Sections 1-8 are about **one Asset Processor shader job**. Section 9 is about a different clock: the time
between editing a node and the preview showing it, which is what a user actually experiences and which for
most of this exercise was not being measured at all. A change can move one and not the other.

Related: `Gems/Atom/Tools/MaterialCanvas/Assets/MaterialCanvas/Pipelines/PreviewOnly/ShaderTemplates/README.md`
covers the preview pipeline's fidelity cuts and per-include line attribution in more detail.

---

## 1. Where the time goes now

One preview shader job, measured 2026-09-04 from job log
`crystal_shader_1_materialcanvaspreview_transparent_standardlighting.shader-1240448048-21902.log`:

```
azslc                      454 ms
dxc   VertexShader vs_6_2   80
dxsc                        11
dxc   PixelShader  ps_6_2  197
dxsc                        13
                           ---
sum of timed children      755 ms
ProcessJob (builder total) 869 ms
job wall clock          ~930–1,320 ms
```

**These are the numbers this exercise started from.** Section 2.5 (SLL parsing) takes the job to
**0.621 s** and section 2.6 (overlapping the two dxc calls) to **0.530 s**, both confirmed from the
builder's own timer. The per-child rows above still describe the original run.

Three different measurements, three different meanings:

- **752 ms** is the sum of the five `ExecuteShaderCompiler elapsedTimeMillis` values. Each is the full
  wall time of one child process — its spawn *plus* its work. It is not 752 ms of spawning, and it is
  not every child process: MCPP runs too, on the `Preprocessor:` line, and the AP does not time it.
- **872 ms** is `ShaderAssetBuilder::ProcessJob`'s own timer, printed as
  `Finished processing … in 0.872 seconds`. Builder-side total.
- **~930–1,320 ms** is the AP's `Processing …` → `Processed …` in `AP_GUI.log`. First run of a session
  is the slow end; repeats settle near 950.

Which decomposes as:

| | ms | share |
|---|---|---|
| five compiler child processes | 752 | 75% |
| rest of `ProcessJob` (MCPP, reflection JSON, SRG layout, variant key, serialization) | 120 | 12% |
| outside `ProcessJob` — AP dispatch, product copy, DB and catalog | ~130 | 13% |

The 120 and the 130 are subtractions between the three timers above, not direct measurements. About
27 ms of the 130 is visible directly as the `Unable to remove file … (We may retry)` /
`SUCCESS: after failure` pairs on each product — cache file lock contention.

Input scale for those numbers: 8,638 lines of preprocessed AZSL (`.azslin`), 4,282 lines of generated
HLSL, 199,530 bytes. Dynamic branches: VS 3, PS 88.

---

## 2. AZSLc — what was changed and what it bought

Measured with `z_azslc_phases.ps1`, running azslc directly on the `.azslin` products the AP leaves in
the cache, with the AP out of the picture.

```
985 ms   baseline
600 ms   DependencySolver cursor       -385
590 ms   single-pass lex                -10   (noise)
395 ms   idExpression left-factoring   -195
377 ms   dead ExtractName call           -4
190 ms   SLL two-stage parse           -136   (section 2.5)
```

**−81% overall.** HLSL output verified byte-identical after each change.

### 2.0 Where the time inside azslc actually goes

Measured with `z_azslc_stages.ps1`, which decomposes a run using only azslc's own mode flags, so it
needs no instrumentation and no rebuild: `--syntax` stops after the parse (`Main.cpp:544`), `--semantic`
adds the semantic walk, MiddleEnd and Validate (`Main.cpp:697` gates emission off), no flag at all adds
`emitter.Run`, and `--full` adds the five reflection dumps. Consecutive differences attribute the total.

Before the SLL change, on the preview transparent shader, spawn subtracted:

| stage | ms | share |
|---|---:|---:|
| parse | 218 | 66.5% |
| semantic + middle-end | 70 | 21.3% |
| HLSL emission | 18 | 5.5% |
| reflection dumps (ia/om/srg/options/bindingdep) | 22 | 6.7% |

**An earlier version of this document put emission at ~105 ms by subtracting parse+semantic from the
total. That was wrong by a factor of five.** Emission and all five reflection dumps together are 40 ms,
so there is nothing to win in the emitter, and the case for attacking parse is much stronger than the
subtraction suggested. Decomposition beats subtraction: the flags were there the whole time.

### 2.1 `DependencySolver::SelectUnmarked` was quadratic — 385 ms

`SelectUnmarked()` restarted its scan from `m_order.begin()` on every call, so the topological sort was
O(n²) in the number of symbols. A cursor that remembers where the previous scan stopped makes it linear:

```cpp
// header: size_t m_selectCursor = 0;   ClearAllTemps(): m_selectCursor = 0;
while (m_selectCursor < m_order.size() && HasPermMark(m_order[m_selectCursor]))
{
    ++m_selectCursor;
}
return m_order[m_selectCursor];
```

**376 → 12 ms** for the solver phase. This is by far the largest single win of the whole exercise and
it is an upstream bug — every O3DE user is paying it.

### 2.2 `nestedNameSpecifier` could derive empty — 195 ms

The grammar let `nestedNameSpecifier` derive the empty string, which made `qualifiedId` able to derive a
bare `Identifier`, which made `idExpression` ambiguous against half the expression grammar. Left-factoring
it:

```antlr
nestedNameSpecifier:
        GlobalSROToken='::' (Identifier '::')*   // ::a::b::   or just ::
    |   (Identifier '::')+                       // a::b::
;
```

`idExpression` LL fallbacks: **11,855 → 3**. Parse phase 430 → 229 ms. Also an upstream bug.

### 2.3 Single-pass lex — ~10 ms, inside noise

`getAllTokens()` followed by `lexer.reset()` lexed the file twice. Replaced with `tokens.fill()` and
`ConstructLineMap(tokens.getTokens(), …)`. Predicted ~25 ms, actually ~10 and inside run-to-run noise.
Kept because it is strictly less work, not because it showed up.

### 2.4 Dead call in `AzslcListener::enterIdExpression` — 4 ms

```cpp
UnqualifiedName currentIdCtxUqName = ExtractNameFromIdExpression(ctx);  // result never read
```

11,031 calls, 3.7 ms. Predicted 15–30 ms. A reminder that call-count intuition is not a cost model.

### 2.5 SLL prediction mode — 136 ms, the largest remaining win

The parse was still 66% of azslc (section 2.0). It was running in ANTLR's **default `LL` prediction
mode**, and nothing in azslc had ever called `setPredictionMode`. Default `LL` means: try SLL, and on
every SLL conflict redo that decision with full outer context (ALL(\*)). The full-context closure is the
expensive path, and this grammar took it thousands of times per shader.

Pure SLL never escalates. Two changes make azslc able to use it:

**`Main.cpp` — two-stage parse.** Stage one runs `PredictionMode::SLL`. SLL accepts every input LL
accepts, except that it can report a syntax error on some inputs LL would have parsed, so a stage that
reports any error is discarded and re-parsed under LL. `Parser::reset()` clears the error count, rewinds
the token stream and frees stage one's nodes, so stage two starts from exactly the previous state.

**`azslParser.g4` — `idExpression` alternatives reordered.** This is the part that matters, and without
it the whole thing is a 32 ms *loss*: stage one failed on every real shader and the LL re-parse ran
anyway. On the preview shader SLL reported exactly three errors, all the same shape —

```
line 2287:19  token='::'  missing ';' at '::'      return Bindless::GetTexture2D(localReadIndex);
line 2309:50  token='::'  missing ';' at '::'      ... = Bindless::GetByteAddressBuffer(...);
line 7614:68  token='::'  missing ';' at '::'      ... = DirectionalLightUtil_PBR::Init(...);
```

On an input starting with an `Identifier` both `idExpression` alternatives are viable, so the decision
conflicts — and a conflict is resolved by taking the **lowest-numbered alternative**. With `unqualifiedId`
first, that answer is wrong for every qualified name: the parser takes `Bindless` alone and then wants
the statement to end. Full-context LL resolved it correctly from the caller, which is exactly why the
order never mattered before — and exactly what made it cost a full-context prediction to discover.
Putting `qualifiedId` first makes the greedier reading win, which is correct under SLL too:

```antlr
idExpression:
        qualifiedId     // could be relatively qualified OR fully qualified.
    |   unqualifiedId   // stricly unqualified (no nested specifiers at all)
;
```

This is safe only because `'::'` can never follow a complete `idExpression`. The only `'::'` in the
grammar outside `nestedNameSpecifier` is `typeofExpression`'s, and that one follows `')'`.

Regenerate the parser with `Source/Grammar/Generate.ps1` after editing the `.g4`. It needs Java, which
is not on PATH on this machine; the JBR shipped with PyCharm works
(`C:\Program Files\JetBrains\PyCharm Community Edition 2024.2\jbrin`). Verify the toolchain first by
regenerating with no grammar change — it reproduces the checked-in files exactly, modulo line endings,
because `Generate.ps1` writes LF into a CRLF tree (section 7.11).

Measured by `z_azslc_ab.ps1`, spawn subtracted, medians of 9 round-robin rounds:

```
parse     226 -> 98 ms    (-128, -57%)
full run  327 -> 190 ms   (-136, -42%)
```

Three independent correctness checks, all clean:

- **725 of 725** `.azslin` inputs in the project cache produce byte-identical output from the old and new
  binaries — the HLSL *and* all five reflection dumps, under identical flags.
- The azslc test suite is unchanged: 379 PASS, 3 TODO, 1 pre-existing FAIL
  (`Advanced/respect-emit-preprocessor-line-directives.py`, failing before this change too). Across 976
  lines of output the **only** difference is the wall clock: **46.5 s → 10.1 s**.
- The generated parser diff is confined to alternative order — same context classes, same accessors, so
  no API change. Consumers use `ctx->qualifiedId()` / `ctx->unqualifiedId()`, never the alternative index.

Like 2.1 and 2.2 this is an upstream bug rather than a Material Canvas one: every O3DE shader compile
pays it, and the production path pays it about three times over.

**Confirmed end to end**, 2026-09-05, from the builder's own timer:

```
ShaderAssetBuilder: Finished processing
  assets/materialcanvaspreview/assets/crystal_shader_materialcanvaspreview_transparent_standardlighting.shader
  in 0.621 seconds
```

**869 → 621 ms, −248 ms, −29% of the whole job.** That is more than the harness predicted (it projected
~735 ms from a −136 ms azslc delta). The harness times azslc alone on a cached `.azslin`; the job also
runs MCPP and the builder's own work, and the earlier 869 ms reading came from `crystal_shader_1`. Take
621 as the number that counts — it is the builder's own timer, which section 7.1 establishes as the one
to trust.

### 2.6 Overlapping the two dxc invocations — 91 ms

`dxc -E VertexShader` and `dxc -E PixelShader` are independent compiles of the same HLSL that ran strictly
back to back. Overlapping them recovers the shorter one.

What blocked it was not the loop but the filenames. Every dxc and dxsc intermediate in
`DX12::ShaderPlatformInterface::CompileHLSLShader` was derived from the source file name and the temp
folder and nothing else, so both entry points wrote **the same five paths**:

```
<stem>.dxil.bin           dxc -Fo, then dxsc's input
<stem>.dxil.txt           dxc -Fh, read back for the dynamic branch count
<stem>.dxil.patched.bin   dxsc -o
<stem>.offsets.json       dxsc -f, escapes into StageDescriptor::m_extraData
<stem>.hlsl.prepend       RHI::PrependFile's output, which dxc holds open while it runs
```

Sharing them is harmless only while the stages are strictly sequential — and it is exactly what stopped
them being concurrent. The `.pdb` was already disambiguated by profile name, so the shape of the problem
was known and only half applied. All five now carry the entry point name (`PrependArguments` already had
an `m_addSuffixToFileName` field for precisely this), falling back to the profile for RayTracing, which
passes no entry point to dxc.

With that done, `ShaderVariantAssetBuilder::CreateShaderVariantAsset` runs the entry points on separate
threads and joins. Only `CompilePlatformInternal` is parallel; the variant creator, the byproducts and the
trace output are all touched afterwards in a fixed order, because none of that is worth making thread safe
for what it would save.

Measured from the builder's own timer, one preview transparent shader:

```
             azslc   VS+dxsc   PS+dxsc   ProcessJob
before         454        91       210        0.869 s      (pre-SLL)
after SLL      234        91       210        0.621 s
after overlap  234     \__ 207 concurrent __/  0.530 s
```

**0.621 → 0.530 s, −91 ms, −15%.** Slightly better than the 80 ms predicted, because the vertex stage's
dxsc call hides under the pixel stage too.

Two pre-existing bugs were fixed in passing, both in the loop being restructured:

- `outputByproducts.emplace(...)` ran once per entry point, and `emplace` on an `AZStd::optional` that
  already holds a value **replaces** it. Every entry point but the last had its intermediate paths
  discarded before the caller could register them. Now merged.
- The loop iterated an `unordered_map`, so shader functions reached the variant creator in hash order.
  Now sorted by entry point name — worth more once the compiles are concurrent.

Verification: job reports `0 errors, 0 warnings`; the two stages write
`..._dx12.PixelShader.dxil.bin` and `..._dx12.VertexShader.dxil.bin`; and the dynamic branch counts come
back correctly attributed, VS 3 and PS 81, which a swapped assignment would not produce.

**One cost worth knowing.** The `ShaderPlatformInterface: Executing ...` and `elapsedTimeMillis` lines from
the two stages now interleave in the job log, so a given `elapsedTimeMillis` can no longer be attributed to
a stage by reading the preceding `Executing` line. The sum of those values also now exceeds the job's wall
clock, which is the expected consequence of running them at once rather than a contradiction of the kind
section 7.1 warns about. `ProcessJob`'s own timer remains the number to trust.

---

## 3. dxc flag A/B

`z_dxc_optlevel.ps1`, 2026-09-04. Seven measured rounds plus a discarded warm-up, arms run round-robin,
medians reported. Driven directly against the cached `.hlsl` with the platform header prepended exactly
as `RHI::PrependFile` does, so no build and no AP run are involved.

```
arm                    median      min      max   dxil bytes
O1 +Fh  (AP today)        231      228      238        37252
O1      (no -Fh)          225      222      228        37252
O0 +Fh                    262      257      268        65276
O0      (no -Fh)          254      243      259        65276
VS O1 +Fh                  63       61       63        12048
baseline (1 line)          22       21       23         2652
```

With the 22 ms spawn baseline subtracted: **PS at `-O1` costs 209 ms, at `-O0` costs 240 ms.**

---

## 4. Changes that look obvious and are wrong

Each of these was measured. They are listed so the next person does not spend the afternoon
rediscovering them.

### `-O0` instead of `-O1` — **slower by 15%**

240 ms against 209. The object code says why: `-O1` emits 37,252 bytes of DXIL, `-O0` emits 65,276. DXC
still runs its whole front end and DXIL emission at `-O0`; it just hands 75% more module to the writer,
validator and signer than the optimizer passes it skipped. The preview templates' original comment
warned that `-O0` can produce DXIL that fails validation — in practice you never get that far, because
it loses on speed first.

`-O1` is itself already the win here. Nothing in O3DE's shader build config sets an optimization level,
so stock shaders run at DXC's `-O3` default.

### Dropping `-Fh` — 3%, and it costs you the diagnostic

6 ms of 209. `ShaderPlatformInterface.cpp:250` says what the file is for — *"used for counting dynamic
branches"* — and line 379 reads it back as a job byproduct. Nothing in the built shader depends on it,
so it looks free to delete. It is not worth deleting: the dynamic branch count is what showed the
production pixel shader carrying 674 branches against the preview's 72, which is the fastest way to tell
whether a slow compile is the graph's fault or the pipeline's. If it is ever removed, make it conditional.

### Dead code elimination in azslc — cannot help

The AP log contains its own natural experiment: the same file, same includes, vertex stage 174 ms and
pixel stage 2,191 ms. If parse were the dominant cost the two would be close, so parse is a small
fraction of the total. And reachability is *derived from* the semantic analysis, so DCE cannot reduce
parse + semantic. It can only shrink emission — and section 2.0 now measures emission at 18 ms and all
five reflection dumps at 22 ms, so the ceiling on this idea is 40 ms even if it removed everything.
(This paragraph used to put emission at 105 ms, by subtraction. It is 18.)

### The `expression` precedence ladder — measured, zero effect

Rewriting the ambiguous expression rule as an explicit precedence ladder produced **identical** LL
fallback counts (3,553). `AmbiguityInfo::ambigAlts` shows all 936 remaining ambiguities are one shape:
`(x)` — ParenthesizedExpression versus CastExpression, the classic C ambiguity. Resolving it needs
semantic feedback into parsing, not a grammar rewrite. The ladder is still not worth writing.

> **This section used to end "The parser is at its floor." That was wrong** — see section 2.5, which took
> another 57% off the parse. The floor being measured was the floor *of `LL` prediction mode*. Those 936
> ambiguities are real and they are still there, but under SLL they cost nothing, because SLL resolves a
> conflict by rule instead of escalating to a full-context closure. The lesson is that "the remaining
> ambiguities are irreducible" and "the parse is as fast as it can be" are different claims, and the
> first does not imply the second.

### Guarding the four `Debug.azsli` includes — 30–50 ms, not worth it

Would recover ~294 lines at the measured 0.10–0.17 ms per line. `Debug.azsli` is deliberately written to
be included unconditionally: its predicates (`IsDirectLightingEnabled`, `IsDiffuseLightingEnabled`,
`AreNormalMapsEnabled`, `UseDebugLight` and six more) return the 'everything on' answer when debugging is
off, so ordinary lighting code calls them without guards. Twelve files across the engine depend on that,
including `ForwardPassVertexAndPixel.azsli`, which calls `DebugModifyOutput` on the main pixel path.
Guarding means shipping a stub header that redeclares that surface and keeping it in sync forever — and
drift would surface as a miscompiled *production* shader, not a build error.

### `ENABLE_PARALLAX=0` — saves nothing

Material Canvas already defaults it to 0 in `MaterialGraphName_Defines.azsli` for both BasePBR and
StandardPBR. Measurements showing a saving were taken on a hand-authored StandardPBR material, where the
default is 1.

### Attacking process spawn — 13%, structurally hard

22 ms × 5 invocations ≈ 100 ms. Real, but eliminating it means calling dxc through `dxcompiler.dll`
instead of spawning the exe, which is a large change to `ShaderPlatformInterface` for a fraction of what
azslc alone costs.

### Suppressing the viewport's repeat rebuilds — tried three times, reverted every time

The preview viewport applies its material **three times per edit**, measured over six consecutive edits
as ~620 ms, ~1590 ms and ~1730 ms after the graph compile completes. Only the first is anywhere near the
shader job (0.62 s). The obvious read is that the last two are redundant and the ~1.1 s tail is free.

**They are not redundant. The first apply is built against the previous edit's shader.**

Why the repeats happen at all: `QueueApplyMaterialIfAffected` starts with

```cpp
const bool affected = !m_appliedMaterialAssetId.IsValid() || ...
```

which is meant to say "nothing has resolved yet, so retry on everything" and to stop once something is on
screen. Under the in-memory preview path it never stops, because the compiler deliberately does not write
a preview `.material` at all (`MaterialGraphCompiler.cpp`, "Removing preview material, which the viewport
now builds itself"). The ID is invalid permanently rather than transiently, so **every catalog
notification in the project rebuilds the preview**, the 18,669-asset catalog save included.

Two attempts to close that hatch, both correct-looking, both wrong:

1. **Close it once an in-memory apply succeeds.** Result: exactly one apply per edit, at ~620 ms — and the
   viewport is permanently **one edit behind**. Moving a node or saving brings it forward, because that is
   the next compile.
2. **Also subscribe to the shader asset GUIDs the applied material type references**, so a shader rebuild
   still triggers the correcting apply. A shader asset keeps its GUID across rebuilds, so the subscription
   does fire. Still one edit behind.
3. **Rebuild on `AZ::Data::AssetBus::OnAssetReloaded`** for those shader assets instead of on the catalog
   notification — the event that means new bytecode is actually loaded rather than merely registered. Still
   one edit behind, and the trace added to prove the handler ran **never printed at all**.

That third result is the useful one, and it kills the whole family of fixes: **the shader assets are never
reloaded in the Material Canvas process.** There is no reload event to key on, so no amount of picking a
better moment to rebuild will work. The only thing that has ever brought the viewport forward is calling
`ApplyInMemoryMaterial` again late enough that `CreateInMemoryMaterialTypeAsset` re-resolves the shader
references from scratch and gets the new products — which is exactly what the incidental catalog traffic
was providing.

> **Correction, 2026-09-06.** The claim in bold above is not supported by the evidence that produced it.
> Attempts 2 and 3 gathered the shader assets to subscribe to by walking the material type's
> `GetGeneralShaderCollection()`. For a Material Canvas preview material type **that collection is empty** --
> the shaders live in the per-pipeline payloads reached through `GetMaterialPipelinePayloads()`. The same
> bug was later found and fixed in `CollectInMemoryShaderRequests`, which was returning zero requests for
> exactly this reason. So the `OnAssetReloaded` handler that "never printed at all" was most likely never
> subscribed to anything, and the run says nothing about whether shader assets reload in this process.
> Treat "there is no reload event to key on" as unproven rather than established. The rest of this
> subsection -- three attempts, all one edit behind, the race with `Material::Create` resolving an
> already-loaded shader -- still stands, and the fix that worked was the one this pointed at anyway:
> compile the shader in process and stop routing the preview through the asset system.

The reason both fail is a race the repeats were papering over. `ApplyInMemoryMaterial` can build a
perfectly valid material while the Asset Processor is still rebuilding the shader this edit changed:
`Material::Create` resolves the shader through the asset system and gets the **already-loaded** instance,
which is the previous edit's bytecode. The catalog notification announcing the new shader arrives *before*
that asset has finished reloading, so rebuilding on it — attempt 2 — is still too early. The applies at
1590 ms and 1730 ms worked only because they were triggered by unrelated later traffic (FinalStage's
products, the catalog save) that happened to land after the reload finished.

So the tail is not waste, it is an accidental retry loop that happens to be late enough to be correct.
Anything that removes it has to replace it with a real signal.

The existing staleness guard does not cover this. It compares the intermediate material type's
modification time against the source's, which says nothing about shaders — and on a value edit the
material type is not even rewritten ("Generated material type is unchanged, skipping write" on every one
of these edits), so the guard always passes.

**Where this actually has to be solved:** not in when the viewport rebuilds, but in what it rebuilds
*from*. Every attempt above assumed the shader products become visible to this process on their own and the
only question was timing. They do not. A fix has to either force the shader assets to reload before
rebuilding, or stop routing the preview's shaders through the asset system at all — which is the second
half of the `InMemoryShaderCompiler.h` spike (DXC, `ShaderAssetCreator`, `ShaderVariantAssetCreator`, and
handing the viewport an in-memory shader with no catalog entry).

Do not attempt a fourth variation on "rebuild at a smarter moment". Three have failed, and the failure mode
is invisible in logs — every attempt logged a clean, successful apply, and only looking at the viewport
caught it.

**Also observed, unexplained:** the viewport hitches noticeably while the material type job is compiling.
That is separate from the staleness and was not investigated.

### Caching the compiled `AZStd::regex` in `ReplaceSymbolsInContainer` — zero

`ReplaceSymbolsInContainer` builds a fresh `AZStd::regex` from its pattern on every call, and a graph
compile calls it with the same handful of patterns hundreds of times: the substitution symbols for a node
are rebuilt for every instruction block and applied once per slot. Caching them thread-locally by pattern,
plus an early-out when the container is empty, looked like an obvious win against a phase that was then
247-301 ms.

It measured **nothing**. The phase stayed at 247-301 ms across four edits.

The actual cost was in the same call chain but a different library: `GetSymbolNameFromText` was building
seven `QRegularExpression` objects per call (section 9.3). Running the expressions was never expensive;
compiling the Qt ones was. The cache was reverted rather than left in a shared utility unmeasured; the
empty-container early-out was kept, because it avoids the work by construction rather than remembering it.

The lesson is narrow and worth keeping: *"this function constructs an expensive object in a loop"* names a
suspect, not a cause. There were two such objects in one call chain and only one of them mattered.

### Instrumenting the 120 ms inside `ProcessJob` — not worth the cycle

It is split across MCPP, reflection JSON parsing, SRG layout, variant key construction and serialization.
The largest single item in there is probably 40–60 ms, which is the same size as overlapping the two dxc
calls — and that one needs no instrumentation and no rebuild to evaluate.

---

## 5. What actually remains

Both items that were here are done: the dxc overlap is section 2.6, and section 2.5 is confirmed end to
end. **The job is now 0.530 s, from 0.869 s at the start of this exercise.**

After that the honest list is short. What is left inside azslc is the semantic walk and middle-end at
about 70 ms; emission and reflection together are 40 ms and not worth touching; the remaining parse is
~98 ms. Every flag-level lever has been tested, and the pipeline is already down to one shader per save
through one RHI backend.

That is the end of the line for the *job*. It is not the end of the line for the *preview*: at the point
the job reached 0.530 s an edit still took about 1.8 s to reach the viewport, and more than half of that
was not the job at all. Section 9 covers where the rest went.

A note on how 2.5 was found, because the same mistake is easy to repeat: the previous version of this
document concluded there was nothing left in azslc, and it did so from two numbers that were both
derived by subtraction rather than measured — an emission cost that was five times too high, and a parse
"floor" that was really a property of the prediction mode nobody had checked was configurable. Both were
answerable with flags azslc already had.

---

## 6. Structural wins already banked

These are not tuning — they change how much work exists at all, and they are the reason the job is ~1 s
rather than ~20 s.

| change | effect |
|---|---|
| Preview-only material pipeline | 4 shaders instead of 21 for a Standard lighting model |
| `DisabledRHIBackends: ["null","vulkan"]` in the preview templates | halves every preview job — the whole MCPP→azslc→DXC chain runs once per enabled RHI |
| Preview / production output split | a save writes the preview alone; production is behind Apply (Ctrl+Shift+A) |
| `ProductionMaterialPipelines = "MainPipeline"` | production builds 3 shaders instead of both pipelines' full set |
| `ENABLE_LIGHT_CULLING` guard in `LightCullingTileIterator.azsli` | NVLC.azsli now charges 0 lines; preview shaders dropped 222 non-blank lines |
| Material property values sent over a bus instead of through the material type | a value-only edit skips the asset rebuild entirely |

For scale, the same graph through the production path: **4.3 s** for 3 MainPipeline shaders, of which
the transparent one alone is 2.47 s.

---

## 7. Traps

Things that cost real time to discover, in rough order of how badly they mislead.

### 7.1 Job log timestamps are flush times, not event times

`JobLogs/**/*.log` lines carry the AP's *receive* timestamp for output relayed from the builder process,
batched. On one job the timestamps implied 490 ms of startup and a 383 ms working window — but the five
child processes alone sum to 752 ms, which does not fit. The contradiction is the tell.

**Use `ShaderAssetBuilder`'s own line instead:** `Finished processing … in 0.872 seconds`. It is a timer
inside the builder and it is trustworthy.

### 7.2 Job logs are per-job and overwritten

A later run of the same job replaces the file. Three consecutive wrong diagnoses in this project came
from reasoning about logs whose dates had not been checked. **Always `ls -t` first.** The newest log is
the answer; anything older describes a build that no longer exists.

### 7.3 A harness can measure mostly itself

An earlier dxc harness reported **1,017 ms for a one-line shader**, while the AP was seeing 59–175 ms for
real ones. The real fixed cost is 22 ms. Any timing harness here needs a baseline arm (a trivial input)
and alternating round-robin rounds with medians, or it reports its own overhead as the result.

### 7.4 `O3DE_MC_POSITION` means different things in different stages

- `MaterialGraphName_VertexEval.azsli`: `const float3 O3DE_MC_POSITION = position;` — **object space**
- `MaterialGraphName_SurfaceEval.azsli`: `const float3 O3DE_MC_POSITION = (float3)IN.position;`

and `ForwardPassVertexData.azsli:145` declares `float4 position : SV_Position`. So in the pixel stage the
stock **Position** node gives you **screen space**. Nothing in the pixel stage carries object position
unless `MATERIAL_USES_VERTEX_LOCALPOSITION` is on and the vertex eval writes `output.localPosition`
(interpolator `UV5`, `ForwardPassVertexData.azsli:173`).

### 7.5 `#ifdef` vs `#if` on the `MATERIAL_USES_VERTEX_*` family

The engine headers **define these to 0** when unused (`ForwardPassVertexData.azsli:37-38`), so `#ifdef` is
true in both states. `MaterialGraphName_VertexEval.azsli` uses `#ifdef`. Anywhere you gate on these,
use `#if`, or you will write to a `VsOutput` member that does not exist in that pass.

### 7.6 Generated files and their templates drift apart silently

`MATERIAL_USES_VERTEX_LOCALPOSITION` support existed in the *generated* `wall_Defines.azsli` and
`wall_VertexEval.azsli` but not in the templates they came from. A re-save regenerates from the template
and the write disappears — while `BasePBR_VertexData.azsli` still declares `localPosition` in `VsOutput`
unconditionally, so it still compiles and the pixel shader reads an **uninitialized interpolator**.

Anything hand-edited in a `<graph>_*.azsli` is one save away from being lost. Put it in
`GraphData/Nodes/MaterialOutputs/{BasePBR,StandardPBR}/MaterialGraphName_*.azsli`.

### 7.7 Two applications writing the same settings file from opposite directions

`MaterialCanvasApplication::ApplyPreviewMaterialPipelineSettings` **created**
`user/Registry/user_preview_material_pipeline.setreg` while
`MaterialCanvasEditorSystemComponent::ApplyPreviewMaterialPipelineSettings` **deleted** it — both on
shutdown. Every switch between the two applications replaced `/O3DE/Atom/RPI/MaterialPipelineFiles` for
the whole project and put it back, which:

- rebuilt every material type in the project, twice per round trip;
- silently gave the production output the preview pipeline, because `MainPipeline` was no longer in the
  list its name resolved against;
- and took the Asset Processor down — one source's products were being deleted for the pipeline it had
  left while being inserted for the one it had joined, the names collided, and `InsertProduct` hit a
  uniqueness constraint (`Statement::Step() resulted in error code 19`), followed immediately by
  `App quit requested`.

Fixed by making the standalone match the Editor: remove only, never write.

### 7.8 A crashed AP leaves a database that keeps crashing it

The code fix above stops *new* collisions. It cannot clean rows the database already holds. Symptoms of
the stale state:

- `Failed to retrieve a valid builder to process job`
- `Output product …_dx12.azslin … is not valid. The file may have been deleted unexpectedly`
- `MaterialTypeAsset: Shader asset not found for source file '…'`
- `Dependent asset (…azshader) could not be loaded`
- and, in the cache, a material folder containing `.azmaterial` and `.azmaterialtype` but **no `.azshader`**

A material whose material type has no shader assets draws nothing, so **the mesh simply disappears**, with
no compile error anywhere. If a mesh vanishes after a graph edit, check
`Cache/pc/assets/<path>/` for `.azshader` files before suspecting the shader code.

Recovery: close the AP, delete `Cache/assetdb.sqlite`, `-shm` and `-wal`, restart. If collisions return,
delete `Cache/` outright.

### 7.9 `Texture2D.Sample` is pixel-only

`CS_SAMPLERS` is the engine's flag for "this stage has no derivatives" (used by GoboTexture,
ParallaxMapping, AlphaUtils, Ibl, Ltc). Any node whose instructions can land in a vertex eval must use
`SampleLevel`, guarded:

```hlsl
#ifdef CS_SAMPLERS
    c = tex.SampleLevel(smp, uv, 0);
#else
    c = tex.Sample(smp, uv);
#endif
```

### 7.10 azslc argument order

`--namespace` is declared as an unbounded `std::vector` option and will swallow a trailing positional
argument. The input file goes **first**, as `AzslCompiler.cpp:71` does it. There is no `-i` flag; input
is `cli.add_option("FILE", …)`, positional.

### 7.11 The whole tree is CRLF

`core.autocrlf` is unset and the tree was converted wholesale. Every `git diff` needs
`-w --ignore-cr-at-eol` or it reports every line of every file as changed — which is how a reverted file
went unnoticed once already.

### 7.12 Reverting instrumentation by un-editing

Retracing edits backwards nearly shipped a silent miscompile: un-editing deleted an `if` in
`TypeofExpr(BinaryExpressionContext*)`, leaving `{ return MangleScalarType("bool"); }` — **every binary
expression would have returned bool**, with no build error. Instrumentation reverts must restore from
git (`git show HEAD:<path>`), never retrace. `z_azslc_instrument.py --revert` is sentinel-based for this
reason.

### 7.13 A GUI process pays for a console on every child it spawns

Driving the shader compilers in process from Material Canvas, rather than letting the Asset Processor do
it, was three times slower for no visible reason:

```
                in Material Canvas   standalone   from an AssetBuilder
azslc                  619-673 ms       226 ms              ~234 ms
DXC, both stages       965-991 ms            -                277 ms
```

Same binary, same arguments, same input, same idle machine. The cause is that `ExecuteShaderCompiler`
asked for `m_showWindow = true`, and a console process launched from a parent with no console of its own
is given a new one -- which on current Windows means spawning a `conhost.exe` next to it. About 350-440 ms
per child. An AssetBuilder is itself a console application, so its children attach to the console already
there and pay none of it; that is the whole reason the Asset Processor path looked fine.

The tell was visual and came from watching the screen rather than the numbers: **three console windows
opening and closing per compile**, one per child.

Two changes fix it. `ExecuteShaderCompiler` no longer asks for a window. And `ProcessWatcher_Win` now maps
that to `CREATE_NO_WINDOW` as well as `SW_HIDE` -- hiding a console does not stop it existing, so a caller
that asked for no window was still paying for an invisible one.

```
azslc  240 ms, DXC 285 ms, whole in-process chain 526 ms
```

**Two wrong diagnoses came first, and both were wrong for the same reason: the test did not reproduce the
condition.**

- *The wait loop spins.* `ExecuteShaderCompiler` polls `IsProcessRunning`, `PeekError` and `PeekOutput`
  flat out with no sleep, which is real and looks damning. Replacing the spin with
  `this_thread::yield()` changed 673 ms to 664 ms -- because on an idle 32-core machine there is no other
  runnable thread to yield to, so `yield()` returns immediately and the loop spins exactly as before. The
  test was inert. A real one-millisecond sleep, which does stop the polling, gave 619 ms: inside the
  652-673 ms noise band. The loop is not the problem and the spin was left alone.
- *The console costs time.* Tested by launching azslc from PowerShell with and without `CreateNoWindow`,
  which showed no difference at all -- because PowerShell **is** a console application, so the child
  attaches to the console that already exists and the flag has nothing to do. The one host where it
  mattered was the one not being tested.

Both are the same mistake as section 7.3 in a different costume: a measurement that cannot observe the
thing it is aimed at will report that the thing is not there.

### 7.14 A comparison harness can pass because both sides are broken

Verifying the SLL change meant running an old and a new azslc over all 725 `.azslin` products and
diffing. The first run reported **725 of 725 identical** in about a second, which is roughly the correct
answer arrived at for entirely the wrong reason: both binaries had been copied to `%TEMP%`, away from
their DLLs, and each died with exit `0xC0000135` (`STATUS_DLL_NOT_FOUND`) before parsing anything. Two
runs that both produce zero files compare equal, file for file.

The tell was the runtime, not the result. 725 shaders cannot compile twice each in a second.

Any A/B harness needs the failure modes to be *loud*: assert the exit code is zero, assert the output
set is non-empty, and check that the comparison can detect a difference at all. `z_azslc_ab.ps1` reports
all three as an `unusable` count, and that count being zero is what makes the identical count mean
anything. This is the same failure as section 7.3 wearing different clothes — a harness reporting a
property of itself rather than of the thing under test.

Related: azslc needs its sibling DLLs, so both binaries in an A/B belong in the build's `bin\profile`
under different names, not in a scratch directory.

### 7.15 An Atom tool that is not the foreground window runs at 4 fps

`AtomToolsApplication::OnIdle` reschedules itself with `UpdateIntervalWhenActive` (1 ms) or
`UpdateIntervalWhenNotActive` (**250 ms**), chosen by `applicationState() & Qt::ApplicationActive`. Every
system tick, and therefore everything that defers work to the next tick, runs at four frames a second the
moment the window loses focus.

This showed up as a rock-steady 253 ms between a shader finishing and the material reaching the screen,
across five consecutive edits, in a code path whose own work is 7-18 ms. It read like a fixed cost inside
the apply. It was the measurement environment: the window was not focused during those runs. A tick probe
settled it in one build -- `Slow tick: 254 ms since the last one`, fifty times, perfectly regular. Under a
focused window the same path measures 2-5 ms.

Any latency measured in an Atom tool has to state whether the window had focus, and a stopwatch reading
taken while alt-tabbing carries up to 250 ms of tick wait that has nothing to do with the code.

---

## 8. Re-measuring

| script | what it does |
|---|---|
| `z_azslc_phases.ps1` | azslc phase table on the `.azslin` products in the cache, AP out of the picture |
| `z_azslc_stages.ps1` | parse / semantic / emission / reflection split using only azslc's own mode flags — no rebuild, no instrumentation |
| `z_azslc_ab.ps1` | two azslc builds compared: all 725 cache shaders for identical output, then timed. Both binaries must sit next to their DLLs — see the header |
| `z_azslc_instrument.py` | adds/removes phase timers in azslc. `--deep` for the 40 SemaCheckListener callbacks, `--profile-parser` for the ANTLR decision profile and ambiguity breakdown, `--check`, `--revert` |
| `z_dxc_optlevel.ps1` | dxc arms (`-O0`/`-O1`, with and without `-Fh`) plus a spawn baseline, against the cached HLSL |
| `z_job_breakdown.ps1` | spawn floor and per-spawn decomposition of a whole job |
| `z_measure_shader_compilers.ps1` | azslc across the cache products, before/after a change |
| `z_attribute_shader_lines.ps1` | per-include line attribution, for the fidelity-cut table |
| `z_preview_instrument.py` | adds/removes the edit-to-viewport probes of section 9 -- graph compile, its phase split, the in-process shader compile, when the apply ran, and slow system ticks. `--add`, `--revert`, `--check`, optionally naming individual probes |

**Revert the azslc instrumentation and rebuild clean before committing or benchmarking.** An
instrumented azslc reports its own phase table into the AP log, which is how to tell at a glance whether
a measurement came from a clean binary.

---

## 9. Edit to viewport — the other clock

Everything above measures one Asset Processor job. This measures what a user sees: node edit to preview
updated. Measured 2026-09-06 from `MaterialCanvas.log`, four to six consecutive edits per build, on the
same graph (`crystal_shader.materialgraph`, one preview shader, transparent standard lighting).

**~1.8 s to 0.8-1.2 s**, in three changes. The spread is real and is mostly the shader compile: azslc and
dxc vary by a couple of hundred milliseconds between runs depending on what else the machine is doing.

| phase | before | after |
|---|---|---|
| graph compile — `OnCompileGraphStarted` to `OnCompileGraphCompleted` | 694–820 ms | **98–112 ms** |
| in-memory shader compile | ran strictly after the graph compile | 658–792 ms, overlapping it |
| shader ready → material on screen | 305 ms | **2–5 ms** |

Two measurements, and they agree. The per-phase figures are from the process's own timers; the 0.8-1.2 s
is a stopwatch held against the screen, which is the only measurement that includes the parts no timer in
this process can see -- the edit reaching the compiler, and the driver building a pipeline state for the
new shader. The earlier 1.45 s reading was against a build that still had both the apply gap of 9.2 and,
as it turned out, an unfocused window (section 7.15).

The probes that produced the per-phase numbers are not in the tree. `z_preview_instrument.py --add` puts
them back and `--revert` takes them out; `--check` says which are present, and either verb accepts
individual probe names. Each probe is an exact before/after text pair rather than a sentinel, so a revert
restores the original bytes and any drift fails loudly instead of half-applying -- section 7.12 is why.

### 9.1 The graph compile was mostly waiting for jobs nothing was waiting on

`GraphCompiler::ReportGeneratedFileStatus` blocks until the Asset Processor has finished every job for
every file the compile generated. Removing it took the compile from 694–820 ms to 237–320 ms, so it was
roughly 450–580 ms of it.

None of it was needed. A typical graph edit rewrites only the generated `.azsli` instruction files. The
`.materialtype` comes out byte for byte identical — the compiler says so on every edit, `Generated
material type is unchanged, skipping write` — and so do the `.shader` files. The Asset Processor reruns
the material type's Pipeline and Final stages anyway, because those azsli are fingerprint dependencies of
the job (`MaterialTypeBuilder::PipelineStage::CreateJobsHelper`, `m_materialShaderCode` and
`m_materialShaderDefines`), and it rebuilds them into an intermediate identical to the one already on
disk. The compile was blocking on work whose result it already had.

`MaterialGraphCompiler` now records `m_writtenGeneratedFiles` — the generated files whose *contents*
actually changed this compile, which every write site already knew — and `ShouldReportGeneratedFileStatus`
reports only those. A typical edit reports nothing and does not wait:

```
Asset status wait: 0 ms for 2 changed file(s).
```

A compile that genuinely rewrites the material type or a shader still waits, exactly as before.

Paired with this is a new `GraphDocumentNotificationBus::OnCompileGraphProcessing`, raised where
`ReportGeneratedFileStatus` sets `State::Processing`. The generated files are on disk by then, so a
listener that only needs the files can start there instead of at completion. The viewport uses it to start
the shader compile, which is what lets the two overlap whenever a wait does still happen.

**A wrong turn worth recording.** The obvious next move looked like stopping the Asset Processor from
rebuilding the preview material type at all — the material-type equivalent of the
`SkipIncludeFileDependencies` flag added for shaders. The AP log says don't bother: that rebuild is
~190 ms and it lands *after* the in-process compile rather than during it. The 390 ms hole measured
between its two jobs was a builder process connecting, not CPU. That would have been an engine-wide
builder change for nothing.

### 9.2 Applying before the shader is ready costs more than waiting for it

With the wait gone, `OnCompileGraphCompleted` now fires a few hundred milliseconds *before* the in-memory
shader is ready. It applied the material immediately, which meant building an entire material — instance,
property overrides, pipeline state — around the previous edit's shader, and throwing it away when the real
one arrived. Worse, that throwaway build occupied the main thread at exactly the moment the real result
landed:

```
In-memory shader compile took 585 ms ... finishing 585 ms after the compile.
Preview using 1 shader(s) compiled in process, 890 ms after the compile finished.
```

305 ms, for a function whose own work is 7–18 ms. The apply is now deferred while
`m_shaderCompileInFlight` is set. The compile job queues an apply when it finishes whether or not it
produced anything, so this is a deferral rather than a decision not to apply, and the failure path still
reaches the Asset Processor through `ClearFingerprintForAsset`.

```
In-memory shader compile took 632 ms ... finishing 632 ms after the compile.
ApplyMaterial entered 636 ms after the compile finished.
Preview material applied from memory, 642 ms after the compile finished.
```

### 9.3 `GetSymbolNameFromText` compiled seven regular expressions per call

With the wait and the apply gap gone, the graph compile was ~300 ms and phase timers put effectively all
of it in one place:

```
Compile phases: 294 ms total -- tables 3, load templates 1, preprocess 1, instructions 254, export 34.
```

`BuildInstructionsForCurrentNode` walks the graph once per `O3DE_GENERATED_INSTRUCTIONS` block across all
templates. Per node it calls `GetSubstitutionSymbolsFromNode`, which calls
`AtomToolsFramework::GetSymbolNameFromText` once for the node and twice for every slot;
`GetInstructionsFromSlot` then calls it again per slot. That function constructed **seven
`QRegularExpression` objects on every call** — tens of thousands of PCRE compiles per edit, on patterns
that are compile-time constants.

They are `static const` now. `GetDisplayNameFromText` had the same shape and got the same treatment,
though it is not on this path.

```
Compile phases: 106 ms total -- tables 4, load templates 1, preprocess 1, instructions 75, export 24.
```

**247–301 ms → 68–78 ms.** This is in `AtomToolsFramework`, so it applies to Material Editor, Pass Canvas
and Shader Management Console too, not just the preview.

The thing that made this hard to see: an earlier attempt fixed the *other* expensive object in the same
call chain — the `AZStd::regex` built per call in `ReplaceSymbolsInContainer` — and measured exactly zero.
Section 4 has that non-result.

### 9.4 What is left

~660–790 ms of the remaining ~800 ms is the in-memory shader compile, and it is nearly all external
compilers:

| | ms |
|---|---|
| azslc | ~260 |
| dxc, pixel stage (vertex stage hides underneath it) | ~260 |
| dxsc, both stages | ~30 |
| MCPP, process spawn, file IO | ~110 |
| graph compile | ~100 |
| apply | ~5 |

Both compilers have already been through this document once — sections 2.5 and 2.6 for azslc, section 3
and section 4's `-O0` entry for dxc. There is no obvious next move that does not mean either attacking
azslc's semantic walk again or trading viewport frame rate for compile time.

One structural option is untried: the preview still depends on the Asset Processor for the *intermediate
material type*, which is what `CreateInMemoryMaterialTypeAsset` reads. Running `MaterialTypeBuilder`'s
pipeline stage in process would remove the last Asset Processor dependency from the preview path
altogether. It buys no latency today — that rebuild already lands after the compile finishes (9.1) — so it
is only worth doing if something else makes it the critical path.
