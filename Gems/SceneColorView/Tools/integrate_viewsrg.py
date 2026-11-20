#!/usr/bin/env python3
"""Install a self-contained shared View SRG extension, preserving project edits."""
import argparse
import datetime
import pathlib
import sys

START = '// BEGIN SceneColorView managed include'
END = '// END SceneColorView managed include'
LEGACY_BLOCK = START + '\n#include <SceneColorView/ViewSrg.azsli>\n' + END
# Copy declarations, not an include: disabled Gems no longer supply ShaderLib
# search paths. This project-local block must survive without the Gem installed.
DECLARATIONS_PATH = pathlib.Path(__file__).resolve().parents[1] / 'Assets/ShaderLib/SceneColorView/ViewSrg.azsli'
DECLARATIONS = DECLARATIONS_PATH.read_text(encoding='utf-8').split('partial ShaderResourceGroup ViewSrg', 1)[1]
BLOCK = (START + '\n// Self-contained layout extension; safe when SceneColorView is disabled.\n'
         '// Unused declarations do not create a capture pass or allocate a capture image.\n'
         + 'partial ShaderResourceGroup ViewSrg' + DECLARATIONS.rstrip() + '\n' + END)
DEFAULT = '''#pragma once
#include <Atom/Features/SrgSemantics.azsli>

partial ShaderResourceGroup ViewSrg : SRG_PerView
{
};
'''

def integrate(project, remove=False):
    project = pathlib.Path(project).resolve()
    if not (project / 'project.json').is_file():
        raise ValueError('The project path must contain project.json.')
    path = project / 'ShaderLib' / 'viewsrg.srgi'
    existing = path.read_bytes() if path.exists() else None
    text = existing.decode('utf-8-sig') if existing is not None else DEFAULT
    if text.count(START) != text.count(END) or text.count(START) > 1:
        raise ValueError('Malformed or repeated managed include markers; refusing to edit.')
    if START in text:
        begin = text.index(START)
        end = text.index(END, begin) + len(END)
        current_block = text[begin:end].replace('\r\n', '\n')
        if current_block not in (BLOCK, LEGACY_BLOCK):
            raise ValueError('The managed block was edited; refusing to remove or replace it.')
        if not remove and current_block == BLOCK:
            return path, None, False
        text = text[:begin] + ('' if remove else BLOCK) + text[end:]
    elif remove:
        return path, None, False
    else:
        if 'partial ShaderResourceGroup ViewSrg' not in text:
            raise ValueError('Expected a partial ViewSrg declaration. No project file was changed.')
        if '#include <SceneColorView/ViewSrg.azsli>' in text:
            raise ValueError('An unmanaged SceneColorView include is already present. Keep only one include.')
        text = text + ('' if text.endswith('\n') else '\n') + '\n' + BLOCK + '\n'
    path.parent.mkdir(parents=True, exist_ok=True)
    backup = None
    if existing is not None:
        timestamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
        backup = path.with_name(path.name + '.SceneColorView.' + timestamp + '.bak')
        backup.write_bytes(existing)
    newline = '\r\n' if existing and b'\r\n' in existing else '\n'
    data = text.replace('\r\n', '\n').replace('\n', newline).encode('utf-8')
    if existing and existing.startswith(b'\xef\xbb\xbf'):
        data = b'\xef\xbb\xbf' + data
    temporary = path.with_name(path.name + '.SceneColorView.tmp')
    temporary.write_bytes(data)
    temporary.replace(path)
    return path, backup, True

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', required=True)
    parser.add_argument('--remove', action='store_true')
    args = parser.parse_args()
    try:
        path, backup, changed = integrate(args.project, args.remove)
    except (ValueError, OSError, UnicodeError) as error:
        parser.exit(1, str(error) + '\n')
    print(('Updated: ' if changed else 'Already in requested state: ') + str(path))
    if backup:
        print('Backup: ' + str(backup))
    if not args.remove:
        print('Self-contained View SRG installed. Let SceneAndViewSrgs and affected shaders rebuild.')

if __name__ == '__main__':
    main()
