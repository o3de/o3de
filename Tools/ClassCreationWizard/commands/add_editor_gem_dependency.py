#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

from gem_dependency import apply_to_target, editor_counterpart_name
from command_plugin import WizardCommand, CommandContext, CommandRegistry, CMakeAnalyzer


@CommandRegistry.register("add_editor_gem_dependency")
class AddEditorGemDependencyCommand(WizardCommand):
    """Add a gem dependency to the EDITOR counterpart of the target the wizard selected.

    The wizard's dropdown selects a RUNTIME target (e.g. <Gem>.API, <Gem>.Private.Object, or the
    bare <Gem> shared module) as ctx.build_target. This command maps that to its editor
    counterpart by O3DE's gem naming convention (<Gem> -> <Gem>.Editor, <Gem>.API ->
    <Gem>.Editor.API, <Gem>.Private.Object -> <Gem>.Editor.Private.Object, ...) and edits THAT
    target instead -- reactively, so it follows whatever the user actually picked rather than
    assuming a fixed target name.

    No-ops (with a log line, not an error) when the selected target has no editor counterpart in
    the file, e.g. the gem builds without host tools, or the picked target genuinely has none.
    """

    def __init__(self, dependency: str):
        self.dependency = dependency

    @property
    def name(self) -> str:
        return "add_editor_gem_dependency"

    @property
    def description(self) -> str:
        return "Add gem dependency to the editor counterpart of the selected target"

    def execute(self, ctx: CommandContext) -> bool:
        # Guard: don't add a gem as a dependency of itself
        dep_name = self.dependency
        if dep_name.startswith("Gem::"):
            dep_name = dep_name[len("Gem::"):]
        dep_gem = dep_name.split(".")[0]  # Strip .API, .Private, etc.
        if dep_gem == ctx.namespace:
            ctx.log(f"Skipping self-dependency: {self.dependency} (target gem is {ctx.namespace})")
            return True

        if not ctx.build_target:
            ctx.log("Warning: No build target selected")
            return True

        editor_name = editor_counterpart_name(ctx.build_target.name, ctx.namespace)
        if editor_name is None:
            ctx.log(f"Warning: '{ctx.build_target.name}' has no editor counterpart to target, "
                    f"skipping dependency '{self.dependency}'")
            return True

        editor_target = next(
            (t for t in CMakeAnalyzer.scan_targets(ctx.build_target.file.parent, ctx.namespace)
             if t.name == editor_name),
            None)
        if editor_target is None:
            ctx.log(f"Warning: could not find editor target '{editor_name}', "
                    f"skipping dependency '{self.dependency}'")
            return True

        ctx.log(f"Adding dependency '{self.dependency}' to editor target '{editor_target.name}'...")
        return apply_to_target(ctx, editor_target, self.dependency)
