#ifndef VL6180X_H
#define VL6180X_H

#include <stdbool.h>
#include <stdint.h>

#define VL6180X_OUT_OF_RANGE (255)

bool vl6180x_init(void);

/**
 * Does a single range measurement
 * @param range contains the measured range or VL6180X_OUT_OF_RANGE
 *        if out of range.
 * @return True if success, False if error
 * NOTE: Polling-based
 */
bool vl6180x_read_range_single(uint8_t *range);

#endif /* VL6180X_H */
