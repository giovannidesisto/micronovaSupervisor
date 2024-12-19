# micronovaSupervisor
A simple project to supervise your Micronova contoller base stove / boiler

By usign several sw & hw modules, the goal of the project is providing a full platform to
* log data from the boiler into db timeseries for further analisys
* provide simple and easy user interface to interact with data
* define a TTL level adapter between the most common UART Full duplex interfaces and the 5V
* define a whole comprensive architecture for reading data, storicize them and analize them



Please READ the MicronovaSupervisor.pdf to get an overview of the project


# in order to install required sw run the following scripts #
`chmod +x install.sh`
`sudo ./install.sh`


- it is possible to update the MariaDb config by running  `mysql_secure_installation` command in temrinal
- after installation is possible to get access to grafana at url http://127.0.0.1:3000 user admin, pwd admin
