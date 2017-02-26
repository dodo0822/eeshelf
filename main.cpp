#include <mbed.h>

#include "EthernetInterface.h"

#include "MFRC522.h"
#include "adafruit-ssd1306/Adafruit_SSD1306.h"
#include "mcp230xx/MCP230xx.h"
#include "Servo.h"

#define SHOP_SERVER "192.168.0.103"

DigitalOut led(LED1);

MFRC522 rfid(PB_5, D12, D13, PA_4, D8);
I2C i2c(PB_9, PB_8);
Adafruit_SSD1306_I2c oled(i2c, D4);
MCP23008 mcp(i2c);

EthernetInterface net;

DigitalOut usTrig(PD_12);
DigitalIn usEcho(PD_13);

Servo servo1(D6);
Servo servo2(D5);

TCPSocket socket;

uint8_t cid[10];
uint8_t cidSize;
uint16_t basket;

uint8_t state;

bool keys[4] = { false };

char sckBuf[256];
char rcvBuf[256];

void render() {
	oled.clearDisplay();
	oled.printfPos(30, 5, "eeShelf");

	switch(state) {
	case 0:
		oled.printfPos(0, 20, "Your card please..");
		break;
	case 1:
		oled.printfPos(0, 20, "Hello! Basket value: ");
		oled.setTextSize(2);
		oled.printfPos(10, 40, "$ %u", basket);
		oled.setTextSize(1);
		break;
	case 2:
		oled.printfPos(0, 20, "Server error");
		oled.printfPos(0, 35, "press any key");
		break;
	case 3:
		oled.printfPos(0, 15, "Thank you!");
		oled.printfPos(0, 30, "Current total:");
		oled.setTextSize(2);
		oled.printfPos(10, 45, "$ %u", basket);
		oled.setTextSize(1);
	default:

		break;
	}

	oled.display();
}

float measure() {
	Timer timer;
	float start, end;
	float dist = 0;

	for(uint8_t i = 0; i < 3; ++i) {
		usTrig = 0;
		wait_ms(5);
		timer.reset();
		timer.start();
		usTrig = 1;
		wait_us(10);
		usTrig = 0;
		while(!usEcho) {
			start = timer.read();
		}
		while(usEcho) {
			end = timer.read();
		}
		dist += (end-start) * 17150;
	}
	return dist/3;
}

int8_t poll_key() {
	int8_t key_pressed = -1;
	for(uint8_t i = 0; i < 4; ++i) {
		if(!mcp.input(i)) {
			if(keys[i]) continue;
			keys[i] = true;
			key_pressed = i;
		} else {
			keys[i] = false;
		}
	}
	return key_pressed;
}

bool update_basket() {
	printf("updating basket..\r\n");
	char sckBuf[256] = "";
	char rcvBuf[256];
	sprintf(sckBuf, "r ");
	for(uint8_t i = 0; i < cidSize; ++i) {
		sprintf(sckBuf + strlen(sckBuf), "%x", cid[i]);
	}
	socket.send(sckBuf, strlen(sckBuf));

	uint8_t rcvLen = socket.recv(rcvBuf, sizeof rcvBuf);
	rcvBuf[rcvLen] = '\0';

	if(strcmp(rcvBuf, "-1") == 0) {
		printf("server error!\r\n");
		return false;
	}

	printf("update ok\r\n");
	basket = 0;
	printf("%d\r\n", strlen(rcvBuf));
	for(int i = 0; i < strlen(rcvBuf); ++i) {
		basket *= 10;
		basket += rcvBuf[i] - '0';
	}
	return true;
}

int main() {
	printf("starting..\r\n");

	servo1 = 0.0;
	servo2 = 1.0;

	oled.clearDisplay();
	oled.printfPos(30, 5, "initializing..");
	oled.display();

	net.connect();

	socket.open(&net);
	socket.connect(SHOP_SERVER, 5000);

	const char *ip = net.get_ip_address();
	printf("IP address is: %s\r\n", ip ? ip : "No IP");

	i2c.frequency(400000);
	for(uint8_t i = 0; i < 4; ++i) {
		mcp.setup(i, MCP23008::IN);
		mcp.pullup(i, true);
	}

	rfid.PCD_Init();
	while(1) {
		state = 0;
		render();
		led = 1;

		// try to read RFID card
		if(!rfid.PICC_IsNewCardPresent()) {
			wait(0.3);
			continue;
		}

		if(!rfid.PICC_ReadCardSerial()) {
			wait(0.3);
			continue;
		}

		led = 0;

		// read card
		cidSize = rfid.uid.size;
		for(uint8_t i = 0; i < cidSize; ++i) {
			cid[i] = rfid.uid.uidByte[i];
			printf(" %X", cid[i]);
		}
		printf("\r\n");

		// update basket state
		if(!update_basket()) {
			state = 2;
			render(); 

			while(1) {
				if(poll_key() != -1) break;
				wait(0.2);
			}
			continue;
		}

		state = 1;
		render();

		// open the door
		servo1 = 0.9;
		servo2 = 0.1;

		float orig_dist = measure();
		printf("orig distance is: %f\r\n", orig_dist);

		// wait for key press
		while(1) {
			int8_t key = poll_key();
			if(key == 3) {
				break;
			}
			wait(0.2);
		}

		// close the door
		servo1 = 0.0;
		servo2 = 1.0;

		wait(0.3);

		// calculate and send result
		float dist = measure();
		printf("new distance is: %f\r\n", dist);

		int8_t quantity = round((dist - orig_dist) / 5);

		char sckBuf[256] = "";
		sprintf(sckBuf, "s ");
		for(uint8_t i = 0; i < cidSize; ++i) {
			sprintf(sckBuf + strlen(sckBuf), "%x", cid[i]);
		}
		sprintf(sckBuf + strlen(sckBuf), " 1 %d", quantity);
		socket.send(sckBuf, strlen(sckBuf));

		wait(0.3);

		if(!update_basket()) {
			state = 2;
			render();

			while(1) {
				if(poll_key() != -1) break;
				wait(0.2);
			}
			continue;
		}

		state = 3;
		render();

		wait(3);

	}
}