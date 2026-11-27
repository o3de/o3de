#!/usr/bin/env python3
import importlib.util
import pathlib
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('integrate_viewsrg', pathlib.Path(__file__).with_name('integrate_viewsrg.py'))
integration = importlib.util.module_from_spec(spec)
spec.loader.exec_module(integration)

class IntegrationTests(unittest.TestCase):
    def make_project(self, path):
        (path / 'project.json').write_text('{"project_name":"IntegrationTest"}')

    def test_preserves_custom_fields_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            project = pathlib.Path(directory)
            self.make_project(project)
            shaderlib = project / 'ShaderLib'
            shaderlib.mkdir()
            target = shaderlib / 'viewsrg.srgi'
            original = ('#pragma once\r\npartial ShaderResourceGroup ViewSrg : SRG_PerView\r\n'
                        '{\r\n    float4 m_myExistingSetting;\r\n};\r\n// keep this  \r\n').encode()
            target.write_bytes(b'\xef\xbb\xbf' + original)
            _, backup, changed = integration.integrate(project)
            self.assertTrue(changed)
            self.assertEqual(backup.read_bytes(), b'\xef\xbb\xbf' + original)
            installed = target.read_bytes()
            self.assertTrue(installed.startswith(b'\xef\xbb\xbf' + original))
            self.assertEqual(installed.count(integration.START.encode()), 1)
            self.assertFalse(integration.integrate(project)[2])
            self.assertEqual(target.read_bytes(), installed)
            # Simulate a user's unrelated edit between installation and removal.
            with target.open('ab') as file:
                file.write(b'\r\n// later project edit\r\n')
            integration.integrate(project, remove=True)
            result = target.read_bytes()
            self.assertTrue(result.startswith(b'\xef\xbb\xbf' + original))
            self.assertIn(b'// later project edit', result)
            self.assertNotIn(integration.START.encode(), result)

    def test_new_project_and_remove_without_install(self):
        with tempfile.TemporaryDirectory() as directory:
            project = pathlib.Path(directory)
            self.make_project(project)
            path, _, changed = integration.integrate(project, remove=True)
            self.assertFalse(changed)
            self.assertFalse(path.exists())
            integration.integrate(project)
            self.assertIn('partial ShaderResourceGroup ViewSrg : SRG_PerView', path.read_text())

    def test_refuses_unknown_srg_without_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            project = pathlib.Path(directory)
            self.make_project(project)
            (project / 'ShaderLib').mkdir()
            target = project / 'ShaderLib/viewsrg.srgi'
            original = 'ShaderResourceGroup ViewSrg : SRG_PerView {};'
            target.write_text(original)
            with self.assertRaises(ValueError): integration.integrate(project)
            self.assertEqual(target.read_text(), original)

    def test_refuses_edited_managed_block(self):
        with tempfile.TemporaryDirectory() as directory:
            project = pathlib.Path(directory)
            self.make_project(project)
            target, _, _ = integration.integrate(project)
            target.write_text(target.read_text().replace('m_sceneColorViewAvailable', 'm_editedField'))
            original = target.read_bytes()
            with self.assertRaises(ValueError): integration.integrate(project, remove=True)
            self.assertEqual(target.read_bytes(), original)

    def test_migrates_legacy_include_and_preserves_project(self):
        with tempfile.TemporaryDirectory() as directory:
            project = pathlib.Path(directory)
            self.make_project(project)
            (project / 'ShaderLib').mkdir()
            target = project / 'ShaderLib/viewsrg.srgi'
            original = integration.DEFAULT + '\n// unrelated project setting\n' + integration.LEGACY_BLOCK + '\n'
            target.write_text(original)
            _, backup, changed = integration.integrate(project)
            self.assertTrue(changed)
            self.assertEqual(backup.read_text(), original)
            self.assertIn('// unrelated project setting', target.read_text())
            self.assertNotIn('#include <SceneColorView/', target.read_text())
            self.assertIn(integration.BLOCK, target.read_text())
            self.assertFalse(integration.integrate(project)[2])

    def test_disabled_gem_needs_no_gem_shader_includes(self):
        with tempfile.TemporaryDirectory() as directory:
            project = pathlib.Path(directory)
            self.make_project(project)
            target, _, _ = integration.integrate(project)
            block = target.read_text().split(integration.START, 1)[1].split(integration.END, 1)[0]
            # Every added declaration is local. Removing the Gem's entire
            # include search path cannot invalidate a dependency in this block.
            self.assertNotIn('#include', block)
            self.assertIn('Texture2D<float4> m_sceneColorViewImage;', block)
            self.assertIn('uint m_sceneColorViewAvailable;', block)
            self.assertIn('float2 m_sceneColorViewTexelSize;', block)
            self.assertIn('Sampler m_sceneColorViewSampler', block)

if __name__ == '__main__':
    unittest.main()
