#include <stdlib.h>

// ired sensor plugs straight into arduino board for this program

// pin config
#define IRED_INP 24
#define IRED_GND 22 // optional
#define IRED_VCC  4 // optional
#define DISP_LED 13

// other config
#define LOGSIZE 200
#define BAUD 57600

// globals
uint8_t sig[LOGSIZE];
uint8_t rdy = 0;
uint8_t bit = 0;
uint16_t pos = 0;
char strtmp[5];

void ParseSamsung(uint8_t newsig, uint8_t len) {
	// Codes are 4 bytes
	// START HI 120-121
	// SPACE LO 16-17
	// DATA0 HI 13-15
	// DATA1 HI 44-45
	if(newsig!=0) { // high
		// START signal
		if((len>117)&&(len<124)) {
			for(uint8_t i=0; i<4; i++) sig[i]=0;
			rdy=1;
			bit=0;
			pos=0;
		// DATA0 signal
		} else if((len>10)&&(len<18)&&(rdy>0)) {
			sig[pos] = (sig[pos]<<1);
			bit++;
		// DATA1 signal - observed with length of -, after valid start sig
		} else if((len>41)&&(len<48)&&(rdy>0)) {
			sig[pos] = (sig[pos]<<1) | 1;
			bit++;
		// garbage signal
		} else {
			rdy=0;
		}
		// increment
		if(bit==8) {
			bit=0;
			pos++;
			if(pos==4) {
				// output
				Serial.write("\n");
				for(pos=0; pos<4; pos++) {
					Serial.write("0x");
					itoa(sig[pos],strtmp,16);
					if((strtmp[0]>96)&&(strtmp[0]<173)) strtmp[0]=strtmp[0]-32;
					if((strtmp[1]>96)&&(strtmp[1]<173)) strtmp[1]=strtmp[1]-32;
					if(strtmp[1]==0) {
						strtmp[1]=strtmp[0];
						strtmp[0]='0';
						strtmp[2]=0;
					}
					Serial.write(strtmp);
					if(pos<3) Serial.write(',');
				}
				rdy=0;
			}
		}
	// SPACER signals
	} else {
		if(!((len>13)&&(len<20))) {
			rdy=0;
		}
	}
}

void ParseSony(uint8_t newsig, uint8_t len) {
	// Codes are 3 nibbles
	// START LO 69-71
	// SPACE HI 13-16
	// DATA0 LO 18-19
	// DATA1 LO 36-37
	if(newsig==0) { // low
		// START signal
		if((len>66)&&(len<74)) {
			for(uint8_t i=0; i<4; i++) sig[i]=0;
			rdy=1;
			bit=0;
			pos=0;
		// DATA0 signal
		} else if((len>15)&&(len<22)&&(rdy>0)) {
			sig[pos] = (sig[pos]<<1);
			bit++;
		// DATA1 signal
		} else if((len>33)&&(len<40)&&(rdy>0)) {
			sig[pos] = (sig[pos]<<1) | 1;
			bit++;
		// garbage signal
		} else {
			rdy=0;
		}
		// increment
		if(bit==4) {
			bit=0;
			pos++;
			if(pos==3) {
				// output
				Serial.write("\n");
				for(pos=0; pos<3; pos++) {
					Serial.write("0x");
					itoa(sig[pos],strtmp,16);
					if((strtmp[0]>96)&&(strtmp[0]<173)) strtmp[0]=strtmp[0]-32;
					Serial.write(strtmp);
					if(pos<2) Serial.write(",");
				}
				rdy=0;
			}
		}
	// SPACER signal
	} else {
		if(!((len>10)&&(len<19))) {
			rdy=0;
		}
	}
}

