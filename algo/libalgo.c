#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <uthash.h>

#include "params.h"
#include "macros.h"

static const double SNR_TARGET_PER_SF[6] = {
    -7.5  + SNR_INCREASE_THRESHOLD,   // SF7
    -10.0 + SNR_INCREASE_THRESHOLD,  // SF8
    -12.5 + SNR_INCREASE_THRESHOLD,  // SF9
    -15.0 + SNR_INCREASE_THRESHOLD,  // SF10
    -17.5 + SNR_INCREASE_THRESHOLD,  // SF11
    -20.0 + SNR_INCREASE_THRESHOLD   // SF12
};

typedef struct {
    uint64_t nodeId;
    double ePrev;
    __typeof__(STABILITY_WINDOW) stableCount;
    int sf;
    double txPower;
    UT_hash_handle hh;
} PDState;

typedef struct {
    int dr;
    float bw;
    int rxPower;
    uint64_t nodeId;
} Uplink;

typedef struct {
    int dr;
    int txPower;
    double calculatedSNR;
    double deltaP;
} Command;

PDState* pd = NULL;

PDState* findPDState(uint64_t nodeId) {
    PDState* state;

    HASH_FIND(hh, pd, &nodeId, sizeof(uint64_t), state);
    return state;
}

void initPDState(uint64_t nodeId) {
    PDState* state = (PDState*)malloc(sizeof(PDState));
    state->nodeId = nodeId;
    state->ePrev = 0.0;
    state->stableCount = 0;
    state->sf = SF_INITIAL;
    state->txPower = P_INITIAL;
    HASH_ADD(hh, pd, nodeId, sizeof(uint64_t), state);
}

Command pid(const Uplink* uplink) {
    PDState* state = findPDState(uplink->nodeId);
    if (state == NULL) {
        initPDState(uplink->nodeId);
        state = findPDState(uplink->nodeId);
    }

    int actualSf = DR_TO_SF(uplink->dr);
    if (actualSf != state->sf) {
        printf("Node %lu: actual SF%d != expected SF%d, re-syncing\n",
               uplink->nodeId, actualSf, state->sf);
        state->sf = actualSf;
        state->stableCount = 0;
        state->ePrev = 0.0;
    }

    double snr = uplink->rxPower - NOISE_FLOOR_DBM;
    double target = SNR_TARGET_PER_SF[state->sf - SF_MIN];
    double error = target - snr;

    double deltaP = KP * error + KD * (error - state->ePrev);
    deltaP = round(deltaP / 2.0) * 2.0;
    state->ePrev = error;

    double txPower = CLAMP(state->txPower + deltaP, P_MIN, P_MAX);

    double snrAtMaxPower = snr + (P_MAX - state->txPower);
    int linkStressed =
        (snr < target) || ((snrAtMaxPower - target) < COMFORT_MARGIN);
    int sf = state->sf;

    if (linkStressed && sf < SF_MAX) {
        sf++;
        txPower = P_MAX;
        state->stableCount = 0;
        state->ePrev = 0.0;
    } else if ((-error) > MARGIN_THRESHOLD && txPower <= P_MIN) {
        state->stableCount++;
        if (state->stableCount >= STABILITY_WINDOW && sf > SF_MIN) {
            sf--;
            txPower = P_MAX;
            state->stableCount = 0;
            state->ePrev = 0.0;
        } else if (sf == SF_MIN) {
            state->stableCount = STABILITY_WINDOW;
        }
    } else {
        state->stableCount = 0;
    }

    state->sf = sf;
    state->txPower = txPower;

    Command cmd = {
        .dr = SF_TO_DR(sf),
        .txPower = (int)txPower,
        .calculatedSNR = (double)snr,
        .deltaP = (double)deltaP
    };

    printf(
        "Node %lu command: DR %d (SF%d), TX %d dBm  [snr=%.1f "
        "target=%.1f err=%.1f]\n",
        uplink->nodeId, cmd.dr, sf, cmd.txPower, snr, target, error);

    return cmd;
}