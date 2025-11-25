# micronovaSupervisor
A simple project to supervise your Micronova controller based stove / boiler

By using several sw & hw modules, the goal of the project is providing a full platform to:
* log data from the boiler into db time series for further analysis
* provide simple and easy user interface to interact with data
* define a TTL level adapter between the most common UART Full duplex interfaces and the 5V
* define a whole comprehensive architecture for reading data, save and analyze them

Please READ the [MicronovaSupervisor.pdf](/MicronovaSupervisor.pdf) to get an overview of the project

![System layout](/ProjectDocumentation/ProjectBlockDiagram.jpeg "System layout")

# Setup Procedure

## 1) Prepare Host Machine
Follow instructions on https://www.raspberrypi.com/software/ to prepare a working OS image for your Raspberry Pi.

## 2) Install Required Software
Run the following commands:
```bash
chmod +x install.sh
sudo ./install.sh
```
- You can update MariaDB configuration using `mysql_secure_installation`.
- Grafana will be accessible at `http://127.0.0.1:3000` (user: admin, password: admin).

## 3) Create the Log Table
```bash
mysql -u root -proot -e 'create schema micronovaLogger;'
mysql -u root -proot micronovaLogger < ./createLogTable.sql
```

## 4) Configure Arduino MKR1010
Depending on the version you choose:

### Option A: OLD Version (Socket Listener Java)
Upload the script [MKR1010_WIFI_READ_WRITE.ino](/MKR1010_WIFI_READ_WRITE/MKR1010_WIFI_READ_WRITE.ino):
- Set your WiFi SSID & password
- Set the server IP & port for sending the datagram (check your WiFi IP with `ip a | grep wl`)

### Option B: NEW Version (MQTT via Node-RED)
Upload the script [MKR1010_WIFI_READ_WRITE_MQTT.ino](/MKR1010_WIFI_READ_WRITE_MQTT/MKR1010_WIFI_READ_WRITE_MQTT.ino):
- Set your WiFi SSID & password
- Set the MQTT broker IP and port (default `1883`)
- Set the MQTT user to `arduinomkr1000`
- Set the topic to `klover`

## 5) Test UART Adapter (for OLD Version)
Use an Oscilloscope to verify the RX level on the PNP transistor emitter is 5V.

![Oscilloscope capture](/img/oscilloscope_capture.png "Oscilloscope capture")
![Adapter schematics](/ProjectDocumentation/micronovaUartAdapter.png "Adapter schematics")

## 6) Choose Supervisor Version and Start Services

### Option A: OLD Version (Socket Listener Java)
1. Compile & start the Java Spooler Buffer:
```bash
cd SpoolerBuffer/
mvn clean package
java -jar target/MicronovaBufferSpooler-0.0.1-SNAPSHOT.jar 8000 jdbc:mariadb://127.0.0.1:3306/micronovaLogger root root
```
2. (Optional) Create a systemd service to start the jar automatically on boot:
```bash
sudo nano /etc/systemd/system/micronova.service
```
Add the following content:
```
[Unit]
Description=Micronova Buffer Spooler
After=network.target

[Service]
User=pi
WorkingDirectory=/home/pi/SpoolerBuffer
ExecStart=/usr/bin/java -jar /home/pi/SpoolerBuffer/target/MicronovaBufferSpooler-0.0.1-SNAPSHOT.jar 8000 jdbc:mariadb://127.0.0.1:3306/micronovaLogger root root
SuccessExitStatus=143
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```
Enable and start the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable micronova
sudo systemctl start micronova
sudo systemctl status micronova
```

### Option B: NEW Version (MQTT via Node-RED)
1. Install Node-RED:
```bash
bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered)
```
2. Install Mosquitto MQTT broker and enable service:
```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
sudo systemctl status mosquitto
```
3. Configure Mosquitto authentication and ACL:
```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd arduinomkr1000
sudo mosquitto_passwd -c /etc/mosquitto/passwd nodreduser
```
Create `/etc/mosquitto/aclfile` with rules:
```
user nodreduser
topic readwrite # for Node-RED

user arduinomkr1000
topic write klover
```
Reference it in `/etc/mosquitto/mosquitto.conf`:
```
allow_anonymous false
password_file /etc/mosquitto/passwd
acl_file /etc/mosquitto/aclfile
```
Restart Mosquitto:
```bash
sudo systemctl restart mosquitto
sudo systemctl status mosquitto
```
4. Deploy the Node-RED flow provided in [nodred_flow.json](/ProjectDocumentation/nodred_flow.json).
5. Make sure Arduino MKR1010 publishes to MQTT topic `klover` using `arduinomkr1000`.

## 7) Import Grafana Dashboard
Download and import the preconfigured dashboard:
[Configuration file](/ProjectDocumentation/grafana_cfg.json)

![Overview](/img/overview_start.png "Overview - starting the boiler")

