#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Console-only wording and text-entry affordances in the actual UI domain."""
import re
TOKENS=re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',re.S)
WORDS={
 'Keyboard':'Enter text', 'On-screen keyboard':'Enter text',
 'This PC':'This PS5', 'THIS PC':'THIS PS5',
 'Keep me signed in on this PC':'Keep me signed in on this PS5',
 'This PC remembers your sign-in.':'This PS5 remembers your sign-in.',
 'Your desktop keyring remembers this sign-in.':'Your sign-in is saved on this PS5.',
 'Session-only sign-in. A desktop keyring is unavailable.':'Signed in until DKR-R closes.',
 'Folders first, then .z64, .v64 and .n64 files. Nothing leaves this PC.':'Folders first, then .z64, .v64 and .n64 files. ROMs stay on your PS5.',
 'Exit DKR-R and return to the desktop?':'Close DKR-R?',
 'F1 / Esc':'Touchpad / Circle',
 'Image previews are available in the launcher.':'Open the mod library to preview images.',
 'Return to the launcher to update.':'Return to the main DKR-R menu to update.',
 'Save editing is unavailable while the game is running. Return to the launcher first.':'Close the active race to edit saves in the main DKR-R menu.',
 'Return to the launcher before joining another lobby.':'Return to the main DKR-R menu before joining another lobby.',
 'Finish the current game and return to the launcher to host or join a lobby.':'Open the online lobby from the main DKR-R menu to host or join.',
 'Return to the launcher and leave the lobby to change mods.':'Return to the main DKR-R menu and leave the lobby to change mods.',
 'Return to the launcher and leave the lobby to change tracks. ':'Return to the main DKR-R menu and leave the lobby to change tracks. ',
 'Joining was cancelled. You are still in the launcher.':'Joining was cancelled. You are still in the DKR-R menu.',
 'Returning safely to the launcher...':'Returning to the DKR-R menu...',
}
def compose(name,text):
    if not (name.endswith('_ui.inl') or name in ('runtime_ui.cpp','runtime_ui_paddock.inl','store_account_page.inl','directory_ui.inl','invite_ui.inl','connection_ui.inl','report_ui.inl','store_field.inl')):return text
    def token(m):
        s=m[0]
        if not s.startswith('"'):return s
        plain=s[1:-1]
        replacement=WORDS.get(plain)
        if replacement is None:
            replacement=plain.replace('Races connect PC to PC.','Races connect directly between players.').replace('or PC performance score.','or console performance score.')
        if name=='runtime_controls_ui.inl' and plain=='Keyboard':return s # Hidden keyboard columns keep their internal contract.
        return '"'+replacement+'"'
    text=TOKENS.sub(token,text)
    if name=='runtime_ui_paddock.inl':
        at=text.index('inline bool PaddockKeyboardButton() {');end=text.index('\n}\n',at)+2
        text=text[:at]+'''inline bool PaddockKeyboardButton() {
    const auto press=PaddockBeginPress("##text-entry",{44,44});
    auto* draw=ImGui::GetWindowDrawList();
    PaddockFill(draw,press.min,press.max,PaddockRound(8),PaddockCol(press.hovered?0x234E65:0x173A4E));
    PaddockStroke(draw,press.min,press.max,PaddockRound(8),PaddockCol(0x476373),1);
    // A vector pencil stays sharp at every TV output scale.
    const auto p=press.min;const auto ink=PaddockCol(0xFFF0C2);
    // The glyph spans x 10.5..33.5, y 10..34: centred in the 44 px button.
    draw->AddQuadFilled({p.x+12.5F,p.y+25},{p.x+27.5F,p.y+10},{p.x+32.5F,p.y+15},{p.x+17.5F,p.y+30},ink);
    draw->AddTriangleFilled({p.x+10.5F,p.y+32},{p.x+12.5F,p.y+25},{p.x+17.5F,p.y+30},ink);
    draw->AddLine({p.x+12.5F,p.y+34},{p.x+33.5F,p.y+34},ink,1.5F);
    PaddockEndPress(press,8,1);
    if(press.hovered||press.focused){const auto item=GImGui->LastItemData;ImGui::SetTooltip("Enter text with the controller");GImGui->LastItemData=item;}
    return press.pressed;
}'''+text[end:]
        old='activated&&GImGui->ActiveIdSource==ImGuiInputSource_Gamepad'
        if text.count(old)!=1:raise ValueError('Changed shared text-entry activation')
        text=text.replace(old,'activated&&(GImGui->ActiveIdSource==ImGuiInputSource_Gamepad||GImGui->ActiveIdSource==ImGuiInputSource_Mouse)')
    if name=='mod_store_ui.inl':
        old='const bool controller_keyboard = ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown) &&'
        if text.count(old)==1:
            text=text.replace(old,'const bool controller_keyboard = ((ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown)) || ImGui::IsItemActivated()) &&')
        elif '/library_ui/store_field.inl"' not in text:raise ValueError('Changed Store text-entry activation')
    if name=='store_field.inl':
        old='const bool controller=ImGui::IsItemFocused()&&ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown)&&'
        if text.count(old)!=1:raise ValueError('Changed shared Library text-entry activation')
        text=text.replace(old,'const bool controller=((ImGui::IsItemFocused()&&ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown))||ImGui::IsItemActivated())&&')
    if name=='sdk2_tools_ui.inl':
        old='''            const std::string shortcut=g_sdk2_key_capture?"Press a key (Esc cancels)":std::string("Keyboard: ")+SDL_GetScancodeName(SDL_Scancode(g_sdk2_tools_key));
            if(ImGui::Button(shortcut.c_str(),{280,40}))g_sdk2_key_capture=true;'''
        # This optional developer page must not offer a physical keyboard bind.
        if old not in text:raise ValueError('Changed SDK tools shortcuts')
        text=text.replace(old,'            g_sdk2_key_capture=false;')
        text=text.replace('Back/Select + Y controller shortcut','Touchpad + Triangle shortcut')
    return text
