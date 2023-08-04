EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L 0_my_teensy:myTeensy3.6 U1
U 1 1 64C58045
P 5150 4000
F 0 "U1" H 5700 4550 50  0000 L CNN
F 1 "myTeensy3.6" H 5500 4450 50  0000 L CNN
F 2 "0_my_teensy:myTeensy3.6" H 5150 4000 50  0001 C CNN
F 3 "" H 5150 4000 50  0001 C CNN
	1    5150 4000
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J5
U 1 1 64C5C08D
P 4000 5750
F 0 "J5" V 4250 5700 50  0000 L CNN
F 1 "LEDS" V 4150 5650 50  0000 L CNN
F 2 "0_my_footprints:myPinSocket_1x03" H 4000 5750 50  0001 C CNN
F 3 "~" H 4000 5750 50  0001 C CNN
	1    4000 5750
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x05 J6
U 1 1 64C5D02E
P 4700 5750
F 0 "J6" V 4950 5700 50  0000 L CNN
F 1 "ROTARY12" V 4850 5550 50  0000 L CNN
F 2 "0_my_footprints:myPinSocket_1x05" H 4700 5750 50  0001 C CNN
F 3 "~" H 4700 5750 50  0001 C CNN
	1    4700 5750
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J2
U 1 1 64C61392
P 4950 1300
F 0 "J2" V 5200 1300 50  0000 R CNN
F 1 "TFT_CONTROL" V 5100 1500 50  0000 R CNN
F 2 "0_my_footprints:myPinSocket_1x08" H 4950 1300 50  0001 C CNN
F 3 "~" H 4950 1300 50  0001 C CNN
	1    4950 1300
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x06 J1
U 1 1 64C62D8F
P 3950 1300
F 0 "J1" V 4200 1300 50  0000 R CNN
F 1 "EXPR" V 4100 1350 50  0000 R CNN
F 2 "0_my_footprints:myPinSocket_1x06" H 3950 1300 50  0001 C CNN
F 3 "~" H 3950 1300 50  0001 C CNN
	1    3950 1300
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x05 J7
U 1 1 64C75495
P 5300 5750
F 0 "J7" V 5550 5700 50  0000 L CNN
F 1 "ROTARY34" V 5450 5550 50  0000 L CNN
F 2 "0_my_footprints:myPinSocket_1x05" H 5300 5750 50  0001 C CNN
F 3 "~" H 5300 5750 50  0001 C CNN
	1    5300 5750
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x05 J8
U 1 1 64C7A741
P 5950 5750
F 0 "J8" V 6200 5700 50  0000 L CNN
F 1 "SW_OUT" V 6100 5650 50  0000 L CNN
F 2 "0_my_footprints:myPinSocket_1x05" H 5950 5750 50  0001 C CNN
F 3 "~" H 5950 5750 50  0001 C CNN
	1    5950 5750
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x05 J9
U 1 1 64C7D5D7
P 6450 5750
F 0 "J9" V 6700 5700 50  0000 L CNN
F 1 "SW_IN" V 6600 5650 50  0000 L CNN
F 2 "0_my_footprints:myPinSocket_1x05" H 6450 5750 50  0001 C CNN
F 3 "~" H 6450 5750 50  0001 C CNN
	1    6450 5750
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J3
U 1 1 64C8119F
P 5900 1300
F 0 "J3" V 6150 1300 50  0000 R CNN
F 1 "TFT_DATA" V 6050 1500 50  0000 R CNN
F 2 "0_my_footprints:myPinSocket_1x08" H 5900 1300 50  0001 C CNN
F 3 "~" H 5900 1300 50  0001 C CNN
	1    5900 1300
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J4
U 1 1 64C8B260
P 5000 6650
F 0 "J4" V 4900 6100 50  0000 L CNN
F 1 "SERIAL" V 5000 6050 50  0000 L CNN
F 2 "0_my_footprints:myPinSocket_1x03" H 5000 6650 50  0001 C CNN
F 3 "~" H 5000 6650 50  0001 C CNN
	1    5000 6650
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR013
U 1 1 64C98700
P 4100 5550
F 0 "#PWR013" H 4100 5300 50  0001 C CNN
F 1 "GND" V 4100 5350 50  0000 C CNN
F 2 "" H 4100 5550 50  0001 C CNN
F 3 "" H 4100 5550 50  0001 C CNN
	1    4100 5550
	-1   0    0    1   
