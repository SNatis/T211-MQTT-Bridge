# T211 MQTT Bridge

With this firmware for ESP-01, it is possible to read data from Belgian smart electricity meters and send them via the MQTT protocol to your home automation box.


## Features

- It is compatible with the majority of smart meters installed in Belgium
- Transfers information to an MQTT server every 3 seconds. The topic refers to the counter number and the payload contains the value of this counter.
- Automates specific tasks to prevent their injection into the network.
- Real-time monitoring of your installation's consumption and electrical injection.

