// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Identity of this PS5 port, shown on the About page. PORT_RELEASE.json is the
// source of truth; tools/test_ps5_about_release.py fails if they disagree.
namespace dkrr::port_release {
inline constexpr const char *tag = "ps5-v1.2.2";
inline constexpr const char *upstream_version = "1.0.15";
inline constexpr const char *repository = "https://github.com/drwellness666/DKR-R-PS5";
inline constexpr const char *release_notes = "https://github.com/drwellness666/DKR-R-PS5/releases/tag/ps5-v1.2.2";
}
