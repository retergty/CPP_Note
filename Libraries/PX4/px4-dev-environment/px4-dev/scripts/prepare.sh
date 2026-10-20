#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
command -v xauth >/dev/null || { echo 'Install host xauth: sudo apt install xauth'; exit 1; }
: "${DISPLAY:?Run this in your logged-in desktop terminal}"
umask 077
python3 - "$(id -u)" "$(id -g)" "$DISPLAY" <<'PYCONFIG'
import pathlib,re,sys
root=pathlib.Path.cwd()
configs=[]
names=set()
workspaces=set()
for envfile in sorted((root/'env').glob('*.env')):
    values={}
    for line in envfile.read_text().splitlines():
        line=line.strip()
        if not line or line.startswith('#'): continue
        key,sep,value=line.partition('=')
        if not sep: raise SystemExit(f'Invalid line in {envfile.name}')
        values[key.strip()]=value.strip()
    for key in ('ENV_NAME','WORKSPACE_DIR','SOURCE_FOLDER','ROS_DOMAIN_ID'):
        if not values.get(key): raise SystemExit(f'Missing {key} in {envfile.name}')
    name=values['ENV_NAME']
    if not re.fullmatch(r'[a-z0-9][a-z0-9_-]*',name) or name in names:
        raise SystemExit(f'Invalid or duplicate ENV_NAME in {envfile.name}')
    names.add(name)
    for key in ('WORKSPACE_DIR','SOURCE_FOLDER'):
        value=values[key]
        path=pathlib.PurePosixPath(value)
        if path.is_absolute() or '..' in path.parts or not path.parts or not re.fullmatch(r'[A-Za-z0-9_./-]+',value):
            raise SystemExit(f'{key} must be a simple relative path in {envfile.name}')
    workspace=root/values['WORKSPACE_DIR']
    if workspace.resolve() in workspaces: raise SystemExit('Environments must use different workspace directories')
    workspaces.add(workspace.resolve())
    source=workspace/values['SOURCE_FOLDER']
    if not source.resolve().is_relative_to(root): raise SystemExit('Source must be inside PX4 root')
    for filename in ('ubuntu.sh','requirements.txt'):
        if not (source/'Tools/setup'/filename).is_file():
            raise SystemExit(f'Missing existing installer: {source}/Tools/setup/{filename}')
    try: domain=int(values['ROS_DOMAIN_ID'])
    except ValueError: raise SystemExit(f'Invalid ROS_DOMAIN_ID in {envfile.name}')
    if not 0<=domain<=232: raise SystemExit(f'Invalid ROS_DOMAIN_ID in {envfile.name}')
    configs.append((workspace,source))
if not configs: raise SystemExit('No environment files found in env/')
ignore=['**','!Dockerfile']
for workspace,source in configs:
    for state in ('codex','cc-switch','cache','gz','vscode-server'):
        (workspace/'container-state'/state).mkdir(parents=True,exist_ok=True)
    (workspace/'ros2_ws/src').mkdir(parents=True,exist_ok=True)
    config=workspace/'container-state/codex/config.toml'
    if not config.exists(): config.touch()
    relative=(source/'Tools/setup').relative_to(root)
    for parent in reversed(relative.parents):
        if str(parent)!='.': ignore.append('!'+parent.as_posix()+'/')
    ignore += ['!'+relative.as_posix()+'/','!'+relative.as_posix()+'/ubuntu.sh','!'+relative.as_posix()+'/requirements.txt']
(root/'.dockerignore').write_text('\n'.join(dict.fromkeys(ignore))+'\n')
for directory in ('state','shared'): (root/directory).mkdir(exist_ok=True)
p=root/'.env'
lines=p.read_text().splitlines() if p.exists() else []
keys=('DEV_UID=','DEV_GID=','HOST_DISPLAY=','PX4_REF=','PX4_MSGS_REF=','PX4_SOURCE_REL=','ENV_NAME=','WORKSPACE_DIR=','SOURCE_FOLDER=','ROS_DOMAIN_ID=')
lines=[line for line in lines if not line.startswith(keys)]
lines += ['DEV_UID='+sys.argv[1],'DEV_GID='+sys.argv[2],'HOST_DISPLAY='+sys.argv[3]]
p.write_text('\n'.join(lines)+'\n')
PYCONFIG
cookie=$(xauth nlist "$DISPLAY")
[[ -n "$cookie" ]] || { echo 'No X11 cookie found; check XAUTHORITY in this desktop session.'; exit 1; }
# FamilyWild avoids the host/container hostname mismatch. Authorize only this cookie.
tmp_auth=$(mktemp "$PWD/state/Xauthority.XXXXXX")
trap 'rm -f -- "$tmp_auth"' EXIT
printf '%s\n' "$cookie" | sed 's/^..../ffff/' | xauth -f "$tmp_auth" nmerge -
# Preserve the inode if a running container already bind-mounted this file.
cat "$tmp_auth" > state/Xauthority
chmod 600 state/Xauthority .env
echo 'Prepared environments from env/*.env, .env, .dockerignore and Xauthority.'
