
#include <cstdio>
#include <sapi/sys.hpp>
#include <sapi/hal.hpp>
#include <sapi/var.hpp>


int main(int argc, char * argv[]){
	Cli cli(argc, argv);
	cli.set_publisher("Stratify Labs, Inc");
	cli.handle_version();
	String escape;
	String buffer;
	int ret;

	int port = 0;
	int bitrate = 115200;
	UartPinAssignment pin_assignment;

	if( cli.is_option("-b") ){
		bitrate = cli.get_option_value("-b");
	}

	if( cli.is_option("-p") ){
		port = cli.get_option_value("-p");
	}

	if( cli.is_option("-rx") ){
		pin_assignment->rx = cli.get_option_pin("-rx");
	}

	if( cli.is_option("-tx") ){
		pin_assignment->tx = cli.get_option_pin("-tx");
	}


	//now connect the VCP to the UART
	Uart uart(port);
	Dev serial;

	if( uart.open(Uart::NONBLOCK | Uart::RDWR) < 0 ){
		printf("Failed to open UART %d\n", port);
		exit(1);
	}

	ret = uart.set_attr(Uart::FLAG_SET_LINE_CODING | Uart::FLAG_IS_STOP1 | Uart::FLAG_IS_PARITY_NONE,
			bitrate,
			8,
			pin_assignment);

	if( ret < 0 ){
		printf("Failed to set UART attributes\n");
		exit(1);
	}


	if( serial.open("/dev/link-phy-usb", Dev::RDWR | Dev::NONBLOCK) < 0 ){
		printf("Failed to open USB serial port\n");
		exit(1);
	}

	serial.ioctl(I_FIFO_INIT);

	Pin led(2,10);


	led.init(Pin::FLAG_SET_OUTPUT);
	led.set_value(true);

	printf("Stopping Link in 5 seconds\n");
	Timer::wait_msec(5000);


	//stop the link thread
	pthread_kill(1, SIGSTOP);


	buffer.set_capacity(256);
	escape = "`exit`\n";

	do {

		if( (ret = serial.read(buffer.data(), buffer.capacity())) > 0 ){
			uart.write(buffer.data_const(), ret);
		}

		if( escape == buffer ){
			break;
		}

		buffer.clear();
		if( (ret = uart.read(buffer.data(), buffer.capacity())) > 0 ){
			serial.write(buffer.data_const(), ret);
		}


	} while( buffer != escape );


	pthread_kill(1, SIGCONT);


	return 0;
}
