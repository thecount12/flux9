/* api.h - Entry points for hosts (Lux). */
#ifndef FLUX9_API_H
#define FLUX9_API_H

#include "flux9.h"

/* MATMUL T0,T1,T2 via the bytecode VM. Writes T0 into dest. Returns 0. */
int flux9_matmul(utensor_t *dest, const utensor_t *a, const utensor_t *b);

#endif /* FLUX9_API_H */
