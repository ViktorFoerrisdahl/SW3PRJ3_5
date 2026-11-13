"""Build a flat, traceable Markdown view of repository documentation."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unicodedata
from urllib.parse import quote, urlsplit, urlunsplit
import posixpath
import zipfile

from pypdf import PdfReader
from code.tools.context.quality import audit_docx, ocr

ROOT = Path(__file__).resolve().parents[3]
PANDOC_FORMATS = {'.docx': 'docx', '.odt': 'odt', '.rtf': 'rtf', '.html': 'html',
                  '.htm': 'html', '.rst': 'rst', '.epub': 'epub'}
IMAGES = {'.png', '.jpg', '.jpeg', '.gif', '.webp', '.svg', '.tif', '.tiff'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def output_name(relative):
    # Hash the full path to distinguish identical names in different directories.
    slug = re.sub(r'[^a-z0-9]+', '-', relative.stem.lower()).strip('-')[:65] or 'dokument'
    key = hashlib.sha256(relative.as_posix().encode()).hexdigest()[:16]
    return f'{slug}--{key}.md'


def source_link(relative):
    return '../' + quote(relative.as_posix(), safe='/')


def pandoc_text(path, relative, staging):
    result = subprocess.run(
        ['pandoc', '--from', PANDOC_FORMATS[path.suffix.lower()], '--to', 'json', str(path)],
        capture_output=True, text=True, check=True, timeout=120)
    tree = json.loads(result.stdout)
    warnings, summary = audit_docx(path, tree) if path.suffix.lower() == '.docx' else ([], '')
    images = []

    def preserve_images(node):
        if isinstance(node, list):
            return [preserve_images(item) for item in node]
        if not isinstance(node, dict):
            return node
        if node.get('t') == 'Image':
            target = node['c'][-1][0]
            if path.suffix.lower() == '.docx':
                member = 'word/' + target.removeprefix('./')
                with zipfile.ZipFile(path) as archive:
                    if member in archive.namelist() and member.startswith('word/media/'):
                        data = archive.read(member)
                        name = 'asset-' + hashlib.sha256(data).hexdigest()[:20] + Path(member).suffix.lower()
                        (staging / name).write_bytes(data)
                        node['c'][-1][0] = name
                        node['c'][0] = ['', [], []]
                        images.append(name)
                        return node
            warnings.append('En illustration kunne ikke udtrækkes; se originalen.')
            return {'t': 'Link', 'c': [node['c'][0],
                    [{'t': 'Str', 'c': 'Illustration mangler: se originalen'}], [source_link(relative), '']]}
        return {key: preserve_images(value) for key, value in node.items()}

    tree = preserve_images(tree)
    converted = subprocess.run(
        ['pandoc', '--from', 'json', '--to', 'gfm+tex_math_dollars', '--wrap=none'],
        input=json.dumps(tree), capture_output=True, text=True, check=True, timeout=120)
    content = re.sub(r'(?m)^#{1,6}[ \t]*$', '', converted.stdout)
    for name in dict.fromkeys(images):
        try:
            text, notes = ocr(staging / name)
            content += f'\n\n### Billedtekst (OCR) — {name}\n\n{text}\n'
            warnings.extend(notes)
        except (subprocess.SubprocessError, OSError):
            warnings.append(f'OCR kunne ikke køres på {name}; billedet er bevaret i Markdown.')
    if result.stderr.strip() or converted.stderr.strip():
        warnings.append('Pandoc gav en konverteringsadvarsel; se kørselsloggen.')
        print(result.stderr + converted.stderr, file=sys.stderr)
    if summary:
        content += '\n\n---\n\n' + summary + '\n'
    status = 'kontrollér markerede mangler' if warnings else ('tekst og illustrationer' if images else 'klar til læsning')
    if not converted.stdout.strip() and not images and not warnings:
        status = 'tomt dokument'
        content = 'Dokumentet indeholder ingen hovedtekst, tabeller eller formler.\n'
    return content, status, warnings


def pdf_text(path, relative, staging):
    reader = PdfReader(path)
    result = subprocess.run(['pdftotext', '-layout', '-enc', 'UTF-8', str(path), '-'],
                            capture_output=True, text=True, check=True, timeout=120)
    pages = result.stdout.split('\f')
    if pages and not pages[-1].strip():
        pages.pop()
    if len(pages) != len(reader.pages):
        raise ValueError('PDF-sidetallet matcher ikke tekstudtrækket.')
    parts, warnings = [], []
    illustrated = False
    for number, (page, text) in enumerate(zip(reader.pages, pages), 1):
        text = unicodedata.normalize('NFC', text).strip()
        # Preserve page layout for author tables and other aligned text.
        parts.append(f'## Side {number}\n\n' + fenced(text))
        # Render every page so vector graphics and table layout stay available.
        illustrated = True
        name = f'asset-{digest(path)[:20]}-page-{number}'
        subprocess.run(['pdftoppm', '-f', str(number), '-l', str(number), '-scale-to', '1800',
                        '-png', '-singlefile', str(path), str(staging / name)],
                       capture_output=True, check=True, timeout=120)
        parts.append(f'![Side {number}]({name}.png)')
        if not text:
            recognized, notes = ocr(staging / (name + '.png'), psm=3)
            parts.append('### OCR af siden\n\n' + fenced(recognized))
            warnings.append(f'Side {number} er OCR-læst; billedet er bevaret til kontrol af tekst og layout.')
            warnings.extend(f'Side {number}: {note}' for note in notes)
        if '\ufffd' in text:
            warnings.append(f'Side {number} indeholder ugyldige tegn i tekstudtrækket.')
    status = 'kontrollér markerede mangler' if warnings else ('tekst og illustrationer' if illustrated else 'klar til læsning')
    return '\n\n'.join(parts), status, warnings


def fenced(text):
    fence = '`' * max(3, max((len(x) + 1 for x in re.findall(r'`+', text)), default=3))
    return f'{fence}text\n{text}\n{fence}\n'


def markdown_text(path, relative):
    tree = json.loads(subprocess.check_output(['pandoc', '-f', 'gfm+tex_math_dollars', '-t', 'json', str(path)],
                                              text=True, timeout=120))

    def rebase(node):
        if isinstance(node, list):
            return [rebase(item) for item in node]
        if not isinstance(node, dict):
            return node
        if node.get('t') in ('Link', 'Image'):
            target = urlsplit(node['c'][-1][0])
            if not target.scheme and not target.netloc and not target.path.startswith('/'):
                base = '../' + relative.parent.as_posix()
                new_path = posixpath.normpath(base + '/' + target.path) if target.path else '../' + relative.as_posix()
                node['c'][-1][0] = urlunsplit(('', '', new_path, target.query, target.fragment))
        return {key: rebase(value) for key, value in node.items()}

    return subprocess.check_output(['pandoc', '-f', 'json', '-t', 'gfm+tex_math_dollars', '--wrap=none'],
                                   input=json.dumps(rebase(tree)), text=True, timeout=120)


def extract(path, relative, staging):
    suffix = path.suffix.lower()
    if suffix == '.md':
        return markdown_text(path, relative), 'klar til læsning', []
    if suffix in {'.txt', '.csv', '.tsv', '.json', '.yaml', '.yml'}:
        return fenced(path.read_text(encoding='utf-8-sig')), 'tekst kopieret', []
    if suffix in PANDOC_FORMATS:
        return pandoc_text(path, relative, staging)
    if suffix == '.pdf':
        return pdf_text(path, relative, staging)
    if suffix == '.url':
        raw = path.read_bytes()
        content = raw.decode('utf-16') if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else raw.decode('utf-8-sig')
        urls = re.findall(r'^URL=(.+)$', content, re.MULTILINE | re.IGNORECASE)
        return '\n'.join(f'- Eksternt mål: `{url.strip()}`' for url in urls), 'manuel læsning kræves', [
            'Genvejen indeholder kun en adresse. Målets indhold er ikke hentet; eksportér det til docs/.']
    if suffix in IMAGES:
        content = f'![{path.name}]({source_link(relative)})\n'
        try:
            text, notes = ocr(path)
            content += '\n## Billedtekst (OCR)\n\n' + fenced(text)
        except (subprocess.SubprocessError, OSError):
            notes = ['OCR kunne ikke læse dette billedformat; billedet er vist ovenfor.']
        notes.append('OCR gengiver tekst, ikke pile, geometri eller relationer. Brug den viste illustration til visuel information.')
        return content, 'billede med OCR' if '## Billedtekst' in content else 'billede', notes
    return '', 'manuel læsning kræves', [
        f'Formatet {suffix or "uden filendelse"} understøttes ikke til tekstudtræk. Eksportér til DOCX, PDF eller Markdown; originalen er bevaret.']


def render_index(entries):
    lines = ['# Projektkontekst', '',
             'Genereret fra `docs/`. Originalerne er kilderne; kopierne er et læseindeks.', '',
             'Læs først AI-strategien, terminology.md og code-conventions.md via listen nedenfor. '
             'Læs derefter relevante analyser og referater. Se AGENTS.md i roden.', '',
             'Brug Markdown som primær læsekilde. **Klar til læsning** har ingen fundne konverteringsmangler; '
             'det er ikke en faglig godkendelse. Illustrationer vises direkte i kopierne. '
             'Åbn kun originalen ved en konkret mangel, tvivl eller behov for originalt layout.', '',
             '| Kilde | Markdown | Status |', '| --- | --- | --- |']
    for entry in entries:
        label = entry['source'].replace('|', r'\|').replace('[', r'\[').replace(']', r'\]')
        lines.append(f'| [{label}]({source_link(Path(entry["source"]))}) | '
                     f'[Læs]({entry["output"]}) | {entry["status"]} |')
    return '\n'.join(lines) + '\n'


def build(root):
    docs = root / 'docs'
    target = docs / 'context'
    if not docs.is_dir():
        raise ValueError('Mappen docs/ mangler.')
    if target.is_symlink():
        raise ValueError('docs/context må ikke være et symbolsk link.')
    entries = []
    failures = []
    with tempfile.TemporaryDirectory(prefix='atlas-context-') as temp:
        staging = Path(temp)
        for path in sorted(docs.rglob('*')):
            relative = path.relative_to(docs)
            if relative.parts[0] == 'context' or any(part.startswith('.') for part in relative.parts):
                continue
            if path.is_symlink():
                raise ValueError(f'Symbolske links understøttes ikke: {relative}')
            if not path.is_file():
                continue
            name = output_name(relative)
            try:
                content, status, warnings = extract(path, relative, staging)
                if not content.strip() and status != 'manuel læsning kræves':
                    warnings.append('Intet tekstindhold udtrukket. Kontrollér originalen manuelt.')
                    status = 'manuel læsning kræves'
            except Exception as error:
                status = 'FEJL'
                content = ''
                warnings = ['Konvertering fejlede. Brug originalen; se kørselsloggen.']
                failures.append(relative.as_posix())
                print(f'{relative}: {error}', file=sys.stderr)
            sha = digest(path)
            header = (f'# {path.name}\n\n'
                      f'> Genereret fil. Ret originalen, ikke denne kopi.\n\n'
                      f'- Kilde: [{relative.as_posix()}]({source_link(relative)})\n'
                      f'- SHA-256: `{sha}`\n- Status: {status}\n\n'
                      + '\n'.join(f'> {warning}' for warning in warnings) + '\n\n---\n\n')
            (staging / name).write_text(header + content.rstrip() + '\n', encoding='utf-8')
            entries.append({'source': relative.as_posix(), 'sha256': sha, 'output': name,
                            'output_sha256': digest(staging / name), 'status': status,
                            'notes': warnings})
        assets = {p.name: digest(p) for p in staging.glob('asset-*')}
        (staging / 'assets.json').write_text(json.dumps(assets, indent=2) + '\n', encoding='utf-8')
        (staging / 'INDEX.md').write_text(render_index(entries), encoding='utf-8')
        (staging / 'manifest.json').write_text(json.dumps(entries, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        target.mkdir(exist_ok=True)
        # This directory is exclusively generated. Remove entries for deleted/renamed sources.
        for old in target.iterdir():
            if old.is_dir() and not old.is_symlink():
                shutil.rmtree(old)
            else:
                old.unlink()
        for generated in staging.iterdir():
            shutil.copyfile(generated, target / generated.name)
    print(f'{len(entries)} kilder behandlet; {len(failures)} konverteringsfejl.')
    return bool(failures)


def check(root):
    docs = root / 'docs'
    manifest = docs / 'context/manifest.json'
    if not manifest.exists() or not (docs / 'context/INDEX.md').is_file():
        return True
    entries = json.loads(manifest.read_text(encoding='utf-8'))
    actual = {p.relative_to(docs).as_posix(): digest(p) for p in docs.rglob('*')
              if p.is_file() and p.relative_to(docs).parts[0] != 'context'
              and not any(part.startswith('.') for part in p.relative_to(docs).parts)}
    expected = {entry['source']: entry['sha256'] for entry in entries}
    if (docs / 'context/INDEX.md').read_text(encoding='utf-8') != render_index(entries):
        return True
    assets_path = docs / 'context/assets.json'
    if not assets_path.is_file():
        return True
    assets = json.loads(assets_path.read_text(encoding='utf-8'))
    for name, sha in assets.items():
        path = docs / 'context' / name
        if not path.is_file() or digest(path) != sha:
            return True
    expected_files = {'INDEX.md', 'manifest.json', 'assets.json'} | set(assets) | {entry['output'] for entry in entries}
    actual_files = {p.name for p in (docs / 'context').iterdir() if not p.name.startswith('.')}
    if actual_files != expected_files:
        return True
    if actual != expected:
        return True
    for entry in entries:
        output = docs / 'context' / entry['output']
        if entry['status'] == 'FEJL' or not output.is_file():
            return True
        if digest(output) != entry.get('output_sha256'):
            return True
    return False


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Check source hashes without converting.')
    args = parser.parse_args()
    sys.exit(check(ROOT) if args.check else build(ROOT))
