
#include <cstdio>
#include <sapi/sys.hpp>
#include <sapi/hal.hpp>
#include <sapi/chrono.hpp>
#include <sapi/var.hpp>


static void show_usage(const Cli & cli);

int main(int argc, char * argv[]){
    Cli cli(argc, argv);
    cli.set_publisher("Stratify Labs, Inc");
    cli.handle_version();
    String escape;
    String buffer;
    int ret;
    bool is_bsp;
    bool is_echo;

    UartAttr uart_attr;

    if( cli.handle_uart(uart_attr) == false ){
        show_usage(cli);
        exit(1);
    }

    if( cli.is_option("-bsp") ){
        is_bsp = true;
    } else {
        is_bsp = false;
    }


    if( cli.is_option("-echo") ){
        is_echo = true;
    } else {
        is_echo = false;
    }


    //now connect the VCP to the UART
    Uart uart(uart_attr.port());
    Device serial;

    if( uart.open(Uart::NONBLOCK | Uart::RDWR) < 0 ){
        printf("%s>Failed to open UART %d\n", cli.name(), uart_attr.port());
        exit(1);
    }


    if( is_bsp ){
        if( (ret = uart.set_attr()) < 0 ){
            printf("%s>Failed to set BSP attributes: %d (%d)\n", cli.name(), uart.error_number(), ret);
        }
    } else {
        ret = uart.set_attr(uart_attr);
        if( ret < 0 ){
            printf("%s>Failed to set attributes: %d (%d)\n", cli.name(), uart.error_number(), ret);
        }
    }

    if( ret < 0 ){
        exit(1);
    }


    if( serial.open("/dev/link-phy-usb", Device::RDWR | Device::NONBLOCK) < 0 ){
        printf("%s>Failed to open USB serial port\n", cli.name());
        exit(1);
    }

    serial.ioctl(I_FIFO_INIT);


    printf("%s>Stopping Link in 5 seconds\n", cli.name());
    printf("%s>Type '`exit`\\n' to quit\n", cli.name());
    ClockTime::wait_seconds(5);


    //stop the link thread
    pthread_kill(1, SIGSTOP);


    buffer.set_capacity(256);
    escape = "`exit`\n";

    do {

        buffer.clear();

        if( (ret = serial.read(buffer.data(), buffer.capacity())) > 0 ){
            uart.write(buffer.data_const(), ret);
            if( is_echo ){
                serial.write(buffer.data_const(), ret);
            }
        }

        if( escape == buffer ){
            break;
        }

        buffer.clear();
        if( (ret = uart.read(buffer.data(), buffer.capacity())) > 0 ){
            serial.write(buffer.data_const(), ret);
        }


    } while( buffer != escape );

    String message;
    message.sprintf("%s>`exit` escape received\n", cli.name());
    serial.write(message);
    message.sprintf("%s>exiting\n", cli.name(), cli.name());
    serial.write(message);

    pthread_kill(1, SIGCONT);


    return 0;
}

void show_usage(const Cli & cli){
    printf("%s usage:\n", cli.name());
    printf("\t-uart <port>\n");
    printf("\t-rx <port.pin>\n");
    printf("\t-tx <port.pin>\n");
}
