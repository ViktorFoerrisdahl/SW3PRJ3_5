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
from urllib.parse import quote
import zipfile

from pypdf import PdfReader

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


def pandoc_text(path, relative):
    result = subprocess.run(
        ['pandoc', '--from', PANDOC_FORMATS[path.suffix.lower()], '--to', 'json', str(path)],
        capture_output=True, text=True, check=True, timeout=120)
    tree = json.loads(result.stdout)
    image_count = 0

    def replace_images(node):
        nonlocal image_count
        if isinstance(node, list):
            return [replace_images(item) for item in node]
        if not isinstance(node, dict):
            return node
        if node.get('t') == 'Image':
            image_count += 1
            # The original contains the image; do not create broken media links.
            return {'t': 'Link', 'c': [node['c'][0],
                    [{'t': 'Str', 'c': 'Illustration: se originaldokumentet'}],
                    [source_link(relative), '']]}
        return {key: replace_images(value) for key, value in node.items()}

    tree = replace_images(tree)
    converted = subprocess.run(
        ['pandoc', '--from', 'json', '--to', 'gfm', '--wrap=none'],
        input=json.dumps(tree), capture_output=True, text=True, check=True, timeout=120)
    warnings = ['Automatisk tekstudtræk: kontrollér layout, formler og tabeller i originalen.']
    if image_count:
        warnings.append(f'{image_count} illustration(er) skal læses i originalen; billedindhold er ikke konverteret.')
    if path.suffix.lower() == '.docx':
        with zipfile.ZipFile(path) as archive:
            if any(name.startswith('word/media/') for name in archive.namelist()) and not image_count:
                warnings.append('Dokumentet indeholder medier, som kræver manuel læsning.')
    if result.stderr.strip() or converted.stderr.strip():
        warnings.append('Pandoc gav en advarsel; kontrollér originaldokumentet og kørselsloggen.')
        print(result.stderr + converted.stderr, file=sys.stderr)
    return converted.stdout, 'tekst udtrukket – kontrollér original', warnings


def extract(path, relative):
    suffix = path.suffix.lower()
    if suffix in {'.md', '.txt', '.csv', '.tsv', '.json', '.yaml', '.yml'}:
        content = path.read_text(encoding='utf-8-sig')
        if suffix != '.md':
            fence = '`' * max(3, max((len(x) + 1 for x in re.findall(r'`+', content)), default=3))
            content = f'{fence}\n{content}\n{fence}\n'
        return content, 'tekst kopieret', ['Relative links i kildeteksten er relative til originalfilens mappe.']
    if suffix in PANDOC_FORMATS:
        return pandoc_text(path, relative)
    if suffix == '.pdf':
        reader = PdfReader(path)
        parts = []
        empty = []
        for number, page in enumerate(reader.pages, 1):
            content = page.extract_text() or ''
            if not content.strip():
                empty.append(str(number))
            parts.append(f'## Side {number}\n\n{content}')
        warnings = ['PDF-tekst kan miste læserækkefølge, formler, tabeller og illustrationer. Kontrollér originalen.']
        if empty:
            warnings.append('Sider uden udtrukket tekst: ' + ', '.join(empty) + '. OCR eller manuel transskription kræves.')
        return '\n\n'.join(parts), 'tekst udtrukket – kontrollér original', warnings
    if suffix == '.url':
        raw = path.read_bytes()
        content = raw.decode('utf-16') if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else raw.decode('utf-8-sig')
        urls = re.findall(r'^URL=(.+)$', content, re.MULTILINE | re.IGNORECASE)
        return '\n'.join(f'- Eksternt mål: `{url.strip()}`' for url in urls), 'manuel læsning kræves', [
            'Genvejen indeholder kun en adresse. Målets indhold er ikke hentet; eksportér det til docs/.']
    if suffix in IMAGES:
        return f'[Åbn billedet]({source_link(relative)})\n', 'manuel læsning kræves', [
            'Billedtekst og diagramrelationer er ikke udtrukket. Læs billedet og tilføj en dansk Markdown-beskrivelse ved siden af originalen.']
    return '', 'manuel læsning kræves', [
        f'Formatet {suffix or "uden filendelse"} understøttes ikke til tekstudtræk. Eksportér til DOCX, PDF eller Markdown; originalen er bevaret.']


def render_index(entries):
    lines = ['# Projektkontekst', '',
             'Genereret fra `docs/`. Originalerne er kilderne; kopierne er et læseindeks.', '',
             'Læs først AI-strategien, terminology.md og code-conventions.md via listen nedenfor. '
             'Læs derefter relevante analyser og referater. Se AGENTS.md i roden.', '',
             '**Manuel læsning kræves** betyder, at indhold mangler i tekstkonteksten. '
             'Tekstudtræk af PDF og kontordokumenter er heller ikke en fuldstændig gengivelse.', '',
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
                content, status, warnings = extract(path, relative)
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
                            'output_sha256': digest(staging / name), 'status': status})
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
    expected_files = {'INDEX.md', 'manifest.json'} | {entry['output'] for entry in entries}
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
