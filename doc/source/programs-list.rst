
rs-canopen-dump
----------------

This program connects to CAN bus and attempt to parse all incoming frames, and
print the frame in a human-readable format on the standard output.


rs-canopen-monitor
------------------

This program connects to CAN bus and attempt to parse all incoming frames, and
maintains a list of recent frames on the standard output. Unlike rs-opencan-dump,
this applications clears the screen between each update.

The application is called as::

    $ rs-canopen-monitor CAN-DEVICE

where CAN-DEVICE is, e.g., can0 or can1, etc.


rs-canopen-nmt
--------------

CANopen network management: this application can read and set the state
of CANopen devices. Possible state commands are *start*, *stop*, *pre-op*,
*reset-app*, *reset-com*. 

The application is called as::

    $ rs-canopen-monitor CAN-DEVICE COMMAND NODE

where CAN-DEVICE is, e.g., can0 or can1, etc., and COMMAND is one state 
commands listed above (or *poll* for only reading out the state), and 
NODE is the 7-bit or 29-bit CANopen node ID.
   COMMAND options:
       
   * start: Put the node in the started state.
   * stop:  Put the node in the stopped.
   * pre-op: Put the node in the pre-operational mode.
   * reset-app: Reset the application.
   * reset-com: Reset the communicatin.
   
Examples::
   
   $ rs-canopen-monitor can0 stop 07
   $ rs-canopen-monitor can0 start 07

rs-canopen-node-info
--------------------

Print information for a CANopen node, including manufacturer ID, product
ID, software version, etc. These values are read from the device using the
expediated SDO protocol, and should be available for all CANopen compatible
devices. 

The application is called as::

    $ rs-canopen-information CAN-DEVICE NODE

where CAN-DEVICE is, e.g., can0 or can1, etc., NODE is the 7-bit or 29-bit
CANopen node ID.

rs-canopen-pdo-download
-----------------------

This program can be used for downloading PDOs to slave units.


rs-canopen-pdo-upload
---------------------

This program can be used for uploading PDOs from slave units.


rs-canopen-sdo-download
-----------------------

This program can be used for downloading SDOs to slave units.


rs-canopen-sdo-upload
---------------------

This program can be used for uploading SDOs from slave units.

