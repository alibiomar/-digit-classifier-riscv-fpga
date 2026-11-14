// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2023.1 (64-bit)
// Tool Version Limit: 2023.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
// control
// 0x000 : Control signals
//         bit 0  - ap_start (Read/Write/COH)
//         bit 1  - ap_done (Read/COR)
//         bit 2  - ap_idle (Read)
//         bit 3  - ap_ready (Read/COR)
//         bit 7  - auto_restart (Read/Write)
//         bit 9  - interrupt (Read)
//         others - reserved
// 0x004 : Global Interrupt Enable Register
//         bit 0  - Global Interrupt Enable (Read/Write)
//         others - reserved
// 0x008 : IP Interrupt Enable Register (Read/Write)
//         bit 0 - enable ap_done interrupt (Read/Write)
//         bit 1 - enable ap_ready interrupt (Read/Write)
//         others - reserved
// 0x00c : IP Interrupt Status Register (Read/TOW)
//         bit 0 - ap_done (Read/TOW)
//         bit 1 - ap_ready (Read/TOW)
//         others - reserved
// 0x010 : Data signal of ap_return
//         bit 31~0 - ap_return[31:0] (Read)
// 0x400 ~
// 0x7ff : Memory 'img' (784 * 8b)
//         Word n : bit [ 7: 0] - img[4n]
//                  bit [15: 8] - img[4n+1]
//                  bit [23:16] - img[4n+2]
//                  bit [31:24] - img[4n+3]
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XDIGIT_MLP_CONTROL_ADDR_AP_CTRL   0x000
#define XDIGIT_MLP_CONTROL_ADDR_GIE       0x004
#define XDIGIT_MLP_CONTROL_ADDR_IER       0x008
#define XDIGIT_MLP_CONTROL_ADDR_ISR       0x00c
#define XDIGIT_MLP_CONTROL_ADDR_AP_RETURN 0x010
#define XDIGIT_MLP_CONTROL_BITS_AP_RETURN 32
#define XDIGIT_MLP_CONTROL_ADDR_IMG_BASE  0x400
#define XDIGIT_MLP_CONTROL_ADDR_IMG_HIGH  0x7ff
#define XDIGIT_MLP_CONTROL_WIDTH_IMG      8
#define XDIGIT_MLP_CONTROL_DEPTH_IMG      784

