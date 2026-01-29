import re
from pathlib import Path

from command_plugin import WizardCommand, CommandContext, CommandRegistry


@CommandRegistry.register("register_file_list")
class RegisterFileListCommand(WizardCommand):
    """Add component .h/.cpp files to CMake FILES_CMAKE"""

    is_registration_command = True

    def __init__(self, component_name: str):
        self.component_name = component_name

    @property
    def name(self) -> str:
        return "register_file_list"

    @property
    def description(self) -> str:
        return "Add component .h/.cpp files to CMake FILES_CMAKE"

    def execute(self, ctx: CommandContext) -> bool:
        ctx.log(f"Registering files for '{self.component_name}'...")

        if not ctx.build_target:
            ctx.log("Warning: No build target selected, skipping file registration")
            return True

        rel_hdr = f"Source/{self.component_name}.h"
        rel_cpp = f"Source/{self.component_name}.cpp"

        cmake_path = ctx.build_target.file

        # If target has FILES_CMAKE includes, update those
        if ctx.build_target.files_cmake_list:
            for files_cmake in ctx.build_target.files_cmake_list:
                include_path = (cmake_path.parent / files_cmake).resolve()

                if include_path.exists():
                    self._update_files_cmake(include_path, rel_hdr, rel_cpp, ctx)
                    ctx.log(f"Updated: {files_cmake}")
                    return True

        # Otherwise append target_sources directly to CMakeLists.txt
        cmake_text = cmake_path.read_text(encoding="utf-8")

        append = (
            f"\n# Added by Class Wizard\n"
            f"target_sources({ctx.build_target.name} PRIVATE\n"
            f"    {rel_hdr}\n"
            f"    {rel_cpp}\n"
            f")\n"
        )

        if append not in cmake_text:
            cmake_text = cmake_text.rstrip() + append
            cmake_path.write_text(cmake_text, encoding="utf-8", newline="\n")
            ctx.log(f"Added target_sources block to {cmake_path.name}")

        return True

    def _update_files_cmake(self, files_cmake_path: Path, rel_hdr: str, rel_cpp: str, ctx: CommandContext):
        """Update a FILES_CMAKE include file"""
        if files_cmake_path.exists():
            text = files_cmake_path.read_text(encoding="utf-8")
        else:
            text = "set(FILES\n)\n"

        # Find set(FILES ...) block
        match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
        if match:
            end_pos = match.end(1)

            if rel_hdr not in text:
                text = text[:end_pos] + f"    {rel_hdr}\n" + text[end_pos:]

            match = re.search(r'set\s*\(\s*FILES\b(.*?)(\))', text, flags=re.S | re.M)
            if match:
                end_pos = match.end(1)
                if rel_cpp not in text:
                    text = text[:end_pos] + f"    {rel_cpp}\n" + text[end_pos:]
        else:
            text = text.rstrip() + f"\nset(FILES\n    {rel_hdr}\n    {rel_cpp}\n)\n"

        files_cmake_path.write_text(text, encoding="utf-8", newline="\n")
