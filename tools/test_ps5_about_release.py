#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The About page's PS5 port identity must match the release record, and the
1.2.1 menu fixes must stay wired into the final PS5 sources."""
import json,pathlib,re,sys
port=pathlib.Path(__file__).resolve().parents[1]
work=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else port.parent
record=json.loads((port/'PORT_RELEASE.json').read_text())
header=(port/'src/ps5_port_release.hpp').read_text()
def constant(name):
    match=re.search(r'\b'+name+r'\s*=\s*"([^"]+)"',header)
    assert match,name
    return match.group(1)
assert constant('tag')==record['tag'],(constant('tag'),record['tag'])
assert constant('upstream_version')==record['upstream_version']
assert constant('release_notes').endswith('/tag/'+record['tag'])
assert constant('repository')=='https://github.com/drwellness666/DKR-R-PS5'

game=work/'native-rebase/runtime-recomp/src/game'
if game.is_dir():  # the public export carries only the PS5 sources
    details=(game/'runtime_details_ui.inl').read_text()
    # Storage and crash-report choices must be reachable on the PS5 About page.
    ps5_branch=details[details.index('#else\n    (void)support;'):]
    assert 'DrawCrashReportingSettings(inner)' in ps5_branch[:2500],'storage controls missing on PS5'
    assert 'dkrr::port_release::tag' in ps5_branch and 'VISIT PS5 PORT PAGE' in ps5_branch
    assert 'Check for updates on the Updates page' in details
    updates=(game/'runtime_updates_ui.inl').read_text()
    assert 'g_page_navigation_request = kPageUpdates' in updates and 'Update available' in updates
    ui=(port/'src/ps5_update_ui.inl').read_text()
    for phase in ('STEP 1 OF 4','STEP 2 OF 4','STEP 3 OF 4','STEP 4 OF 4'): assert phase in ui,phase
    graphics=(game/'runtime_graphics_ui.inl').read_text()
    assert '{{"Performance"}, {"Quality"}}' in graphics and 'output.resolution = quality ? 2160U : 1080U' in graphics
    assert 'RefreshRate::Display' in graphics
wording=(port/'tools/stage_ps5_ui_wording.py').read_text()
assert 'p.y+10' in wording and 'p.y+37' not in wording,'text-entry pencil must stay centred'
if game.is_dir():
    online=(game/'runtime_online_ui.inl').read_text()
    ui_cpp=(game/'runtime_ui.cpp').read_text()
    assert 'board_min.y + 5.0F}, {x + 6.0F, board_min.y + 11.0F}' in ui_cpp,'magic-code squares must stay inset and shadow-free'
    assert 'MagicCodeLabelType(' in ui_cpp,'magic-code names must fit on one line'
    updates=game/'updates'
    lock=(updates/'update_process.cpp').read_text()
    assert 'permission_error' in lock and 'flock unsupported' in lock,'install lock must tolerate the console volume'
    assert 'ps5][durable-write] fsync ignored' in (updates/'update_core.cpp').read_text()
    assert 'could not save the choice' in (game/'crash_reports/service.cpp').read_text()
    chat_text=(game/'netplay/session_chat/ui.inl').read_text()
    assert chat_text.count('Chat key')==1 and 'defined(__PROSPERO__)' in chat_text,'one chat control per platform'
    assert 'clipped by the 26 px inner corner' not in online and 'top = a.y + 8.0F' in online
if game.is_dir():
    # Lobby chat must never take a controller button by default: Touchpad opens
    # the DKR-R menu, so the chat button is user-chosen and refuses used buttons.
    chat=game/'netplay/session_chat'
    settings=(chat/'settings.hpp').read_text()
    assert 'shortcut_button{-1}' in settings and 'online_chat_button' in settings
    chat_ui=(chat/'ui.inl').read_text()
    assert 'SessionChatButtonConflict(button)' in chat_ui and 'shortcut_controller_binding' in chat_ui and 'controller_binding(player' in chat_ui
    assert 'SDL_CONTROLLER_BUTTON_BACK:return "Touchpad"' in chat_ui
    assert 'is free\n    // in gameplay' not in (game/'runtime_ui.cpp').read_text(),'chat must not be hard-wired to Touchpad'
    online_ui=(game/'runtime_online_ui.inl').read_text()
    assert 'Touchpad opens lobby chat' not in online_ui
main=(port/'src/ps5_game_main.cpp').read_text()
assert 'setenv("TMPDIR",scratch,1)' in main and '/data/dkrr/profile-us-v77/tmp' in main and 'mkdir("/data/dkrr/tmp"' not in main,'temp folder must be inside a profile the title can write'
if game.is_dir():
    service=(game/'storage/service.cpp').read_text()
    assert 'automatic-pass.running' in service and 'constexpr bool background_maintenance = true;' in service,'the automatic pass needs its crash-loop marker'
    assert 'storage-trace.log' in service and 'trace(profile, "maintain")' in service,'manual storage passes must leave a synchronous trace'
if game.is_dir():
    chat_ui_text=(game/'netplay/session_chat/ui.inl').read_text()
    assert 'g_overlay_visible.load()||now_ms' not in chat_ui_text,'chat capture must not depend on the race overlay flag'
    controls=(game/'runtime_controls_ui.inl').read_text()
    assert controls.count('ControlsRule(width);')>=3,'Controls/PS5 groups need separator lines'
    for name in ('storage/maintenance.cpp','updates/update_core.cpp'):
        assert 'std::array<char, 65536>' not in (game/name).read_text(),name+': 64 KiB digest buffer belongs on the heap'
    assert 'trace(profile, "install-directory")' in service and '/app0' in service,'PS5 storage must not look up /proc/self/exe'
    assert 'HandleSessionChatButtonCapture(&event)' in (game/'runtime_ui.cpp').read_text(),'the launcher loop must feed chat button capture'
    assert 'sounds_open,width,false)' in controls,'no rule above the controller sound choice'
    link=(port/'tools/link_native_game.sh').read_text()
    assert 'missing_libc.o' in link and 'readlink; symlink; link; fchown; mkstemp; strcasestr' in link,'the stand-ins for unexported libc functions must be linked'
print('PASS PS5 About identity, storage controls, update phases, presets and private temp directory')