$EndComp
$Comp
L power:+5V #PWR014
U 1 1 64C9DA52
P 3900 5550
F 0 "#PWR014" H 3900 5400 50  0001 C CNN
F 1 "+5V" V 3900 5750 50  0000 C CNN
F 2 "" H 3900 5550 50  0001 C CNN
F 3 "" H 3900 5550 50  0001 C CNN
	1    3900 5550
	1    0    0    -1  
$EndComp
Text GLabel 4000 5550 1    50   Input ~ 0
LEDS
$Comp
L power:+3V3 #PWR016
U 1 1 64C9FDB5
P 5500 5550
F 0 "#PWR016" H 5500 5400 50  0001 C CNN
F 1 "+3V3" V 5500 5750 50  0000 C CNN
F 2 "" H 5500 5550 50  0001 C CNN
F 3 "" H 5500 5550 50  0001 C CNN
	1    5500 5550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR015
U 1 1 64CA12B5
P 4900 5550
F 0 "#PWR015" H 4900 5300 50  0001 C CNN
F 1 "GND" V 4900 5350 50  0000 C CNN
F 2 "" H 4900 5550 50  0001 C CNN
F 3 "" H 4900 5550 50  0001 C CNN
	1    4900 5550
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR011
U 1 1 64CA2492
P 4350 4250
F 0 "#PWR011" H 4350 4000 50  0001 C CNN
F 1 "GND" V 4350 4050 50  0000 C CNN
F 2 "" H 4350 4250 50  0001 C CNN
F 3 "" H 4350 4250 50  0001 C CNN
	1    4350 4250
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR08
U 1 1 64CA2F90
P 4450 2700
F 0 "#PWR08" H 4450 2450 50  0001 C CNN
F 1 "GND" V 4450 2500 50  0000 C CNN
F 2 "" H 4450 2700 50  0001 C CNN
F 3 "" H 4450 2700 50  0001 C CNN
	1    4450 2700
	-1   0    0    1   
$EndComp
$Comp
L power:+5V #PWR07
U 1 1 64CA4C9F
P 4350 2700
F 0 "#PWR07" H 4350 2550 50  0001 C CNN
F 1 "+5V" V 4350 2900 50  0000 C CNN
F 2 "" H 4350 2700 50  0001 C CNN
F 3 "" H 4350 2700 50  0001 C CNN
	1    4350 2700
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR012
U 1 1 64CA8793
P 5750 4250
F 0 "#PWR012" H 5750 4100 50  0001 C CNN
F 1 "+3V3" V 5750 4450 50  0000 C CNN
F 2 "" H 5750 4250 50  0001 C CNN
F 3 "" H 5750 4250 50  0001 C CNN
	1    5750 4250
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR010
U 1 1 64CADA6D
P 5750 2700
F 0 "#PWR010" H 5750 2450 50  0001 C CNN
F 1 "GND" V 5750 2500 50  0000 C CNN
F 2 "" H 5750 2700 50  0001 C CNN
F 3 "" H 5750 2700 50  0001 C CNN
	1    5750 2700
	-1   0    0    1   
$EndComp
Text GLabel 4950 4250 3    50   Input ~ 0
LEDS
Text GLabel 4650 4250 3    50   Input ~ 0
ROT1A
Text GLabel 4750 4250 3    50   Input ~ 0
ROT1B
Text GLabel 4850 4250 3    50   Input ~ 0
ROT2B
Text GLabel 5050 4250 3    50   Input ~ 0
ROT2A
Text GLabel 5150 4250 3    50   Input ~ 0
RX3
Text GLabel 5250 4250 3    50   Input ~ 0
TX3
Text GLabel 5000 6450 1    50   Input ~ 0
RX3
Text GLabel 5100 6450 1    50   Input ~ 0
TX3
$Comp
L power:GND #PWR05
U 1 1 64CBF0C9
P 4900 6450
F 0 "#PWR05" H 4900 6200 50  0001 C CNN
F 1 "GND" V 4900 6250 50  0000 C CNN
F 2 "" H 4900 6450 50  0001 C CNN
F 3 "" H 4900 6450 50  0001 C CNN
	1    4900 6450
	-1   0    0    1   
