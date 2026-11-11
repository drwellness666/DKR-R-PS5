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
    assert 'board_min.y + 5.0F}, 8, 6.0F' in ui_cpp,'magic-code checker must stay inset'
    assert 'clipped by the 26 px inner corner' not in online and 'top = a.y + 8.0F' in online
if game.is_dir():
    # Lobby chat must never take a controller button by default: Touchpad opens
    # the DKR-R menu, so the chat button is user-chosen and refuses used buttons.
    chat=game/'netplay/session_chat'
    settings=(chat/'settings.hpp').read_text()
    assert 'shortcut_button{-1}' in settings and 'online_chat_button' in settings
    chat_ui=(chat/'ui.inl').read_text()
    assert 'SessionChatButtonInUse(button)' in chat_ui and 'shortcut_controller_binding' in chat_ui and 'controller_binding(player' in chat_ui
    assert 'SDL_CONTROLLER_BUTTON_BACK:return "Touchpad"' in chat_ui
    assert 'is free\n    // in gameplay' not in (game/'runtime_ui.cpp').read_text(),'chat must not be hard-wired to Touchpad'
    online_ui=(game/'runtime_online_ui.inl').read_text()
    assert 'Touchpad opens lobby chat' not in online_ui
main=(port/'src/ps5_game_main.cpp').read_text()
assert 'setenv("TMPDIR",scratch,1)' in main
print('PASS PS5 About identity, storage controls, update phases, presets and private temp directory')
