/*
   simple driver for a counterfeit texas inst ads1115 adc 
   datasheet is in docs.

   the quickstart guide on p35 describes how the bytes go 
   back and forth:
        - write high byte first.
        - read returns high byte first.
 */
#include "rpi.h"
#include "i2c.h"

// for bitfield checking
#include "libc/helper-macros.h"

// p21 states device will reset itself when you do an
// i2c "general call" reset with 6h (6 hex = 0x6)
// we need to do this b/c  ---- and this is an easy 
// mistake to make --- even though the pi reboots,
// the adc (and all sensors) are still up and running,
// so have the previous configuration.
void adc_reset(void) {
    uint8_t rst = 0x6;
    i2c_write(0, &rst,1);
}

// write a 16 bite register
// p23 to read/write registers
//  1. write to the low two bits in the address pointer register. (p27)
//  2. read the 2 bytes that come back.
static void adc_write16(uint8_t dev_addr, uint8_t reg, uint16_t v) {
    uint8_t data[] = { reg, v >> 8, v & 0xff };    
    if (!i2c_write(dev_addr, &data, sizeof(data)))
        panic("adc_write16: i2c_write(dev_addr, data, sizeof(data)) returned 0\n");
}

// read a 16-bit register
static uint16_t adc_read16(uint8_t dev_addr, uint8_t reg) {
    if (!i2c_write(dev_addr, &reg, 1))
        panic("adc_read16: i2c_write(dev_addr, &reg, 1) returned 0\n");

    uint8_t data[2];
    if (!i2c_read(dev_addr, data, sizeof(data)))
        panic("adc_read16: i2c_read(dev_addr, data, sizeof(data)) returned 0\n");

    return data[0] << 8 | data[1];
}

void notmain(void) {
	uart_init();
	delay_ms(30);   // allow time for device to boot up.
	i2c_init();
	delay_ms(30);   // allow time to settle after init.

    // 0. set these enums to the right values.

    // dev address: p23
    enum { dev_addr = 0b1001000 };
    // p27: register names
    enum { conversion_reg = 0, config_reg = 1 };

    // p28
    // one way to set fields in a register.
    //   note, these are not clearing them.
#   define PGA(x) ((x)<<(9))
#   define MODE(x) ((x)<<(8))
#   define DR(x) ((x)<<5)

#   define MODE_V(x) (((x)>>8)&1)
#   define DR_V(x) (((x)>>5)&0b111)
#   define PGA_V(x) (((x)>>9)&0b111)

    // reset device to known state.
    adc_reset();
    // we seem to have to wait for power up.
    delay_us(25);

    // 1. get the initial config
	uint16_t c = adc_read16(dev_addr, config_reg);
	printk("initial: config reg = =%x (lo=%b,hi=%b)\n", c,c&0xff,c>>8);

    // 2. sanity check the default default values from p29
    //   mode: 8 is 1
    assert(MODE_V(c) == 1);
    //   dr: 7:5 = 0b100
    assert(DR_V(c) == 0b100);
    //   pg: 11:9 = 010
    assert(PGA_V(c) == 0b010);

    // 3. set the config to:
    //  - PGA gain to +/-4v 
    //  - MODE to continuous.
    //  - DR to 860sps
    // see page 28.
    uint16_t config_val = PGA(1) | MODE(0) | DR(0b111); 
    adc_write16(dev_addr, config_reg, config_val);

    // 4. read back the config and make sure the fields we set
    // are correct.
    config_val = adc_read16(dev_addr, config_reg);
    assert(PGA_V(config_val) == 1);
    assert(MODE_V(config_val) == 0);
    assert(DR_V(config_val) == 0b111);

    // 5. just loop and get readings.
    //  - vary your potentiometer
    //  - should get negative readings for low
    //  - around 20k for high.
    //
    // 
    // make sure: given we set gain to +/- 4v.
    // does the result make sense?
	for(int i = 0; i < 20; i++) {
        short v = adc_read16(dev_addr, conversion_reg);
        printk("out=%d\n", v);
		delay_ms(1000);
	}
	clean_reboot();
}

