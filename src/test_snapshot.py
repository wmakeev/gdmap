#!/usr/bin/env python3
"""Exercise the CLI in fresh processes and keep all scan fixtures in the project."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

cli = str(Path(sys.argv[1]).resolve())
project = Path(sys.argv[2])

with tempfile.TemporaryDirectory(prefix='.snapshot-test-', dir=project) as temporary:
    base = Path(temporary)
    source = base / 'source'
    source.mkdir()
    (source / 'equal-a').write_bytes(b'a' * 37)
    (source / 'equal-b').write_bytes(b'b' * 37)
    (source / 'empty').mkdir()
    (source / 'link').symlink_to('equal-a')
    os.link(source / 'equal-a', source / 'hard')
    os.mkfifo(source / 'fifo')
    (source / 'nested').mkdir()
    (source / 'nested' / 'tiny').write_bytes(b'abc')
    raw_path = os.fsencode(source) + b'/raw-\xff\t\n'
    with open(raw_path, 'wb') as stream:
        stream.write(b'raw')
    with open(source / 'sparse', 'wb') as stream:
        stream.seek(1024 * 1024)
        stream.write(b'x')
    equal_order = [entry.name for entry in os.scandir(source)
                   if entry.name in ('equal-a', 'equal-b', 'hard')][::-1]
    profile = base / 'profile'
    settings = profile / '.gdmap'
    settings.mkdir(parents=True)
    (settings / 'settings.ini').write_bytes(b'[Scan]\nUseReportedSize=false\nLeaveDevice=true\n')
    (settings / 'colors.xml').write_bytes(b'<?xml version="1.0"?><sentinel/>\n')
    original_settings = {p.name: p.read_bytes() for p in settings.iterdir()}
    environment = dict(os.environ, HOME=str(profile), G_DEBUG='fatal-warnings')
    environment.pop('DISPLAY', None)
    environment.pop('WAYLAND_DISPLAY', None)

    def run(*args, success=True):
        result = subprocess.run([cli, *map(str, args)], env=environment,
                                capture_output=True, timeout=10)
        assert (result.returncode == 0) == success, (args, result.returncode, result.stderr)
        assert not result.stderr if success else result.stderr
        return result.stdout

    snapshot = base / 'tree.gdmap'
    before = run('scan', source, snapshot)
    rows = [row.split('\t') for row in before.decode('ascii').splitlines() if not row.startswith('#')]
    by_name = {row[4].split('/')[-1]: row for row in rows}
    assert by_name['link'][2:4] == ['symlink', '7']
    assert by_name['empty'][2:4] == ['directory', '0']
    assert by_name['nested'][2:4] == ['directory', '3']
    assert by_name['sparse'][3] == str(1024 * 1024 + 1)
    assert 'fifo' not in by_name
    assert 'raw-\\377\\t\\n' in by_name
    observed_equal_order = [row[4].split('/')[-1] for row in rows if row[3] == '37']
    assert observed_equal_order == equal_order, (observed_equal_order, equal_order)
    assert int(rows[0][3]) == 3 * 37 + 7 + 3 + 3 + 1024 * 1024 + 1
    for row in rows:
        assert row[4].startswith(str(source.resolve()))
    allocated_snapshot = base / 'allocated.gdmap'
    allocated = run('--allocated-size', '--leave-device', 'scan', source, allocated_snapshot)
    assert b'leave-device=true use-reported-size=false' in allocated
    sparse_stat = (source / 'sparse').stat()
    allocated_bytes = sparse_stat.st_blocks * 512
    expected_size = allocated_bytes if 0 < allocated_bytes < sparse_stat.st_size else sparse_stat.st_size
    assert any(row.split(b'\t')[3] == str(expected_size).encode()
               for row in allocated.splitlines() if row.endswith(b'/sparse'))
    source.rename(base / 'renamed')
    assert run('inspect', snapshot) == before
    assert run('inspect', allocated_snapshot) == allocated
    assert {p.name: p.read_bytes() for p in settings.iterdir()} == original_settings
    run('scan', source, base / 'missing.gdmap', success=False)
    assert not (base / 'missing.gdmap').exists()
    corrupt = base / 'corrupt.gdmap'
    contents = snapshot.read_bytes()
    unsupported = bytes([2]) + contents[1:]
    invalid_parent = contents[:8] + bytes([1]) + contents[9:]
    for value in (b'', b'not a snapshot', contents[:10], contents[:-1], b'\xff' * len(contents), unsupported, invalid_parent):
        corrupt.write_bytes(value)
        run('inspect', corrupt, success=False)
    environment['HOME'] = str(base / 'empty-profile')
    (base / 'empty-profile').mkdir()
    run('scan', base / 'renamed', base / 'empty-profile.gdmap')
    run('inspect', snapshot)
    assert not list((base / 'empty-profile').iterdir())
    print('PASS: offline roundtrip, types, bytes, sums, equal order, raw names, policies, profiles, root failure, corrupt files')
