#!/usr/bin/env python3
"""Check offline overview aggregates, bounded expansion and byte-safe addressing."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

cli = str(Path(sys.argv[1]).resolve())
project = Path(sys.argv[2])
with tempfile.TemporaryDirectory(prefix='.overview-test-', dir=project) as temporary:
    base = Path(temporary)
    source = base / 'source'
    source.mkdir()

    def file(path, size):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b'x' * size)

    file(source / 'large', 1000)
    for i in range(10):
        file(source / 'small' / str(i), 100)
    file(source / 'a' / 'cache' / 'data', 250)
    file(source / 'a' / 'cache' / 'nested' / 'cache' / 'data', 50)
    file(source / 'b' / 'cache' / 'data', 400)
    file(source / 'пробел space' / 'item', 17)
    raw = os.fsencode(source) + b'/raw-\xff\t\n'
    os.mkdir(raw)
    with open(raw + b'/item', 'wb') as stream:
        stream.write(b'xx')
    (source / 'empty').mkdir()
    profile = base / 'profile'
    settings = profile / '.gdmap'
    settings.mkdir(parents=True)
    (settings / 'settings.ini').write_bytes(b'settings sentinel\n')
    (settings / 'colors.xml').write_bytes(b'palette sentinel\n')
    original = {p.name: p.read_bytes() for p in settings.iterdir()}
    environment = dict(os.environ, HOME=str(profile), G_DEBUG='fatal-warnings')
    environment.pop('DISPLAY', None)
    environment.pop('WAYLAND_DISPLAY', None)

    def run(*args, success=True):
        result = subprocess.run([cli, *map(str, args)], env=environment,
                                capture_output=True, timeout=10)
        assert (result.returncode == 0) == success, (args, result.stderr)
        assert not result.stderr if success else result.stderr
        return result.stdout.decode('ascii')

    snapshot = base / 'tree.gdmap'
    run('scan', source, snapshot)
    source.rename(base / 'renamed')
    overview = run('overview', snapshot)
    rows = [line.split('\t') for line in overview.splitlines() if not line.startswith('#')]
    assert rows[0][2:6] == ['2719', '16', '0', '36.78'], rows[0]
    assert ['group', '700', '2', 'cache'] in rows, rows
    assert any(row[0] == 'file' and row[2:6] == ['1000', '1', '0', '100.00'] and row[-1].endswith('/large') for row in rows)
    assert any(row[0] == 'directory' and row[2:6] == ['1000', '10', '0', '10.00'] and row[-1].endswith('/small') for row in rows)
    assert any(row[-1].endswith('/empty') and row[5] == 'none' for row in rows if row[0] != 'group')
    compact = run('--limit', '1', 'overview', snapshot)
    assert 'group\t700\t2\tcache' in compact
    assert '# remaining-nodes=1 remaining-bytes=300' in compact, compact
    limited = run('--limit', '1', 'expand', snapshot)
    assert '# remaining-nodes=6 remaining-bytes=1719' in limited, limited
    children = [line.split('\t') for line in limited.splitlines() if not line.startswith('#')]
    assert len(children) == 2
    assert int(children[1][2]) + 1719 == 2719
    # Both exact raw paths and the printed numeric address work after renaming.
    for suffix in ['пробел space', 'raw-\udcff\t\n']:
        path = str(source / suffix)
        direct = run('--path', path, 'expand', snapshot)
        node = next(line.split('\t')[1] for line in direct.splitlines() if line.startswith('root\t'))
        assert run('--node', node, 'expand', snapshot) == direct
        assert '/item' in direct
    # A persistent session must return the ordinary rows even after an invalid request.
    ids = ['0', node, '0']
    batch = subprocess.run([cli, '--limit', '2', 'expand', snapshot, '--batch'],
                           input=('invalid\n' + '\n'.join(ids) + '\nquit\n').encode(),
                           capture_output=True, check=True, env=environment, timeout=10)
    responses = batch.stdout.decode('ascii').split('end\n')
    assert responses[0].startswith('ready\nerror ')
    assert responses[1:] == [run('--limit', '2', '--node', i, 'expand', snapshot) for i in ids] + ['']
    run('--node', '0', 'expand', snapshot, '--batch', success=False)
    run('overview', snapshot, '--batch', success=False)
    run('--path', source / 'missing', 'expand', snapshot, success=False)
    run('--node', '99999', 'expand', snapshot, success=False)
    run('--limit', '0', 'overview', snapshot, success=False)
    run('--node', '1', '--path', str(source), 'expand', snapshot, success=False)
    assert {p.name: p.read_bytes() for p in settings.iterdir()} == original
    environment['HOME'] = str(base / 'empty-profile')
    (base / 'empty-profile').mkdir()
    run('overview', snapshot)
    run('expand', snapshot)
    assert not list((base / 'empty-profile').iterdir())
    # The scan root is also an outermost basename occurrence.
    cache_root = base / 'cache'
    file(cache_root / 'a' / 'cache' / 'item', 3)
    file(cache_root / 'b' / 'cache' / 'item', 4)
    root_snapshot = base / 'root-cache.gdmap'
    run('scan', cache_root, root_snapshot)
    assert not any(line.startswith('group\t') and line.endswith('\tcache')
                   for line in run('overview', root_snapshot).splitlines())
    print('PASS: exact file/branch/group totals, nested cache exclusion, counts, percentages, bounded remainder, offline raw/UTF-8 paths, profiles, invalid selectors')
