#ifndef C8956B76_0345_4E78_B2F4_285F5D9C0316
#define C8956B76_0345_4E78_B2F4_285F5D9C0316

enum charge_status_t{
    NOT_CHARGING,
    CHARGING,
    CHARGE_COMPLETED,
    UNKNOWN
};

float getBatteryVoltage();
charge_status_t getChargeStatus();

#endif /* C8956B76_0345_4E78_B2F4_285F5D9C0316 */
