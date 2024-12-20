# micronovaSupervisor
A simple project to supervise your Micronova contoller base stove / boiler

By usign several sw & hw modules, the goal of the project is providing a full platform to
* log data from the boiler into db timeseries for further analisys
* provide simple and easy user interface to interact with data
* define a TTL level adapter between the most common UART Full duplex interfaces and the 5V
* define a whole comprensive architecture for reading data, storicize them and analize them



Please READ the [MicronovaSupervisor.pdf](/MicronovaSupervisor.pdf) to get an overview of the project


# setup procedure #
1) prepare host machine (see https://www.raspberrypi.com/software/) to get a working iso for your raspberry host
2) install required sw by running the following commands  
        2.1) `chmod +x install.sh`  
        2.3) `sudo ./install.sh`    
        
        - it is possible to update the MariaDb config by running  `mysql_secure_installation` command in temrinal  
        - after installation is possible to get access to grafana at url http://127.0.0.1:3000 user admin, pwd admin  

3) create the log table by running the following commands  
    3.1) `mysql - u root -proot -e 'create schema micronovaLogger;'`  
    3.2) `mysql -u root -proot micronovaLogger < ./createLogTable.sql`    


4) configure your Arduino MKR1010 by uploading the following script [MKR1010_WIFI_READ_WRITE.ino](/MKR1010_WIFI_READ_WRITE/MKR1010_WIFI_READ_WRITE.ino)  
    - change your SSID & PWD to let the board to use your WiFi  
    - change the ip & port to let the board to send the readed datagram to your server (check your WiFi ip typing  `ip a | grep wl`)  

5) Test the UART adapter using an Oscilloscope: the level of RX signals on the Emitter of the PNP transistor MUST be 5V  

6) Compile & Start Java Spooler Buffer  
    6.1) change dir         `cd SpoolerBuffer/`  
    6.2) compile            `mvn clean package`  
    6.3) test manual run    `java -jar target/MicronovaBufferSpooler-0.0.1-SNAPSHOT.jar 8000 jdbc:mariadb://127.0.0.1:3306/micronovaLogger root root`  
