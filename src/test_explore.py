#!/usr/bin/env python3
"""Follow a numeric selection onto a map and back after the snapshot moves."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

cli = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='gdmap-explore-test-') as temporary:
    work = Path(temporary)
    profile = work / 'home'
    profile.mkdir()
    env = dict(os.environ, HOME=str(profile), TMPDIR=str(work), G_DEBUG='fatal-warnings')
    env.pop('DISPLAY', None)
    env.pop('WAYLAND_DISPLAY', None)

    def run(*args):
        return subprocess.check_output([cli, *args], env=env, text=True)

    source = work / 'data'
    branch = source / 'nested'
    branch.mkdir(parents=True)
    (branch / 'data.bin').write_bytes(b'x' * 80)
    (source / 'a.txt').write_bytes(b'x' * 10)
    snapshot = work / 'scan.gdmap'
    records = run('scan', source, snapshot).splitlines()
    ids = {line.split('\t')[-1]: line.split('\t')[0] for line in records if not line.startswith('#')}
    folder = ids[str(branch)]
    leaf = ids[str(branch / 'data.bin')]
    overview = run('overview', snapshot)
    expansion = run('expand', snapshot, '--node', folder)
    png = work / 'reference.png'
    palette = work / 'palette.xml'
    palette.write_text('<Colors><Color type="Default" r="65535" g="0" b="0"/></Colors>')
    options = ['--width', '202', '--height', '102', '--lighting', '0.55', '--palette', palette]
    run('render', snapshot, png, '--node', folder, *options)
    expected_hit = run('hit', str(png) + '.frame', '--x', '100', '--y', '50')

    with subprocess.Popen([cli, 'explore', snapshot, *options], stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE, env=env, text=True) as proc:
        def receive():
            result = []
            for line in proc.stdout:
                if line == 'end\n':
                    return ''.join(result)
                result.append(line)
            raise AssertionError('session exited before end of response')

        def send(command):
            proc.stdin.write(command + '\n')
            proc.stdin.flush()
            return receive()

        initial = receive()
        assert initial.startswith('ready\n') and initial.endswith(overview)
        snapshot.rename(work / 'moved.gdmap')
        source.rename(work / 'moved-source')
        assert send('hit 1 1').startswith('error ')
        assert send('expand ' + folder) == expansion
        frame = send('map ' + folder)
        preview = Path(next(line[6:] for line in frame.splitlines() if line.startswith('image ')))
        assert preview.read_bytes() == png.read_bytes()
        assert '#ff0000' in frame and 'PNG 202 102' in frame
        assert 'node=0' in frame
        assert send('hit 100 50') == expected_hit and 'node=' + leaf in expected_hit
        assert send('expand ' + leaf).splitlines()[1].split('\t')[2] == '80'
        assert send('map 999999').startswith('error ')
        assert send('hit nan 3').startswith('error ')
        assert send('X' * 512).startswith('error ')
        assert send('hit 100 50') == expected_hit
        assert 'miss' in send('hit 0 0')
        assert send('map ' + folder) == frame
        assert send('overview') == overview
        root_frame = send('map 0')
        assert not preview.exists()
        current = Path(next(line[6:] for line in root_frame.splitlines() if line.startswith('image ')))
        assert current.is_file()
        proc.stdin.close()
        assert proc.wait(timeout=10) == 0 and not current.parent.exists()
    assert not list(profile.iterdir())
