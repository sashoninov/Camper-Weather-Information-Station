#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "alpicool.h"

void alpicool_packet_build(uint8_t *cmd,
                           const alpicool_status_t *st);
