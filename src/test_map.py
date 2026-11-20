#!/usr/bin/env python3
"""Offline render/hit contracts, independent of the live filesystem and display."""
import configparser
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

cli = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='.map-test-', dir=sys.argv[2]) as temporary:
    base = Path(temporary)
    source = base / 'source'
    (source / 'nested' / 'deep').mkdir(parents=True)
    (source / 'a.png').write_bytes(b'a' * 100)
    (source / 'nested' / 'b.c').write_bytes(b'b' * 50)
    for i in range(49):
        (source / 'nested' / 'deep' / f'c{i}.txt').write_bytes(b'c')
    raw = source / 'nested' / 'deep' / os.fsdecode(b'raw-\xff.txt')
    raw.write_bytes(b'c')
    (source / 'empty').mkdir()
    (source / 'zero').touch()
    profile = base / 'profile'
    settings = profile / '.gdmap'
    settings.mkdir(parents=True)
    (settings / 'settings.ini').write_bytes(b'sentinel settings\n')
    (settings / 'colors.xml').write_bytes(b'sentinel colors\n')
    original = {p.name: p.read_bytes() for p in settings.iterdir()}
    env = dict(os.environ, HOME=str(profile), G_DEBUG='fatal-warnings')
    env.pop('DISPLAY', None)
    env.pop('WAYLAND_DISPLAY', None)

    def run(*args, success=True):
        result = subprocess.run([cli, *map(str, args)], env=env, capture_output=True, timeout=15)
        assert (result.returncode == 0) == success, (args, result.returncode, result.stderr)
        assert not result.stderr if success else result.stderr
        return result.stdout.decode()

    snapshot = base / 'tree.gdmap'
    records = [s.split('\t') for s in run('scan', source, snapshot).splitlines() if not s.startswith('#')]
    first = records[1][4]
    nested_left = first.endswith('/nested')
    source.rename(base / 'moved')
    for mode in ['square', 'standard']:
        png = base / f'{mode}.png'
        run('render', snapshot, png, '--width', 202, '--height', 102, '--mode', mode)
        assert struct.unpack('>II', png.read_bytes()[16:24]) == (202, 102)
        frame = Path(str(png) + '.frame')
        metadata = configparser.ConfigParser(interpolation=None)
        metadata.read(frame)
        assert metadata['legend'] and frame.stat().st_size < 3000
        def hit(x, y, *extra):
            return run('hit', frame, '--x', x, '--y', y, *extra)
        for px in [0, 201]:
            assert 'miss' in hit(px, 50)
        assert 'miss' in hit(50, 0) and 'miss' in hit(50, 101)
        assert 'miss' in hit(202, 50)
        for px in [51, 100, 101, 151]:
            left = px <= 100
            output = hit(px, 25)
            assert ('/nested/' in output.splitlines()[1]) == (left == nested_left), output
            scaled = hit(px/2, 25/2, '--preview-width', 101, '--preview-height', 51)
            assert output == scaled
        selected_x = 50 if nested_left else 150
        assert '\t' + str(source) + '/nested\t' in hit(selected_x, 25, '--parent')
        root_png = base / f'{mode}-depth.png'
        run('render', snapshot, root_png, '--width', 202, '--height', 102, '--depth', 1, '--mode', mode)
        assert run('hit', str(root_png)+'.frame', '--x', selected_x, '--y', 25).splitlines()[1].split('\t')[3].endswith('/nested')
    process = subprocess.Popen([cli, 'hit', str(frame), '--batch'], env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    assert process.stdout.readline() == 'ready 202 102\n'
    for point in ['51 25', 'parent 51 25', '0 50', 'invalid']:
        process.stdin.write(point+'\n')
        process.stdin.flush()
        lines = []
        for line in process.stdout:
            if line == 'end\n':
                break
            lines.append(line)
        assert lines and ('error' in lines[0] if point=='invalid' else 'original-coordinate' in lines[0])
    process.stdin.write('quit\n')
    process.stdin.flush()
    process.wait(timeout=5)
    assert process.returncode == 0 and not process.stderr.read()
    for lighting in [0, 0.35, 1]:
        png = base / f'deep-{lighting}.png'
        run('render', snapshot, png, '--path', str(source/'nested'/'deep'), '--lighting', lighting)
        assert run('hit', str(png)+'.frame', '--x', 600, '--y', 400).splitlines()[1].split('\t')[3].endswith('.txt')
    raw_id = next(row[0] for row in records if 'raw-' in row[4])
    run('render', snapshot, base/'raw.png', '--path', raw)
    raw_hit = run('hit', str(base/'raw.png')+'.frame', '--x', 600, '--y', 400)
    assert 'node='+raw_id in raw_hit
    run('render', snapshot, base/'raw-id.png', '--node', raw_id)
    palette = base/'palette.xml'
    palette.write_text('<Colors><Color type="Default" r="65535" g="0" b="0"/></Colors>')
    run('render', snapshot, base/'palette.png', '--palette', palette)
    assert '#ff0000' in Path(str(base/'palette.png')+'.frame').read_text()
    assert (base/'deep-0.png').read_bytes() != (base/'deep-1.png').read_bytes()
    run('render', snapshot, base/'zero.png', '--path', source/'empty')
    assert '\tdirectory\t0\t' in run('hit', str(base/'zero.png')+'.frame', '--x', 600, '--y', 400)
    run('render', snapshot, base/'tiny.png', '--width', 3, '--height', 3)
    for args in [('--width',0), ('--height',-1), ('--depth',-1), ('--lighting','nan'), ('--mode','bad'), ('--path', source/'missing')]:
        run('render', snapshot, base/'bad.png', *args, success=False)
    assert {p.name: p.read_bytes() for p in settings.iterdir()} == original
    empty = base/'empty-profile'
    empty.mkdir()
    env['HOME'] = str(empty)
    run('scan', base/'moved', base/'empty.gdmap')
    run('render', snapshot, base/'blank.png')
    run('hit', str(base/'blank.png')+'.frame', '--x', 30, '--y', 30)
    assert not list(empty.iterdir())
    snapshot.write_bytes(snapshot.read_bytes() + b'changed')
    run('hit', str(base/'blank.png')+'.frame', '--x', 30, '--y', 30, success=False)
    print('PASS: offline PNG sizes, centers, boundaries, border, preview scaling, parent, subtrees, depths, zero sizes, lighting, profiles, snapshot binding')
