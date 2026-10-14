/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The SDK's payload layout remains in force. Include it before this fragment. */
PROVIDE(__eh_frame_start = ADDR(.eh_frame));
PROVIDE(__eh_frame_end = ADDR(.eh_frame) + SIZEOF(.eh_frame));
PROVIDE(__eh_frame_hdr_start = ADDR(.eh_frame_hdr));
PROVIDE(__eh_frame_hdr_end = ADDR(.eh_frame_hdr) + SIZEOF(.eh_frame_hdr));