$EndComp
Text GLabel 5350 4250 3    50   Input ~ 0
ROT3A
Text GLabel 5450 4250 3    50   Input ~ 0
ROT3B
Text GLabel 5550 4250 3    50   Input ~ 0
ROT4B
Text GLabel 5650 4250 3    50   Input ~ 0
ROT4A
Text GLabel 4500 5550 1    50   Input ~ 0
ROT1A
Text GLabel 4600 5550 1    50   Input ~ 0
ROT1B
Text GLabel 4700 5550 1    50   Input ~ 0
ROT2B
Text GLabel 4800 5550 1    50   Input ~ 0
ROT2A
Text GLabel 5100 5550 1    50   Input ~ 0
ROT3A
Text GLabel 5200 5550 1    50   Input ~ 0
ROT3B
Text GLabel 5300 5550 1    50   Input ~ 0
ROT4B
Text GLabel 5400 5550 1    50   Input ~ 0
ROT4A
Text GLabel 5850 4250 3    50   Input ~ 0
SW_OUT1
Text GLabel 5950 4250 3    50   Input ~ 0
SW_OUT2
Text GLabel 6050 4250 3    50   Input ~ 0
SW_OUT3
Text GLabel 6150 4250 3    50   Input ~ 0
SW_OUT4
Text GLabel 6250 4250 3    50   Input ~ 0
SW_OUT5
Text GLabel 6350 4250 3    50   Input ~ 0
SW_IN1
Text GLabel 6450 4250 3    50   Input ~ 0
SW_IN2
Text GLabel 6550 4250 3    50   Input ~ 0
SW_IN3
Text GLabel 6650 4250 3    50   Input ~ 0
SW_IN4
Text GLabel 6650 2700 1    50   Input ~ 0
SW_IN5
Text GLabel 5750 5550 1    50   Input ~ 0
SW_OUT1
Text GLabel 5850 5550 1    50   Input ~ 0
SW_OUT2
Text GLabel 5950 5550 1    50   Input ~ 0
SW_OUT3
Text GLabel 6050 5550 1    50   Input ~ 0
SW_OUT4
Text GLabel 6150 5550 1    50   Input ~ 0
SW_OUT5
Text GLabel 6250 5550 1    50   Input ~ 0
SW_IN1
Text GLabel 6350 5550 1    50   Input ~ 0
SW_IN2
Text GLabel 6450 5550 1    50   Input ~ 0
SW_IN3
Text GLabel 6550 5550 1    50   Input ~ 0
SW_IN4
Text GLabel 6650 5550 1    50   Input ~ 0
SW_IN5
$Comp
L power:GND #PWR01
U 1 1 64CFCC09
P 4250 1500
F 0 "#PWR01" H 4250 1250 50  0001 C CNN
F 1 "GND" V 4250 1300 50  0000 C CNN
F 2 "" H 4250 1500 50  0001 C CNN
F 3 "" H 4250 1500 50  0001 C CNN
	1    4250 1500
	1    0    0    -1  
$EndComp
$Comp
L Device:R R1
U 1 1 64CFCDE5
P 3750 1650
F 0 "R1" H 3600 1650 50  0000 L CNN
F 1 "1K" V 3750 1600 50  0000 L CNN
F 2 "0_my_footprints:myResistor" V 3680 1650 50  0001 C CNN
F 3 "~" H 3750 1650 50  0001 C CNN
	1    3750 1650
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR09
U 1 1 64D070B3
P 4550 2700
F 0 "#PWR09" H 4550 2550 50  0001 C CNN
F 1 "+3V3" V 4550 2900 50  0000 C CNN
F 2 "" H 4550 2700 50  0001 C CNN
F 3 "" H 4550 2700 50  0001 C CNN
	1    4550 2700
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR06
U 1 1 64D07CE1
P 3750 1800
F 0 "#PWR06" H 3750 1650 50  0001 C CNN
F 1 "+3V3" V 3750 2000 50  0000 C CNN
F 2 "" H 3750 1800 50  0001 C CNN
F 3 "" H 3750 1800 50  0001 C CNN
	1    3750 1800
	-1   0    0    1   
