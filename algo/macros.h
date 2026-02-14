#pragma once

#define CLAMP(x, min, max)                              \
    ({                                                  \
        __typeof__(x) _x = (x);                         \
        __typeof__(min) _min = (min);                   \
        __typeof__(max) _max = (max);                   \
        (_x < _min) ? _min : ((_x > _max) ? _max : _x); \
    })

#define DR_TO_SF(dr) (12 - (dr))
#define SF_TO_DR(sf) (12 - (sf))

#define SNR_INCREASE_THRESHOLD 5