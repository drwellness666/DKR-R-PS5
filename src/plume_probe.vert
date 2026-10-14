#version 450
// SPDX-License-Identifier: GPL-3.0-or-later
void main() {
    const vec2 p[3] = vec2[](vec2(-0.7, -0.7), vec2(0.7, -0.7), vec2(0.0, 0.7));
    gl_Position = vec4(p[gl_VertexIndex], 0.0, 1.0);
}
