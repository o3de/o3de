#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
add_feature_processor_registration
==================================

Wires an Atom RPI FeatureProcessor into a gem's RenderingSystemComponent
(Activate) and tears it down in Deactivate.

Generated effect (idempotent)
-----------------------------
  Header:  #include "${feature_processor_name}.h"
  Activate():
      AZ::RPI::FeatureProcessorFactory::Get()
          ->RegisterFeatureProcessor<${feature_processor_name}>();
  Deactivate():
      AZ::RPI::FeatureProcessorFactory::Get()
          ->UnregisterFeatureProcessor<${feature_processor_name}>();

Why this command exists
-----------------------
Mirrors add_pass_creator_call but for the FeatureProcessor side of Atom's
extension surface. The ScreenSpaceConstant variant of the ScreenSpaceRenderPass
template ships a FeatureProcessor that owns per-pipeline injection of a pass;
this command makes that registration self-maintaining when the wizard is
re-run with additional FeatureProcessors.

Args (template.json process_commands entry)
-------------------------------------------
  feature_processor_name : str, required
        The C++ class name of the FeatureProcessor (e.g. "MyEffectFeatureProcessor").
        Used as both the template parameter to RegisterFeatureProcessor and
        the include header name.
  system_component_name  : str, optional
        Override for the system component class name. Defaults to
        "${GemName}RenderingSystemComponent".
"""

import re
from pathlib import Path
from typing import Optional, Tuple

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("add_feature_processor_registration")
class AddFeatureProcessorRegistrationCommand(WizardCommand):
    """Register an RPI FeatureProcessor with FeatureProcessorFactory from a RenderingSystemComponent."""

    def __init__(self, feature_processor_name: str, system_component_name: str = ""):
        self.feature_processor_name = feature_processor_name
        self.system_component_name = system_component_name

    @property
    def name(self) -> str:
        return "add_feature_processor_registration"

    @property
    def description(self) -> str:
        return "Register an Atom FeatureProcessor with FeatureProcessorFactory from a gem's RenderingSystemComponent"

    @property
    def author(self) -> str:
        return "O3DE"

    # ------------------------------------------------------------------
    # Main entry
    # ------------------------------------------------------------------

    def execute(self, ctx: CommandContext) -> bool:
        if not self.feature_processor_name:
            ctx.log("add_feature_processor_registration: feature_processor_name is required, skipping")
            return True

        sys_comp_name = self.system_component_name or f"{ctx.namespace}RenderingSystemComponent"
        cpp_path = self._find_system_component(ctx, sys_comp_name)

        if cpp_path is None:
            ctx.log(
                f"add_feature_processor_registration: {sys_comp_name}.cpp not found. "
                "Either copy_template_files / copy_variant_files did not run, or this "
                "variant does not include a RenderingSystemComponent. Skipping."
            )
            return True

        ctx.log(f"add_feature_processor_registration: registering '{self.feature_processor_name}' in {sys_comp_name}.cpp")

        text = cpp_path.read_text(encoding="utf-8")
        original = text

        text = self._ensure_include(text, f'{self.feature_processor_name}.h')
        text = self._ensure_factory_include(text)
        text = self._inject_call(
            text,
            method=f"{sys_comp_name}::Activate",
            sentinel=f'RegisterFeatureProcessor<{self.feature_processor_name}>',
            line=(f'AZ::RPI::FeatureProcessorFactory::Get()'
                  f'->RegisterFeatureProcessor<{self.feature_processor_name}>();'),
            ctx=ctx,
        )
        text = self._inject_call(
            text,
            method=f"{sys_comp_name}::Deactivate",
            sentinel=f'UnregisterFeatureProcessor<{self.feature_processor_name}>',
            line=(f'AZ::RPI::FeatureProcessorFactory::Get()'
                  f'->UnregisterFeatureProcessor<{self.feature_processor_name}>();'),
            ctx=ctx,
        )

        if text == original:
            ctx.log("add_feature_processor_registration: no changes needed (already registered).")
            return True

        with open(cpp_path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
        ctx.log(f"add_feature_processor_registration: updated {cpp_path}")
        return True

    # ------------------------------------------------------------------
    # File discovery (mirrors add_pass_creator_call)
    # ------------------------------------------------------------------

    @staticmethod
    def _find_system_component(ctx: CommandContext, sys_comp_name: str) -> Optional[Path]:
        candidates = [
            ctx.dest_root / "Code" / "Source" / f"{sys_comp_name}.cpp",
            ctx.dest_root / "Source" / f"{sys_comp_name}.cpp",
            ctx.dest_root / "Code" / "Source" / "Tools" / f"{sys_comp_name}.cpp",
            ctx.dest_root / "Source" / "Tools" / f"{sys_comp_name}.cpp",
        ]
        for c in candidates:
            if c.is_file():
                return c
        return None

    # ------------------------------------------------------------------
    # Text edits (mirror add_pass_creator_call)
    # ------------------------------------------------------------------

    @staticmethod
    def _ensure_include(text: str, header: str) -> str:
        line = f'#include "{header}"'
        if line in text:
            return text

        lines = text.splitlines()
        last_include = -1
        for i, ln in enumerate(lines):
            if ln.lstrip().startswith("#include"):
                last_include = i

        if last_include == -1:
            return line + "\n" + text

        lines.insert(last_include + 1, line)
        return "\n".join(lines) + ("\n" if text.endswith("\n") else "")

    @staticmethod
    def _ensure_factory_include(text: str) -> str:
        line = "#include <Atom/RPI.Public/FeatureProcessorFactory.h>"
        if line in text:
            return text

        lines = text.splitlines()
        last_angle = -1
        for i, ln in enumerate(lines):
            if ln.lstrip().startswith("#include <"):
                last_angle = i

        insert_at = (last_angle + 1) if last_angle >= 0 else 0
        lines.insert(insert_at, line)
        return "\n".join(lines) + ("\n" if text.endswith("\n") else "")

    def _inject_call(self, text: str, method: str, sentinel: str,
                     line: str, ctx: CommandContext) -> str:
        if sentinel in text:
            ctx.log(f"  '{method}' already contains '{sentinel}', skipping")
            return text

        body = self._find_method_body(text, method)
        if body is None:
            ctx.log(f"  Warning: could not locate '{method}' body, skipping injection")
            return text

        body_start, body_end = body

        close_line_start = text.rfind('\n', 0, body_end) + 1
        close_indent = ''
        i = close_line_start
        while i < body_end and text[i] in ' \t':
            close_indent += text[i]
            i += 1

        inner = text[body_start:close_line_start]
        body_indent = None
        for inner_line in inner.splitlines():
            if inner_line.strip():
                lead = re.match(r'^[ \t]+', inner_line)
                if lead:
                    body_indent = lead.group(0)
                break
        indent = body_indent or (close_indent + '    ')

        had_content = bool(inner.strip())
        sep = "\n" if had_content else ""

        injected = f"{sep}{indent}{line}\n"
        return text[:close_line_start] + injected + text[close_line_start:]

    @staticmethod
    def _find_method_body(text: str, method: str) -> Optional[Tuple[int, int]]:
        sig_pattern = re.compile(re.escape(method) + r'\s*\([^)]*\)\s*\{')
        m = sig_pattern.search(text)
        if not m:
            return None

        depth = 1
        i = m.end()
        body_start = i
        while i < len(text) and depth > 0:
            c = text[i]
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    return body_start, i
            i += 1
        return None
