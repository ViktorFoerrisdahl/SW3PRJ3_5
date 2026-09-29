"""Regression tests for document discovery, conversion, and synchronization."""

import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest
import zipfile

from pypdf import PdfWriter

SPEC = importlib.util.spec_from_file_location('generate', Path(__file__).resolve().parents[1] / 'generate.py')
generate = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generate)


class ContextTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.docs = self.root / 'docs'
        self.docs.mkdir()

    def write(self, name, content='En dansk tekst med æ, ø og å.'):
        path = self.docs / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
        return path

    def manifest(self):
        return json.loads((self.docs / 'context/manifest.json').read_text())

    def test_unicode_duplicate_names_links_and_repeatability(self):
        self.write('Møder #1/Noter.md')
        self.write('Analyse/Noter.md', 'En anden kilde.')
        self.assertFalse(generate.build(self.root))
        entries = self.manifest()
        self.assertEqual(len({entry['output'] for entry in entries}), 2)
        index = (self.docs / 'context/INDEX.md').read_text()
        self.assertIn('M%C3%B8der%20%231/Noter.md', index)
        before = {p.name: p.read_bytes() for p in (self.docs / 'context').iterdir()}
        self.assertFalse(generate.build(self.root))
        after = {p.name: p.read_bytes() for p in (self.docs / 'context').iterdir()}
        self.assertEqual(before, after)
        self.assertFalse(generate.check(self.root))

    def test_changes_deletions_and_renames_remove_old_outputs(self):
        original = self.write('før.md')
        generate.build(self.root)
        old_output = self.docs / 'context' / self.manifest()[0]['output']
        original.rename(self.docs / 'efter.md')
        self.assertTrue(generate.check(self.root))
        generate.build(self.root)
        self.assertFalse(old_output.exists())
        self.write('efter.md', 'Ændret indhold')
        self.assertTrue(generate.check(self.root))
        generate.build(self.root)
        (self.docs / 'efter.md').unlink()
        generate.build(self.root)
        self.assertEqual(self.manifest(), [])
        self.assertEqual({p.name for p in (self.docs / 'context').iterdir()}, {'INDEX.md', 'manifest.json'})

    def test_generated_and_hidden_files_are_excluded(self):
        self.write('kilde.md')
        self.write('context/manual.md', 'This is generated territory.')
        self.write('.DS_Store')
        self.write('.hidden/private.md')
        generate.build(self.root)
        self.assertEqual([entry['source'] for entry in self.manifest()], ['kilde.md'])
        self.assertFalse((self.docs / 'context/manual.md').exists())

    def test_unknown_formats_and_images_are_not_silently_lost(self):
        self.write('diagram.png', 'Image placeholder')
        self.write('noter.loop', 'Unsupported data')
        self.assertFalse(generate.build(self.root))
        self.assertEqual(len(self.manifest()), 2)
        self.assertTrue(all(entry['status'] == 'manuel læsning kræves' for entry in self.manifest()))

    def test_corrupt_supported_document_fails_and_retains_diagnostic(self):
        self.write('broken.pdf', 'Not a PDF')
        self.assertTrue(generate.build(self.root))
        self.assertEqual(self.manifest()[0]['status'], 'FEJL')
        self.assertTrue(generate.check(self.root))
        output = self.docs / 'context' / self.manifest()[0]['output']
        self.assertIn('Konvertering fejlede', output.read_text())

    def test_empty_pdf_page_is_flagged(self):
        writer = PdfWriter()
        writer.add_blank_page(width=100, height=100)
        writer.write(self.docs / 'scanned.pdf')
        self.assertFalse(generate.build(self.root))
        output = self.docs / 'context' / self.manifest()[0]['output']
        self.assertIn('Sider uden udtrukket tekst: 1', output.read_text())

    def test_utf16_shortcut_does_not_fetch_external_content(self):
        (self.docs / 'link.url').write_text('[InternetShortcut]\nURL=https://example.invalid/test\n', encoding='utf-16')
        self.assertFalse(generate.build(self.root))
        output = self.docs / 'context' / self.manifest()[0]['output']
        self.assertIn('https://example.invalid/test', output.read_text())
        self.assertEqual(self.manifest()[0]['status'], 'manuel læsning kræves')

    def test_symlink_source_is_rejected(self):
        outside = self.root / 'outside.txt'
        outside.write_text('Not a source')
        (self.docs / 'link.txt').symlink_to(outside)
        with self.assertRaises(ValueError):
            generate.build(self.root)

    def test_missing_output_is_stale(self):
        self.write('source.md')
        generate.build(self.root)
        (self.docs / 'context' / self.manifest()[0]['output']).unlink()
        self.assertTrue(generate.check(self.root))

    def test_edited_output_and_missing_index_are_stale(self):
        self.write('source.md')
        generate.build(self.root)
        output = self.docs / 'context' / self.manifest()[0]['output']
        output.write_text('An accidental manual edit')
        self.assertTrue(generate.check(self.root))
        generate.build(self.root)
        (self.docs / 'context/INDEX.md').unlink()
        self.assertTrue(generate.check(self.root))

    def test_modified_index_and_orphaned_outputs_are_stale(self):
        self.write('source.md')
        generate.build(self.root)
        (self.docs / 'context/INDEX.md').write_text('# Outdated index')
        self.assertTrue(generate.check(self.root))
        generate.build(self.root)
        (self.docs / 'context/orphan.md').write_text('Deleted source content')
        self.assertTrue(generate.check(self.root))
        generate.build(self.root)
        self.assertFalse(generate.check(self.root))
        self.assertFalse((self.docs / 'context/orphan.md').exists())

    @unittest.skipUnless(shutil.which('pandoc'), 'Pandoc is required for DOCX integration.')
    def test_docx_paragraph_table_and_formula(self):
        with zipfile.ZipFile(self.docs / 'test.docx', 'w') as archive:
            archive.writestr('[Content_Types].xml', '''<?xml version="1.0"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
<Default Extension="xml" ContentType="application/xml"/>
<Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>''')
            archive.writestr('_rels/.rels', '''<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>''')
            archive.writestr('word/_rels/document.xml.rels', '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>')
            archive.writestr('word/document.xml', '''<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><w:body>
<w:p><w:r><w:t>Dansk hændelse</w:t></w:r></w:p>
<w:tbl><w:tblPr/><w:tblGrid><w:gridCol w:w="3000"/></w:tblGrid><w:tr><w:tc><w:p><w:r><w:t>sample_rate_hz</w:t></w:r></w:p></w:tc></w:tr></w:tbl>
<w:p><m:oMath><m:r><m:t>x=1</m:t></m:r></m:oMath></w:p>
<w:sectPr/></w:body></w:document>''')
        self.assertFalse(generate.build(self.root))
        output = self.docs / 'context' / self.manifest()[0]['output']
        content = output.read_text()
        self.assertIn('Dansk hændelse', content)
        self.assertIn('sample_rate_hz', content)
        self.assertRegex(content, r'x\s*=\s*1')
        self.assertNotIn('FEJL', content)


if __name__ == '__main__':
    unittest.main()
