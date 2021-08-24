#ifndef VL6180X_H
#define VL6180X_H

#include <stdbool.h>
#include <stdint.h>

/* Comment these out if not connected */
#define VL6180X_SECOND
#define VL6180X_THIRD

#define VL6180X_OUT_OF_RANGE (255)

typedef enum
{
    VL6180X_IDX_FIRST,
#ifdef VL6180X_SECOND
    VL6180X_IDX_SECOND,
#endif
#ifdef VL6180X_THIRD
    VL6180X_IDX_THIRD,
#endif
} vl6180x_idx_t;

/**
 * Initializes the sensors in the vl6180x_idx_t enum.
 * NOTE: Each sensor must have its XSHUT pin connected.
 */
bool vl6180x_init(void);

/**
 * Does a single range measurement
 * @param idx selects specific sensor
 * @param range contains the measured range or VL6180X_OUT_OF_RANGE
 *        if out of range.
 * @return True if success, False if error
 * NOTE: Polling-based
 */
bool vl6180x_read_range_single(vl6180x_idx_t idx, uint8_t *range);

#endif /* VL6180X_H */
