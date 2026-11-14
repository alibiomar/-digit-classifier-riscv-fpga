// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2023.1 (64-bit)
// Tool Version Limit: 2023.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xdigit_mlp.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XDigit_mlp_CfgInitialize(XDigit_mlp *InstancePtr, XDigit_mlp_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XDigit_mlp_Start(XDigit_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_CTRL) & 0x80;
    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XDigit_mlp_IsDone(XDigit_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XDigit_mlp_IsIdle(XDigit_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XDigit_mlp_IsReady(XDigit_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XDigit_mlp_EnableAutoRestart(XDigit_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XDigit_mlp_DisableAutoRestart(XDigit_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_CTRL, 0);
}

u32 XDigit_mlp_Get_return(XDigit_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_AP_RETURN);
    return Data;
}
u32 XDigit_mlp_Get_img_BaseAddress(XDigit_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (InstancePtr->Control_BaseAddress + XDIGIT_MLP_CONTROL_ADDR_IMG_BASE);
}

u32 XDigit_mlp_Get_img_HighAddress(XDigit_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (InstancePtr->Control_BaseAddress + XDIGIT_MLP_CONTROL_ADDR_IMG_HIGH);
}

u32 XDigit_mlp_Get_img_TotalBytes(XDigit_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (XDIGIT_MLP_CONTROL_ADDR_IMG_HIGH - XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + 1);
}

u32 XDigit_mlp_Get_img_BitWidth(XDigit_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDIGIT_MLP_CONTROL_WIDTH_IMG;
}

u32 XDigit_mlp_Get_img_Depth(XDigit_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDIGIT_MLP_CONTROL_DEPTH_IMG;
}

u32 XDigit_mlp_Write_img_Words(XDigit_mlp *InstancePtr, int offset, word_type *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length)*4 > (XDIGIT_MLP_CONTROL_ADDR_IMG_HIGH - XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(int *)(InstancePtr->Control_BaseAddress + XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + (offset + i)*4) = *(data + i);
    }
    return length;
}

u32 XDigit_mlp_Read_img_Words(XDigit_mlp *InstancePtr, int offset, word_type *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length)*4 > (XDIGIT_MLP_CONTROL_ADDR_IMG_HIGH - XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(data + i) = *(int *)(InstancePtr->Control_BaseAddress + XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + (offset + i)*4);
    }
    return length;
}

u32 XDigit_mlp_Write_img_Bytes(XDigit_mlp *InstancePtr, int offset, char *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length) > (XDIGIT_MLP_CONTROL_ADDR_IMG_HIGH - XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(char *)(InstancePtr->Control_BaseAddress + XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + offset + i) = *(data + i);
    }
    return length;
}

u32 XDigit_mlp_Read_img_Bytes(XDigit_mlp *InstancePtr, int offset, char *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length) > (XDIGIT_MLP_CONTROL_ADDR_IMG_HIGH - XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(data + i) = *(char *)(InstancePtr->Control_BaseAddress + XDIGIT_MLP_CONTROL_ADDR_IMG_BASE + offset + i);
    }
    return length;
}

void XDigit_mlp_InterruptGlobalEnable(XDigit_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_GIE, 1);
}

void XDigit_mlp_InterruptGlobalDisable(XDigit_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_GIE, 0);
}

void XDigit_mlp_InterruptEnable(XDigit_mlp *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_IER);
    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_IER, Register | Mask);
}

void XDigit_mlp_InterruptDisable(XDigit_mlp *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_IER);
    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_IER, Register & (~Mask));
}

void XDigit_mlp_InterruptClear(XDigit_mlp *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDigit_mlp_WriteReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_ISR, Mask);
}

u32 XDigit_mlp_InterruptGetEnabled(XDigit_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_IER);
}

u32 XDigit_mlp_InterruptGetStatus(XDigit_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDigit_mlp_ReadReg(InstancePtr->Control_BaseAddress, XDIGIT_MLP_CONTROL_ADDR_ISR);
}

