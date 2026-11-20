#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

from typing import Optional

from gem_dependency import apply_to_target
from command_plugin import WizardCommand, CommandContext, CommandRegistry, CMakeAnalyzer, CMakeTarget


@CommandRegistry.register("add_gem_dependency")
class AddGemDependencyCommand(WizardCommand):
    """Add a gem dependency to the PRIVATE list of a target's BUILD_DEPENDENCIES.

    By default edits the target the wizard's dropdown selected (ctx.build_target). Pass `target`
    (a name suffix, e.g. "DataAsset.Builder") to edit a different, explicitly named target instead.

    For the common case of "the editor counterpart of whatever target got selected" -- e.g. an
    editor component's Source/Tools/Editor*.cpp needs AZ::AzToolsFramework on the EDITOR target,
    not whichever runtime target the dropdown happens to have selected -- use
    add_editor_gem_dependency instead, which derives that target name reactively rather than
    hardcoding a suffix that only holds for one particular target selection.
    """

    def __init__(self, dependency: str, target: Optional[str] = None):
        self.dependency = dependency
        self.target_suffix = target

    @property
    def name(self) -> str:
        return "add_gem_dependency"

    @property
    def description(self) -> str:
        return "Add gem dependency to BUILD_DEPENDENCIES in CMake"

    def execute(self, ctx: CommandContext) -> bool:
        # Guard: don't add a gem as a dependency of itself
        dep_name = self.dependency
        if dep_name.startswith("Gem::"):
            dep_name = dep_name[len("Gem::"):]
        dep_gem = dep_name.split(".")[0]  # Strip .API, .Private, etc.
        if dep_gem == ctx.namespace:
            ctx.log(f"Skipping self-dependency: {self.dependency} (target gem is {ctx.namespace})")
            return True

        build_target = ctx.build_target
        if self.target_suffix:
            build_target = self._resolve_named_target(ctx)
            if build_target is None:
                ctx.log(f"Warning: could not find a target ending in '.{self.target_suffix}', "
                        f"skipping dependency '{self.dependency}'")
                return True
        elif not build_target:
            ctx.log("Warning: No build target selected")
            return True

        ctx.log(f"Adding dependency '{self.dependency}' to target '{build_target.name}'...")
        return apply_to_target(ctx, build_target, self.dependency)

    def _resolve_named_target(self, ctx: CommandContext) -> Optional[CMakeTarget]:
        """Find the target named '<namespace>.<target_suffix>', scanning from ctx.build_target's CMake dir.

        Used when self.target_suffix asks to edit a target OTHER than the one the wizard selected.
        """
        if not ctx.build_target:
            return None

        wanted = f"{ctx.namespace}.{self.target_suffix}"
        for candidate in CMakeAnalyzer.scan_targets(ctx.build_target.file.parent, ctx.namespace):
            if candidate.name == wanted:
                return candidate
        return None
