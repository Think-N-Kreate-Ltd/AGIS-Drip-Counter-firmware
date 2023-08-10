#ifndef C6CB36BA_B63A_4F4A_9782_117CCF730741
#define C6CB36BA_B63A_4F4A_9782_117CCF730741

#include <Wire.h>
#include <Drip_Counter_API_Config.h>

class Drip_Counter {
    public:
        Drip_Counter();  // TODO: initilizer list?
        ~Drip_Counter(); // TODO: cleanup?

        void init();
        bool requestData();
        uint16_t getDripRate();
        uint16_t getNumDrops();
        void getStatus();
        bool firstDropDetected();

    private:
        TwoWire *i2c_dev = NULL;    // pointer to I2C device
        dripCounterDataPackage_t dripCounterDataPackage;

        bool transmissionTest();

};

#endif /* C6CB36BA_B63A_4F4A_9782_117CCF730741 */
