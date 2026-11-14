#include "digit_mlp.h"
#include "weights.h"

int digit_mlp(const unsigned char img[N_IN]) {
#pragma HLS INTERFACE s_axilite port=img
#pragma HLS INTERFACE s_axilite port=return

// W1[j*N_IN + i]: block factor = N_HID gives one bank per hidden neuron j.
#pragma HLS ARRAY_PARTITION variable=W1 type=block factor=32 dim=1
// W2[k*N_HID + j]: cyclic factor = N_HID gives one bank per hidden index j.
#pragma HLS ARRAY_PARTITION variable=W2 type=cyclic factor=32 dim=1
#pragma HLS ARRAY_PARTITION variable=B1 type=complete dim=1

    // ---- layer 1: 784 cycles, all 32 neurons in parallel ----
    int acc[N_HID];
#pragma HLS ARRAY_PARTITION variable=acc type=complete dim=1
    for (int j = 0; j < N_HID; j++) {
#pragma HLS UNROLL
        acc[j] = B1[j];
    }

    for (int i = 0; i < N_IN; i++) {
#pragma HLS PIPELINE II=1
        int x = (int)img[i];
        for (int j = 0; j < N_HID; j++) {
#pragma HLS UNROLL
            acc[j] += x * (int)W1[j * N_IN + i];
        }
    }

    signed char h8[N_HID];
#pragma HLS ARRAY_PARTITION variable=h8 type=complete dim=1
    for (int j = 0; j < N_HID; j++) {
#pragma HLS UNROLL
        int a = acc[j];
        if (a < 0) a = 0;
        a >>= SHIFT;
        if (a > 127) a = 127;
        h8[j] = (signed char)a;
    }

    // ---- layer 2: one output per cycle, 32 MACs in parallel ----
    int best = 0;
    int best_v = -2147483647;
    for (int k = 0; k < N_OUT; k++) {
#pragma HLS PIPELINE II=1
        int a = B2[k];
        for (int j = 0; j < N_HID; j++) {
#pragma HLS UNROLL
            a += (int)h8[j] * (int)W2[k * N_HID + j];
        }
        if (a > best_v) { best_v = a; best = k; }
    }
    return best;
}