void RawTimingLo(uint8_t newsig, uint8_t len) {
	if(newsig==0) {
		sig[pos]=len;
		pos++;
		if(pos==LOGSIZE) {
			for(pos=0; pos<LOGSIZE; pos++) {
				itoa(sig[pos],strtmp,10);
				Serial.write(strtmp);
				Serial.write(" ");
			}
			pos=0;
			_delay_ms(1500);
			Serial.write("\nRDY ");
		}
	}
}

void RawTimingHi(uint8_t newsig, uint8_t len) {
	if(newsig!=0) {
		sig[pos]=len;
		pos++;
		if(pos==LOGSIZE) {
			for(pos=0; pos<LOGSIZE; pos++) {
				itoa(sig[pos],strtmp,10);
				Serial.write(strtmp);
				Serial.write(" ");
			}
			pos=0;
			_delay_ms(1500);
			Serial.write("\nRDY ");
		}
	}
}

void RawTimingFull(uint8_t newsig, uint8_t len) {
	sig[pos]=len;
	pos++;
	if(pos==LOGSIZE) {
		for(pos=0; pos<LOGSIZE; pos++) {
			if(pos & 1) {
				Serial.write("_");
			} else {
				Serial.write("^");
			}
			itoa(sig[pos],strtmp,10);
			Serial.write(strtmp);
			Serial.write(" ");
		}
		pos=0;
		_delay_ms(1500);
		Serial.write("\nRDY ");
	}
}

// arduino-ide complains if missing
void setup() {

	// local vars
	uint8_t mode = 0;
	uint8_t crnt = 0;
	uint8_t last = HIGH;
	uint8_t len = 0;
	void (*action)(uint8_t,uint8_t);

	// global vars
	for(pos=0; pos<LOGSIZE; pos++) {
		sig[pos]=0;
	}
	pos=0;

	// setup pins
	pinMode(IRED_INP,INPUT);
	pinMode(IRED_GND,OUTPUT);
	digitalWrite(IRED_GND,LOW);
	pinMode(IRED_VCC,OUTPUT);
	digitalWrite(IRED_VCC,HIGH);
	pinMode(DISP_LED,OUTPUT);
	digitalWrite(DISP_LED,LOW);

	// setup serial
	Serial.begin(BAUD);
	Serial.write("InfraredScanner\n");

	// get mode from user
	Serial.write("1 = Raw Timing Full\n");
	Serial.write("2 = Raw Timing Only Highs\n");
	Serial.write("3 = Raw Timing Only Lows\n");
	Serial.write("4 = Parse Sony\n");
	Serial.write("5 = Parse Samsung/China\n");
	while(!Serial.available()) ;
	mode = Serial.read();
	while((mode<'1')||(mode>'5')) {
		Serial.write("Invalid try again\n");
		while(!Serial.available()) ;
		mode = Serial.read();
	}
	Serial.write("Mode ");
	Serial.write(mode);
	Serial.write(" Selected\n");
	mode = mode-48;
	switch(mode) {
		case 1: action = &RawTimingFull; break;
		case 2: action = &RawTimingHi; break;
		case 3: action = &RawTimingLo; break;
		case 4: action = &ParseSony; break;
		case 5: action = &ParseSamsung; break;
	}

	// warmup period
	// without this get a lot of junk when raw modes start. not sure if it's because the sensor is
	// settling or if its IR coming from the TXRX leds on arduino board. readings do have a pattern 
	// to it. no amount of scrubbing vars will stop it. not a software issue.
	_delay_ms(1500);
	
	Serial.write("RDY ");

	// main loop
	while(1) {
		// delay and read
		_delay_us(26);
		crnt = digitalRead(IRED_INP);

		// output led
		if(crnt==HIGH) {
			digitalWrite(DISP_LED,LOW);
		} else {
			digitalWrite(DISP_LED,HIGH);
		}

		// log
		if(crnt==last) {
			if(len!=255) len++;
		} else {
			action(last,len);
			// loop
			last = crnt;
			len = 1;
		}
	} 
}

// arduino-ide complains if missing
void loop() {
	delay(1);
}

