#include "alpicool_packet.h"
#include <string.h>

/* ============================
 * TRUE NL50 CHECKSUM
 * ============================ */
static uint8_t nl50_checksum(int8_t temp,
                             uint8_t eco,
                             uint8_t power,
                             uint8_t lock,
                             uint8_t protect)
{
    int cs = 0xFD; // базова чексума при -20, всички статуси 0, sign=03

    // Температура
    cs += (temp - (-20));

    // Статуси
    cs += eco;
    cs += power;
    cs += lock;
    cs += protect;   // 0=L, 1=M, 2=H

    return (uint8_t)cs;
}

/* ============================
 * SIGN BYTE RULES
 * ============================ */
static uint8_t nl50_sign(int8_t temp, uint8_t protect)
{
    if (temp >= 0)
        return 0x03;

    // Изключения при минусови
    if (temp == -20 && protect <= 1) return 0x03;
    if (temp == -19 && protect <= 1) return 0x03;

    return 0x04;
}

/* ============================
 * UNIVERSAL PACKET BUILDER
 * ============================ */
void alpicool_packet_build(uint8_t *cmd,
                           const alpicool_status_t *st)
{
    /* Header */
    cmd[0] = 0xFE;
    cmd[1] = 0xFE;
    cmd[2] = 0x11;
    cmd[3] = 0x02;

    /* Flags */
	cmd[4] = st->lock;
	cmd[5] = st->power;
	cmd[6] = st->mode;
	cmd[7] = st->batt_protect;
	cmd[8] = (uint8_t)st->set_temp;

    /* Temperature-dependent block */



    cmd[9]  = 0x14;
    cmd[10] = 0xEC;
    cmd[11] = 0x02;
  

    /* Always zero */
    cmd[12] = 0x00;
    cmd[13] = 0x00;
    cmd[14] = 0x00;
    cmd[15] = 0x00;
    cmd[16] = 0x00;
    cmd[17] = 0x00;

    /* SIGN BYTE (байт 18) */
    cmd[18] = nl50_sign(
		st->set_temp,
		st->batt_protect);

    /* TRUE NL50 CHECKSUM */
    cmd[19] = nl50_checksum(
		st->set_temp,
		st->mode,
		st->power,
		st->lock,
		st->batt_protect);
}
