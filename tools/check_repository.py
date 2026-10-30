"""Check maintained documentation links, roadmap structure and source boundaries."""
import argparse
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def check(root=ROOT):
    errors=[];links=0;pages=[]
    for folder in ('docs','examples','mod-porter','targets','tests'):
        pages.extend(p for p in (root/folder).rglob('*.md') if not any(part in ('bin','obj','build','__pycache__','third-party') for part in p.relative_to(root/folder).parts) and p.name!='RESEARCH-WORKSPACE.md')
    pages += [root/'README.md',root/'CHANGELOG.md']
    for page in pages:
        text=page.read_text(encoding='utf-8-sig')
        for target in re.findall(r'\[[^\]]*\]\(([^)\n]+)\)',text):
            value=target.strip('<>').split('#',1)[0]
            if not value or '://' in value or value.startswith('mailto:'):continue
            if not (page.parent/value).exists():errors.append(f'{page.relative_to(root)}: missing link {value}')
            links+=1
    for name in ('COMPLETION-ROADMAP.md','PC-RUNTIME-ROADMAP.md'):
        text=(root/'docs'/name).read_text(encoding='utf-8')
        if '- [x] ' not in text or '- [ ] ' not in text:errors.append(name+': roadmap needs completed and open milestones')
    if list((root/'tools').glob('test_*.py')):errors.append('Regression runners belong in tests/')
    if list((root/'tools').glob('*fixture.cpp')):errors.append('Compiled fixtures belong in tests/fixtures/')
    for folder in ('runtime','mod-porter','packaging','include','sdk','tools','tests','targets'):
        if not (root/folder).is_dir():errors.append('Missing source category: '+folder)
    for old,new in (('source','runtime'),('desktop','mod-porter'),('distribution','packaging')):
        if (root/old).exists():errors.append(f'Obsolete root directory {old}/; use {new}/')
    for folder in ('guides','reference','development','verification','archive'):
        if not (root/'docs'/folder).is_dir():errors.append('Missing documentation category: '+folder)
    return dict(passed=not errors,pages=len(pages),local_links=links,errors=errors)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',type=Path,default=ROOT)
    result=check(parser.parse_args().root.resolve());print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
