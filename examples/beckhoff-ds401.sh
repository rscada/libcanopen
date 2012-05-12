sudo ifconfig can0 up

rs-canopen-nmt can0 start 05

rs-canopen-nmt can0 poll 05

# read input register
rs-canopen-sdo-upload can0 05 6000 01


# read output register
rs-canopen-sdo-upload can0 05 6200 01

# write output register
rs-canopen-sdo-download can0 05 6200 01 03
