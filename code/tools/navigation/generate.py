"""Maintain generated file navigation in the code and docs READMEs."""

import argparse
from pathlib import Path
import re
import subprocess
from urllib.parse import quote, unquote, urlsplit

ROOT = Path(__file__).resolve().parents[3]
START = '<!-- BEGIN GENERATED NAVIGATION -->'
END = '<!-- END GENERATED NAVIGATION -->'


def source_files(root):
    result = subprocess.run(
        ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z', '--', 'code', 'docs'],
        cwd=root, capture_output=True, check=True)
    return sorted({Path(name.decode('utf-8')) for name in result.stdout.split(b'\0')
                   if name and (root / name.decode('utf-8')).is_file()}, key=lambda p: p.as_posix().casefold())


def navigation(area, files):
    lines = ['## Filoversigt', '', 'Dette afsnit opdateres automatisk af CI. Ret teksten uden for markørerne.', '']
    if area == 'docs':
        lines += ['[Genereret AI-kontekst og kildeindeks](context/INDEX.md)', '']
    for path in files:
        if path.parts[0] != area or path == Path(area) / 'README.md':
            continue
        relative = path.relative_to(area)
        if relative.parts[0] == 'context' and area == 'docs':
            continue
        label = relative.as_posix().replace('\\', '\\\\').replace('[', r'\[').replace(']', r'\]')
        lines.append(f'- [{label}]({quote(relative.as_posix(), safe="/")})')
    return '\n'.join(lines) + '\n'


def update_text(text, body):
    if text.count(START) != 1 or text.count(END) != 1 or text.index(START) >= text.index(END):
        raise ValueError('README skal indeholde præcis ét korrekt placeret sæt navigationsmarkører.')
    before, rest = text.split(START)
    _, after = rest.split(END)
    return before + START + '\n\n' + body + '\n' + END + after


def check_links(path, text):
    # These READMEs use inline Markdown links; ignore examples in fenced code.
    text = re.sub(r'^```.*?^```[^\n]*$', '', text, flags=re.MULTILINE | re.DOTALL)
    for raw in re.findall(r'\]\((<[^>]+>|[^\s)]+)(?:\s+"[^"\n]*")?\)', text):
        target = raw.strip('<>')
        url = urlsplit(target)
        if url.scheme or url.netloc or not url.path:
            continue
        if not (path.parent / unquote(url.path)).exists():
            raise ValueError(f'Dødt lokalt link i {path}: {target}')


def run(root, check=False):
    files = source_files(root)
    stale = False
    for area in ('code', 'docs'):
        path = root / area / 'README.md'
        text = path.read_text(encoding='utf-8')
        updated = update_text(text, navigation(area, files))
        if check:
            if updated != text:
                print(f'Forældet navigation: {area}/README.md')
                stale = True
            check_links(path, text)
        elif updated != text:
            path.write_text(updated, encoding='utf-8')
            print(f'Opdateret: {area}/README.md')
    return stale


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Check navigation and local link targets.')
    args = parser.parse_args()
    raise SystemExit(run(ROOT, args.check))
