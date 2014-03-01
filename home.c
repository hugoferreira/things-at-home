#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <mosquitto.h>

char *cmdsOn[16] = {"000000100""SLLS""SSSSSLLL""000000",
                    "000000100""SLLS""LSSSSLLL""000000",
                    "000000100""SLLS""SLSSSLLL""000000",
                    "000000100""SLLS""LLSSSLLL""000000",
                    "000000100""SLLS""SSLSSLLL""000000",
                    "000000100""SLLS""LSLSSLLL""000000",
                    "000000100""SLLS""SLLSSLLL""000000",
                    "000000100""SLLS""LLLSSLLL""000000",
                    "000000100""SLLS""SSSLSLLL""000000",
                    "000000100""SLLS""LSSLSLLL""000000",
                    "000000100""SLLS""SLSLSLLL""000000",
                    "000000100""SLLS""LLSLSLLL""000000",
                    "000000100""SLLS""SSLLSLLL""000000",
                    "000000100""SLLS""LSLLSLLL""000000",
                    "000000100""SLLS""SLLLSLLL""000000",
                    "000000100""SLLS""LLLLSLLL""000000"};

char *cmdsOff[16] = {"000000100""SLLS""SSSSSLLS""000000",
                     "000000100""SLLS""LSSSSLLS""000000",
                     "000000100""SLLS""SLSSSLLS""000000",
                     "000000100""SLLS""LLSSSLLS""000000",
                     "000000100""SLLS""SSLSSLLS""000000",
                     "000000100""SLLS""LSLSSLLS""000000",
                     "000000100""SLLS""SLLSSLLS""000000",
                     "000000100""SLLS""LLLSSLLS""000000",
                     "000000100""SLLS""SSSLSLLS""000000",
                     "000000100""SLLS""LSSLSLLS""000000",
                     "000000100""SLLS""SLSLSLLS""000000",
                     "000000100""SLLS""LLSLSLLS""000000",
                     "000000100""SLLS""SSLLSLLS""000000",
                     "000000100""SLLS""LSLLSLLS""000000",
                     "000000100""SLLS""SLLLSLLS""000000",
                     "000000100""SLLS""LLLLSLLS""000000"};

int msgLen = 27; // Use 149 for DIO triple button commands
int repeat = 5;
int messageDelay = 50;
int highBurstLength = 185;
int lowBurstLength  = 370;
int rf433pin = 7;

void nanoSecondDelay(int d) {
  delayMicroseconds(d);
}

int sendrfmsg(char* rfmsg) {
  if (wiringPiSetup () == -1) return 1;

  pinMode (rf433pin, OUTPUT) ;

  printf("Sending RF message.\n");

  for (int j = 0; j < repeat; j++) {
    for (int i = 0; i < strlen(rfmsg); i++) {
      char b = rfmsg[i];
      if (b == 'S') {
        digitalWrite(rf433pin, HIGH);
        nanoSecondDelay(highBurstLength);
        digitalWrite(rf433pin, LOW);
        nanoSecondDelay(lowBurstLength * 2);
        digitalWrite(rf433pin, HIGH);
        nanoSecondDelay(highBurstLength);
        digitalWrite(rf433pin, LOW);
        nanoSecondDelay(lowBurstLength * 2);
      } else if (b == '1') {
        digitalWrite(rf433pin, HIGH);
        nanoSecondDelay(highBurstLength);
        digitalWrite(rf433pin, LOW);
        nanoSecondDelay(lowBurstLength);
      } else if (b == 'L') {
        digitalWrite(rf433pin, HIGH);
        nanoSecondDelay(highBurstLength * 4);
        digitalWrite(rf433pin, LOW);
        nanoSecondDelay(lowBurstLength);
        digitalWrite(rf433pin, HIGH);
        nanoSecondDelay(highBurstLength);
        digitalWrite(rf433pin, LOW);
        nanoSecondDelay(lowBurstLength * 2);
      } else if (b == '0') {
        digitalWrite(rf433pin, LOW);
        nanoSecondDelay(lowBurstLength);
      } else if (b == '2') {
        digitalWrite(rf433pin, HIGH);
        nanoSecondDelay(highBurstLength * 4);
        digitalWrite(rf433pin, LOW);
        nanoSecondDelay(lowBurstLength);
      }
    }

    delay(messageDelay);
  }

  return 0;
}

void rf433_raw_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	int sensor = 0;

	if (message->payloadlen) {
		printf("[MQTT] %s %s.\n", message->topic, message->payload);
		if (strcmp(message->topic, "home/rf433") == 0) {
			sendrfmsg(message->payload);
		} else if (sscanf (message->topic, "home/sensors/%d", &sensor) == 1) {
			printf("Message %s to sensor %d\n", sensor, message->payload);
			if (strcmp(message->payload, "on") == 0) {
				sendrfmsg(cmdsOn[sensor - 1]);
			} else if (strcmp(message->payload, "off") == 0) {
				sendrfmsg(cmdsOff[sensor - 1]);
			}
		}
	} else {
		printf("%s (null)\n", message->topic);
	}
	fflush(stdout);
}

int main(int argc, char *argv[]) {
	char *id = "things-at-home";
	int i;
	char *host = "localhost";
	int port = 1883;
	int keepalive = 60;
	bool clean_session = false;
	struct mosquitto *mosq = NULL;

	printf("Attempting to connect to broker.\n");

	mosquitto_lib_init();
	mosq = mosquitto_new(id, clean_session, NULL);

	if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	mosquitto_message_callback_set(mosq, rf433_raw_callback);

	if (mosquitto_connect(mosq, host, port, keepalive)) {
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}

	printf("Successful.\n");
	mosquitto_subscribe(mosq, NULL, "home/rf433", 2);
    mosquitto_subscribe(mosq, NULL, "home/sensors/#", 2);

	mosquitto_loop_forever(mosq, 1000, 100);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return 0;
}
