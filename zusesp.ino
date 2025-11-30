#include <Arduino.h>
#include "Zusi3Schnittstelle.h"
#include <SimpleCLI.h>
#include <Preferences.h>


Zusi3Schnittstelle* zusi;

SimpleCLI cli;
Command cmdWifi;
Command cmdHelp;
Command cmdZusi;

Preferences prefs;


void errorCallback(cmd_error* e) {
	CommandError cmdError(e);
	Serial.print("ERROR: ");
	Serial.println(cmdError.toString());

	Serial.println();
	Serial.println("HELP MESSAGE:");
	Serial.println(cli.toString());
}


void wifiCallback(Command* c) {
	Argument arg = c->getArgument(0);
	String ssid = arg.getValue();

	arg = c->getArgument(1);
	String password = arg.getValue();

	prefs.putString("ssid", ssid);
	prefs.putString("password", password);

	Serial.print("Saved ssid \"");
	Serial.print(ssid);
	Serial.print("\" and password \"");
	Serial.print(password);
	Serial.print("\"");
	Serial.println();

	Serial.println("Restarting...");
	ESP.restart();
}


void zusiCallback(Command* c) {
	Argument arg = c->getArgument(0);
  String ip = arg.getValue();
	
	arg = c->getArgument(1);
  String port = arg.getValue();

	prefs.putString("ip", ip);
	prefs.putInt("port", port.toInt());

	Serial.print("Saved ip \"");
	Serial.print(ip);
	Serial.print("\" and port \"");
	Serial.print(port.toInt());
	Serial.print("\"");
	Serial.println();

	Serial.println("Restarting...");
	ESP.restart();
}


void updateCli() {
	if (Serial.available()) {
		String input = Serial.readStringUntil('\n');
		cli.parse(input);
	}

	if (cli.available()) {
    Command c = cli.getCmd();

		int argNum = c.countArgs();

		Serial.print("> ");
		Serial.print(c.getName());
		Serial.print(' ');

		for (int i = 0; i<argNum; ++i) {
			Argument arg = c.getArgument(i);
			if(arg.isSet()) {
				Serial.print(arg.toString());
				Serial.print(' ');
			}
		}

		Serial.println();

		if (c == cmdWifi) { wifiCallback(&c); }
		if (c == cmdZusi) { zusiCallback(&c); }
		else if (c == cmdHelp) { 
			Serial.println();
			Serial.println("HELP MESSAGE:");
			Serial.println(cli.toString());
		}
	}
}


void setup() {
	Serial.begin(115200);

	prefs.begin("ESP Tacho", false);

	cli.setOnError(errorCallback);

	cmdWifi = cli.addCmd("wifi");
	cmdWifi.addArg("ssid");
	cmdWifi.addArg("password");
	cmdWifi.setDescription("Restarts the esp32 with the new wifi configuration.");

	cmdZusi = cli.addCmd("zusi");
	cmdZusi.addArg("ip");
	cmdZusi.addArg("port");
	cmdZusi.setDescription("Restarts the esp32 with the new zusi.");

	cmdHelp = cli.addCommand("help");
	cmdHelp.setDescription("Prints this help message.");

	pinMode(19, OUTPUT);
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(3, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);

	String ssid = prefs.getString("ssid", "");
	String password = prefs.getString("password", "");
	String ip = prefs.getString("ip", "");
	int port = prefs.getInt("port", 0);

#if defined(ESP8266_Wifi) || defined(ESP32_Wifi)
	Serial.print("Verbinde mit ");
	Serial.println(ssid);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		updateCli();
	}

	Serial.println("\nVerbunden");
	Serial.print("IP-Adresse: ");
	Serial.println(WiFi.localIP());
#endif

#ifdef ESP32_Ethernet
	ETH.begin();
#endif

#ifdef Ethernet_Shield
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		return;
	} else {
		Serial.print("IP-Adresse: ");
		Serial.println(Ethernet.localIP());
	}
#endif

#ifdef AVR_Wifi
	if (WiFi.status() == WL_NO_SHIELD) {
		Serial.println("WiFi shield nicht vorhanden");
		return;
	}
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		updateCli();
		delay(500);
		Serial.print(".");
	}
	Serial.println("\nVerbunden");
	Serial.print("IP-Adresse: ");
	Serial.println(WiFi.localIP());
#endif
	zusi = new Zusi3Schnittstelle(ip, port, "ESP Tacho");
	// zusi->setDebugOutput(true); this causes a random error
	zusi->reqFstAnz(Geschwindigkeit);
	zusi->reqFstAnz(Status_Zugbeeinflussung);
	zusi->requestFuehrerstandsbedienung(false);
	zusi->requestProgrammdaten(false);
	int i = 0;
	while (!zusi->connect()) {
		updateCli();
		Serial.print("Verbindung zu Zusi fehlgeschlagen (");
		Serial.print(++i);
		Serial.println(")");
		delay(2000);
	}
	Serial.println("Verbunden mit Zusi");
}


void loop() {
	updateCli();

	Node *node = zusi->update();
	if (node != NULL) {
		for (int i = 0; i < node->getNodes()->size(); i++) {
			Node *subNode = node->getNodes()->get(i);
			if (subNode->getIDAsInt() == 0x0A) {
				for (int j = 0; j < subNode->getAttribute()->size(); j++) {
					Attribute *attr = subNode->getAttribute()->get(j);
					if (attr->getIDAsInt() == Geschwindigkeit) {
						double kilometersPerHour = attr->getDATAAsFloat() * 3.6F;
						int pwm = round(0.00547 * kilometersPerHour * kilometersPerHour + 1.26531 * kilometersPerHour + -2.12189);
						analogWrite(19, pwm);
						//Serial.print((int) kilometersPerHour);
						//Serial.println(" km/h");
						//Serial.print("pwm: ");
						//Serial.println(pwm);

					}
				}
				for (int j = 0; j < subNode->getNodes()->size(); j++) {
					Node *itemNode = subNode->getNodes()->get(j);
					if (itemNode->getIDAsInt() == Status_Zugbeeinflussung) {
						for (int k = 0; k < itemNode->getNodes()->size(); k++) {
							Node *pzbNode = itemNode->getNodes()->get(k);
							if (pzbNode->getIDAsInt() == 0x03) {
								for (int l = 0; l < pzbNode->getAttribute()->size(); l++) {
									Attribute *attr = pzbNode->getAttribute()->get(l);
									boolean state = attr->getDATAAsBoolean();
									switch (attr->getIDAsInt()) {
									case 5: //1000Hz
										digitalWrite(4, state); //D3
										break;
									case 6: //55
										digitalWrite(3, state); //D0
										break;
									case 7: //70
										digitalWrite(1, state); //D1
										break;
									case 8: //85
										digitalWrite(0, state); //D2
										break;
									case 10: //500Hz
										digitalWrite(5, state); //D7
										break;
									case 11:  //Befehl 40
										digitalWrite(6, state); //D5
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
