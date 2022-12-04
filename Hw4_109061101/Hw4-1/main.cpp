#include "DynamicMessageBufferFactory.h"
#include "TextLCD.h"
#include "UARTTransport.h"
#include "blink_led_server.h"
#include "drivers/DigitalOut.h"
#include "erpc_basic_codec.h"
#include "erpc_crc16.h"
#include "erpc_simple_server.h"
#include "mbed.h"

/**
 * Macros for setting console flow control.
 */
#define CONSOLE_FLOWCONTROL_RTS 1
#define CONSOLE_FLOWCONTROL_CTS 2
#define CONSOLE_FLOWCONTROL_RTSCTS 3
#define mbed_console_concat_(x) CONSOLE_FLOWCONTROL_##x
#define mbed_console_concat(x) mbed_console_concat_(x)
#define CONSOLE_FLOWCONTROL \
    mbed_console_concat(MBED_CONF_TARGET_CONSOLE_UART_FLOW_CONTROL)

mbed::DigitalOut led1(LED1, 1);
mbed::DigitalOut led2(LED2, 1);
mbed::DigitalOut led3(LED3, 1);
mbed::DigitalOut* leds[] = {&led1, &led2, &led3};

/** erpc infrastructure */
ep::UARTTransport uart_transport(D1, D0, 9600);
ep::DynamicMessageBufferFactory dynamic_mbf;
erpc::BasicCodecFactory basic_cf;
erpc::Crc16 crc16;
erpc::SimpleServer rpc_server;

/** LED service */
LEDBlinkService_service led_service;

// Host PC Communication channels
static BufferedSerial pc(USBTX, USBRX);  // tx, rx

// I2C Communication
mbed::I2C i2c_lcd(D14, D15);  // SDA, SCL

// TextLCD_SPI lcd(&spi_lcd, p8, TextLCD::LCD40x4);   // SPI bus, 74595
// expander, CS pin, LCD Type
TextLCD_I2C lcd(
    &i2c_lcd, 0x4E,
    TextLCD::LCD16x2);  // I2C bus, PCF8574 Slaveaddress, LCD Type
                        // TextLCD_I2C lcd(&i2c_lcd, 0x42, TextLCD::LCD16x2,
                        // TextLCD::WS0010);
                        // I2C bus, PCF8574 Slaveaddress, LCD Type, Device Type
                        // TextLCD_SPI_N lcd(&spi_lcd, p8, p9);
                        // SPI bus, CS pin, RS pin, LCDType=LCD16x2, BL=NC,
                        // LCDTCtrl=ST7032_3V3
// TextLCD_I2C_N lcd(&i2c_lcd, ST7032_SA, TextLCD::LCD16x2, NC,
// TextLCD::ST7032_3V3);
//  I2C bus, Slaveaddress, LCD Type, BL=NC, LCDTCtrl=ST7032_3V3

FileHandle* mbed::mbed_override_console(int fd) { return &pc; }

/****** erpc declarations *******/
// uLCD: putc
void led_on(uint8_t led) { lcd.putc('+'); }

// uLCD: locate
void led_off(uint8_t led) {
    lcd.setCursor(TextLCD::CurOff_BlkOn);
    lcd.locate(9, 0);
}

int main() {
    // Initialize the rpc server
    uart_transport.setCrc16(&crc16);

    // Set up hardware flow control, if needed
#if CONSOLE_FLOWCONTROL == CONSOLE_FLOWCONTROL_RTS
    uart_transport.set_flow_control(mbed::SerialBase::RTS, STDIO_UART_RTS, NC);
#elif CONSOLE_FLOWCONTROL == CONSOLE_FLOWCONTROL_CTS
    uart_transport.set_flow_control(mbed::SerialBase::CTS, NC, STDIO_UART_CTS);
#elif CONSOLE_FLOWCONTROL == CONSOLE_FLOWCONTROL_RTSCTS
    uart_transport.set_flow_control(mbed::SerialBase::RTSCTS, STDIO_UART_RTS,
                                    STDIO_UART_CTS);
#endif

    printf("Initializing server.\n");
    rpc_server.setTransport(&uart_transport);
    rpc_server.setCodecFactory(&basic_cf);
    rpc_server.setMessageBufferFactory(&dynamic_mbf);

    // Add the led service to the server
    printf("Adding uLCD server.\n");
    rpc_server.addService(&led_service);

    // Run the server. This should never exit
    printf("Running server.\n");
    rpc_server.run();
}