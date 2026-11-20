"""Extract OCR text and audit DOCX structure without relying on a language model."""

from collections import Counter
import csv
import io
from pathlib import Path
import re
import subprocess
import unicodedata
import xml.etree.ElementTree as ET
import zipfile

NS = {'w': 'http://schemas.openxmlformats.org/wordprocessingml/2006/main',
      'm': 'http://schemas.openxmlformats.org/officeDocument/2006/math'}


def tokens(text):
    return Counter(re.findall(r'\w+', unicodedata.normalize('NFC', text).casefold()))


def nodes(tree, kind):
    if isinstance(tree, dict):
        if tree.get('t') == kind:
            yield tree
        for value in tree.values():
            yield from nodes(value, kind)
    elif isinstance(tree, list):
        for value in tree:
            yield from nodes(value, kind)


def audit_docx(path, tree):
    """Report concrete content losses and unsupported Word features."""
    with zipfile.ZipFile(path) as archive:
        body = ET.fromstring(archive.read('word/document.xml'))
        source = '\n'.join(''.join(node.text or '' for node in para.findall('.//w:t', NS))
                           for para in body.findall('.//w:p', NS))
        extracted = ' '.join(node['c'] for node in nodes(tree, 'Str'))
        extracted += ' ' + ' '.join(node['c'][-1] for kind in ('Code', 'CodeBlock') for node in nodes(tree, kind))
        missing = tokens(source) - tokens(extracted)
        warnings = []
        if missing:
            sample = ', '.join(sorted(missing)[:12])
            warnings.append(f'Tekstkontrol: {sum(missing.values())} ord/tal fra Word-teksten er ikke genfundet i udtrækket ({sample}).')
        for tag, kind, label in [('w:tbl', 'Table', 'tabeller'), ('m:oMath', 'Math', 'formler')]:
            expected = len(body.findall('.//' + tag, NS))
            actual = len(list(nodes(tree, kind)))
            if actual < expected:
                warnings.append(f'Strukturkontrol: kun {actual} af {expected} {label} er bevaret.')
        features = [('w:txbxContent', 'tekstbokse'), ('w:object', 'indlejrede objekter'),
                    ('w:ins', 'registrerede tilføjelser'), ('w:del', 'registrerede sletninger')]
        for tag, label in features:
            if body.findall('.//' + tag, NS):
                warnings.append(f'Word indeholder {label}; kontrollér dette indhold i originalen.')
        if any(name.startswith('word/charts/') for name in archive.namelist()):
            warnings.append('Word indeholder diagramobjekter, som ikke er almindelige billeder.')
        extra_parts = [name for name in archive.namelist() if re.fullmatch(r'word/(header\d+|footer\d+|comments)\.xml', name)]
        for name in extra_parts:
            extra = ET.fromstring(archive.read(name))
            if any((node.text or '').strip() for node in extra.findall('.//w:t', NS)):
                warnings.append(f'Ekstra tekst i {Path(name).name} er ikke en del af hovedteksten.')
        summary = (f'Kontrol: {sum(tokens(source).values())} ord/tal, '
                   f'{len(body.findall(".//w:tbl", NS))} tabeller og '
                   f'{len(body.findall(".//m:oMath", NS))} formler sammenholdt med Word-kilden.')
        return warnings, summary


def ocr(path, psm=11):
    """Return text and concrete OCR cautions; confidence is not an accuracy guarantee."""
    result = subprocess.run(['tesseract', str(path), 'stdout', '-l', 'dan+eng',
                             '--psm', str(psm), '-c', 'tessedit_create_tsv=1'], capture_output=True, text=True,
                            check=True, timeout=120)
    rows = list(csv.DictReader(io.StringIO(result.stdout), delimiter='\t', quoting=csv.QUOTE_NONE))
    lines = {}
    uncertain = 0
    words = 0
    for row in rows:
        text = (row.get('text') or '').strip()
        if not text or row.get('level') != '5':
            continue
        words += 1
        uncertain += float(row['conf']) < 60
        key = tuple(row[name] for name in ('page_num', 'block_num', 'par_num', 'line_num'))
        lines.setdefault(key, []).append(text)
    warnings = []
    if not words:
        warnings.append('OCR fandt ingen tekst; læs den viste illustration.')
    elif uncertain:
        warnings.append(f'OCR markerer {uncertain} af {words} ord som usikre; sammenhold dem med den viste illustration.')
    return '\n'.join(' '.join(line) for line in lines.values()), warnings