$EndComp
Text GLabel 3850 1500 3    50   Input ~ 0
EXPR1
Text GLabel 3950 1500 3    50   Input ~ 0
EXPR2
Text GLabel 4050 1500 3    50   Input ~ 0
EXPR3
Text GLabel 4150 1500 3    50   Input ~ 0
EXPR4
Text GLabel 4650 2700 1    50   Input ~ 0
EXPR1
Text GLabel 4750 2700 1    50   Input ~ 0
EXPR2
Text GLabel 4850 2700 1    50   Input ~ 0
EXPR3
Text GLabel 4950 2700 1    50   Input ~ 0
EXPR4
$Comp
L power:+5V #PWR02
U 1 1 64D14F5D
P 4650 1500
F 0 "#PWR02" H 4650 1350 50  0001 C CNN
F 1 "+5V" V 4650 1700 50  0000 C CNN
F 2 "" H 4650 1500 50  0001 C CNN
F 3 "" H 4650 1500 50  0001 C CNN
	1    4650 1500
	-1   0    0    1   
$EndComp
$Comp
L power:+3V3 #PWR03
U 1 1 64D171C2
P 5250 1500
F 0 "#PWR03" H 5250 1350 50  0001 C CNN
F 1 "+3V3" V 5250 1700 50  0000 C CNN
F 2 "" H 5250 1500 50  0001 C CNN
F 3 "" H 5250 1500 50  0001 C CNN
	1    5250 1500
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR04
U 1 1 64D19F15
P 5350 1500
F 0 "#PWR04" H 5350 1250 50  0001 C CNN
F 1 "GND" V 5350 1300 50  0000 C CNN
F 2 "" H 5350 1500 50  0001 C CNN
F 3 "" H 5350 1500 50  0001 C CNN
	1    5350 1500
	1    0    0    -1  
$EndComp
Text GLabel 4750 1500 3    50   Input ~ 0
TFT_RES
Text GLabel 4850 1500 3    50   Input ~ 0
TFT_CS
Text GLabel 4950 1500 3    50   Input ~ 0
TFT_CD_RS
Text GLabel 5050 1500 3    50   Input ~ 0
TFT_WR
Text GLabel 5150 1500 3    50   Input ~ 0
TFT_RD
Text GLabel 5600 1500 3    50   Input ~ 0
TFT_DAT1
Text GLabel 5700 1500 3    50   Input ~ 0
TFT_DAT0
Text GLabel 5800 1500 3    50   Input ~ 0
TFT_DAT7
Text GLabel 5900 1500 3    50   Input ~ 0
TFT_DAT6
Text GLabel 6000 1500 3    50   Input ~ 0
TFT_DAT5
Text GLabel 6100 1500 3    50   Input ~ 0
TFT_DAT4
Text GLabel 6200 1500 3    50   Input ~ 0
TFT_DAT3
Text GLabel 6300 1500 3    50   Input ~ 0
TFT_DAT2
Text GLabel 5150 2700 1    50   Input ~ 0
TFT_RES
Text GLabel 5250 2700 1    50   Input ~ 0
TFT_CS
Text GLabel 5350 2700 1    50   Input ~ 0
TFT_CD_RS
Text GLabel 5450 2700 1    50   Input ~ 0
TFT_WR
Text GLabel 5550 2700 1    50   Input ~ 0
TFT_RD
Text GLabel 5050 2700 1    50   Input ~ 0
TFT_DAT1
Text GLabel 5650 2700 1    50   Input ~ 0
TFT_DAT0
Text GLabel 6050 2700 1    50   Input ~ 0
TFT_DAT7
Text GLabel 6150 2700 1    50   Input ~ 0
TFT_DAT6
Text GLabel 6250 2700 1    50   Input ~ 0
TFT_DAT5
Text GLabel 6350 2700 1    50   Input ~ 0
TFT_DAT4
Text GLabel 6450 2700 1    50   Input ~ 0
TFT_DAT3
Text GLabel 6550 2700 1    50   Input ~ 0
TFT_DAT2
NoConn ~ 5850 2700
NoConn ~ 5950 2700
Text Notes 5100 6800 3    50   ~ 0
GND\nTIP\nSLEEVE
$EndSCHEMATC
