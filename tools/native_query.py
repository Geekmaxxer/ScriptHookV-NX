"""Find exact Switch SDK signatures and static registration coverage, offline."""
import argparse
import hashlib
import json
from pathlib import Path

def search(query, registered=False, root=None):
    root = root or Path(__file__).resolve().parents[1]
    index = json.loads((root/'sdk/native-index.json').read_text())
    if index['header_sha256'] != hashlib.sha256((root/'include/natives.h').read_bytes()).hexdigest():
        raise ValueError('Native index is stale; regenerate it from the matching wrapper coverage report')
    query = query.casefold()
    rows = [n for n in index['natives'] if (query in n['name'].casefold() or query == n['hash'].casefold())
            and (not registered or n['registered'])]
    return sorted(rows, key=lambda n: (not n['name'].casefold().endswith('::'+query), n['name']))

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('query'); p.add_argument('--registered', action='store_true')
    p.add_argument('--limit', type=int, default=20); p.add_argument('--json', action='store_true'); a = p.parse_args()
    if a.limit < 1: p.error('--limit must be positive')
    try: rows = search(a.query, a.registered)
    except (OSError, ValueError, KeyError) as error: p.exit(1, str(error)+'\n')
    if a.json: print(json.dumps(dict(matches=len(rows), results=rows[:a.limit], runtime_verified=False), indent=2))
    else:
        for row in rows[:a.limit]:
            print(row['signature']+'\n  '+row['hash']+'  '+('static target registration' if row['registered'] else 'registration not found'))
        print(f'{len(rows)} matches. Registration is availability evidence, not console behavior verification.')
    raise SystemExit(0 if rows else 2)

if __name__ == '__main__':
    main()
