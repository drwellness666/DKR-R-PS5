// SPDX-License-Identifier: GPL-3.0-or-later
// Measures the final launcher font atlas: punctuation and digits merged into
// Racing Banana and the control face must share the letters' cap height and
// optical centre. No ROM, GPU or window is used.
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "generated/selawik_font.h"
#include "generated/racing_banana_font.h"
#include "generated/jumpman_font.h"
// The localisation merge only fills non-ASCII gaps; it is outside this check.
namespace dkr::localisation { inline std::string_view active() { return "en"; } }
void MergeLocalisationFonts(ImFontAtlas*) {}
void LoadBrandLogoIntoAtlas() { ImGui::GetIO().Fonts->TexDesiredWidth = 2048; }
std::uint64_t g_font_generation = 0;
int g_race_button_flat = 0;
ImFont* g_race_button_flat_font = nullptr;
ImFont *g_font_controls = nullptr, *g_font_heading = nullptr, *g_font_title = nullptr, *g_font_fps = nullptr;
#include "runtime_ui_paddock.inl"
#include "launcher_fonts.inl"

static unsigned char* atlas = nullptr;
static int aw = 0, ah = 0;
static int failures = 0;

struct Ink { bool ok; float top, bottom; };
// Rows of the glyph quad whose atlas coverage exceeds 50 %, in pixels above the baseline.
static Ink ink(ImFont* font, char ch) {
    const auto* g = font->FindGlyphNoFallback(static_cast<ImWchar>(static_cast<unsigned char>(ch)));
    if (g == nullptr || !g->Visible) return {false, 0, 0};
    const float base = font->Ascent, height = g->Y1 - g->Y0;
    float top = -1e9F, bottom = 1e9F;
    const int rows = std::max(1, static_cast<int>(std::lround((g->V1 - g->V0) * ah)));
    for (int r = 0; r < rows; ++r) {
        const int ay = static_cast<int>(g->V0 * ah) + r;
        bool on = false;
        for (int ax = static_cast<int>(g->U0 * aw); ax < static_cast<int>(g->U1 * aw); ++ax)
            on |= atlas[(static_cast<std::size_t>(ay) * aw + ax) * 4 + 3] > 127;
        if (!on) continue;
        top = std::max(top, base - (g->Y0 + height * r / rows));
        bottom = std::min(bottom, base - (g->Y0 + height * (r + 1) / rows));
    }
    return {top > -1e8F, top, bottom};
}

static void expect(bool ok, const char* face, char ch, const char* what, float value) {
    if (ok) return;
    std::fprintf(stderr, "FAIL: %s '%c' %s (%.2f)\n", face, ch, what, value);
    ++failures;
}

// Every character is compared with the face's own 'H'.
static void check_face(const char* name, ImFont* font, const char* centred, const char* digits) {
    const Ink cap = ink(font, 'H');
    expect(cap.ok, name, 'H', "missing", 0);
    if (!cap.ok) return;
    const float height = cap.top - cap.bottom;
    for (const char* c = centred; *c; ++c) {
        const Ink i = ink(font, *c);
        expect(i.ok, name, *c, "missing", 0);
        if (!i.ok) continue;
        const float centre = ((i.top + i.bottom) / 2 - cap.bottom) / height;
        expect(centre > 0.42F && centre < 0.58F, name, *c, "optical centre off the cap middle", centre);
    }
    for (const char* c = digits; *c; ++c) {
        const Ink i = ink(font, *c);
        if (!i.ok) { expect(false, name, *c, "missing", 0); continue; }
        const float ratio = (i.top - i.bottom) / height;
        expect(ratio > 0.90F && ratio < 1.06F, name, *c, "digit height differs from cap height", ratio);
        expect(std::fabs(i.bottom - cap.bottom) <= 1.5F, name, *c, "digit baseline differs", i.bottom - cap.bottom);
    }
}

int main() {
    ImGui::CreateContext();
    LoadLauncherFonts();
    auto& io = ImGui::GetIO();
    io.Fonts->GetTexDataAsRGBA32(&atlas, &aw, &ah);
    check_face("body", io.FontDefault, "-/()0", "0123456789");
    for (std::size_t i = 0; i < kPaddockSignSizes.size(); ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "sign %.0f", kPaddockSignSizes[i]);
        check_face(name, g_paddock_sign[i], "-/()0", "0123456789");
    }
    check_face("controls", g_font_controls, "", "0123456789");
    check_face("heading", g_font_heading, "-/()0", "0123456789");
    if (aw > 2048 || ah > 2048) {
        std::fprintf(stderr, "FAIL: launcher font atlas grew to %dx%d\n", aw, ah);
        ++failures;
    }
    ImGui::DestroyContext();
    if (failures != 0) return 1;
    std::printf("PASS launcher typography: punctuation centred and digits at cap height in %zu faces, atlas=%dx%d\n",
                kPaddockSignSizes.size() + 3, aw, ah);
    return 0;
}
