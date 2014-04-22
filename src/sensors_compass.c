#include "board.h"
#include "mw.h"

#include "common/axis.h"

int16_t magADC[XYZ_AXIS_COUNT];
sensor_align_e magAlign = 0;
#ifdef MAG
static uint8_t magInit = 0;

void compassInit(void)
{
    // initialize and calibration. turn on led during mag calibration (calibration routine blinks it)
    LED1_ON;
    hmc5883lInit();
    LED1_OFF;
    magInit = 1;
}

int compassGetADC(void)
{
    static uint32_t t, tCal = 0;
    static int16_t magZeroTempMin[3];
    static int16_t magZeroTempMax[3];
    uint32_t axis;

    if ((int32_t)(currentTime - t) < 0)
        return 0;                 //each read is spaced by 100ms
    t = currentTime + 100000;

    // Read mag sensor
    hmc5883lRead(magADC);
    alignSensors(magADC, magADC, magAlign);

    if (f.CALIBRATE_MAG) {
        tCal = t;
        for (axis = 0; axis < 3; axis++) {
            masterConfig.magZero[axis] = 0;
            magZeroTempMin[axis] = magADC[axis];
            magZeroTempMax[axis] = magADC[axis];
        }
        f.CALIBRATE_MAG = 0;
    }

    if (magInit) {              // we apply offset only once mag calibration is done
        magADC[X] -= masterConfig.magZero[X];
        magADC[Y] -= masterConfig.magZero[Y];
        magADC[Z] -= masterConfig.magZero[Z];
    }

    if (tCal != 0) {
        if ((t - tCal) < 30000000) {    // 30s: you have 30s to turn the multi in all directions
            LED0_TOGGLE;
            for (axis = 0; axis < 3; axis++) {
                if (magADC[axis] < magZeroTempMin[axis])
                    magZeroTempMin[axis] = magADC[axis];
                if (magADC[axis] > magZeroTempMax[axis])
                    magZeroTempMax[axis] = magADC[axis];
            }
        } else {
            tCal = 0;
            for (axis = 0; axis < 3; axis++) {
                masterConfig.magZero[axis] = (magZeroTempMin[axis] + magZeroTempMax[axis]) / 2; // Calculate offsets
            }
            copyCurrentProfileToProfileSlot(masterConfig.current_profile);
            writeEEPROM();
            readEEPROMAndNotify();
        }
    }

    return 1;
}
#endif
