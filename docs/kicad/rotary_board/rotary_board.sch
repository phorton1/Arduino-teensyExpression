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
L Device:Rotary_Encoder_Switch SW1
U 1 1 64C5B1C3
P 5100 2700
F 0 "SW1" H 5100 3067 50  0000 C CNN
F 1 "Rotary_Encoder_Switch" H 5100 2976 50  0000 C CNN
F 2 "0_my_footprints:myRotaryEncoder" H 4950 2860 50  0001 C CNN
F 3 "~" H 5100 2960 50  0001 C CNN
	1    5100 2700
	1    0    0    -1  
$EndComp
$Comp
L Device:Rotary_Encoder_Switch SW3
U 1 1 64C5CF2D
P 5100 3550
F 0 "SW3" H 5100 3917 50  0000 C CNN
F 1 "Rotary_Encoder_Switch" H 5100 3826 50  0000 C CNN
F 2 "0_my_footprints:myRotaryEncoder" H 4950 3710 50  0001 C CNN
F 3 "~" H 5100 3810 50  0001 C CNN
	1    5100 3550
	1    0    0    -1  
$EndComp
$Comp
L Device:Rotary_Encoder_Switch SW2
U 1 1 64C5D992
P 6900 2700
F 0 "SW2" H 6900 3067 50  0000 C CNN
F 1 "Rotary_Encoder_Switch" H 6900 2976 50  0000 C CNN
F 2 "0_my_footprints:myRotaryEncoder" H 6750 2860 50  0001 C CNN
F 3 "~" H 6900 2960 50  0001 C CNN
	1    6900 2700
	1    0    0    -1  
$EndComp
$Comp
L Device:Rotary_Encoder_Switch SW4
U 1 1 64C5E81E
P 6900 3550
F 0 "SW4" H 6900 3917 50  0000 C CNN
F 1 "Rotary_Encoder_Switch" H 6900 3826 50  0000 C CNN
F 2 "0_my_footprints:myRotaryEncoder" H 6750 3710 50  0001 C CNN
F 3 "~" H 6900 3810 50  0001 C CNN
	1    6900 3550
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x05 J2
U 1 1 64C68167
P 3450 3500
F 0 "J2" H 3700 3400 50  0000 C CNN
F 1 "ROTARY34" H 3700 3500 50  0000 C CNN
F 2 "0_my_footprints:my5Pin" H 3450 3500 50  0001 C CNN
F 3 "~" H 3450 3500 50  0001 C CNN
	1    3450 3500
	-1   0    0    -1  
$EndComp
Text GLabel 4800 2600 0    50   Input ~ 0
ROT1A
Text GLabel 4800 2800 0    50   Input ~ 0
ROT1B
Text GLabel 6600 2600 0    50   Input ~ 0
ROT2A
Text GLabel 6600 2800 0    50   Input ~ 0
ROT2B
Text GLabel 4800 3450 0    50   Input ~ 0
ROT3A
Text GLabel 4800 3650 0    50   Input ~ 0
ROT3B
Text GLabel 6600 3450 0    50   Input ~ 0
ROT4A
Text GLabel 6600 3650 0    50   Input ~ 0
ROT4B
Text GLabel 3650 2600 2    50   Input ~ 0
ROT1A
Text GLabel 3650 2700 2    50   Input ~ 0
ROT1B
Text GLabel 3650 2900 2    50   Input ~ 0
ROT2A
Text GLabel 3650 2800 2    50   Input ~ 0
ROT2B
$Comp
L Connector_Generic:Conn_01x05 J1
U 1 1 64C679BC
P 3450 2800
F 0 "J1" H 3700 2700 50  0000 C CNN
F 1 "ROTARY12" H 3700 2800 50  0000 C CNN
F 2 "0_my_footprints:my5Pin" H 3450 2800 50  0001 C CNN
F 3 "~" H 3450 2800 50  0001 C CNN
	1    3450 2800
	-1   0    0    -1  
$EndComp
Text GLabel 3650 3300 2    50   Input ~ 0
ROT3A
Text GLabel 3650 3400 2    50   Input ~ 0
ROT3B
Text GLabel 3650 3600 2    50   Input ~ 0
ROT4A
Text GLabel 3650 3500 2    50   Input ~ 0
ROT4B
$Comp
L power:GND #PWR08
U 1 1 64C96CEE
P 3650 3000
F 0 "#PWR08" H 3650 2750 50  0001 C CNN
F 1 "GND" V 3655 2872 50  0000 R CNN
F 2 "" H 3650 3000 50  0001 C CNN
F 3 "" H 3650 3000 50  0001 C CNN
	1    3650 3000
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR01
U 1 1 64C9A6A3
P 5400 2600
F 0 "#PWR01" H 5400 2350 50  0001 C CNN
F 1 "GND" V 5405 2472 50  0000 R CNN
F 2 "" H 5400 2600 50  0001 C CNN
F 3 "" H 5400 2600 50  0001 C CNN
	1    5400 2600
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR05
U 1 1 64C9A970
P 5400 2800
F 0 "#PWR05" H 5400 2550 50  0001 C CNN
F 1 "GND" V 5405 2672 50  0000 R CNN
F 2 "" H 5400 2800 50  0001 C CNN
F 3 "" H 5400 2800 50  0001 C CNN
	1    5400 2800
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR09
U 1 1 64C9ACF2
P 5400 3450
F 0 "#PWR09" H 5400 3200 50  0001 C CNN
F 1 "GND" V 5405 3322 50  0000 R CNN
F 2 "" H 5400 3450 50  0001 C CNN
F 3 "" H 5400 3450 50  0001 C CNN
	1    5400 3450
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR013
U 1 1 64C9B036
P 5400 3650
F 0 "#PWR013" H 5400 3400 50  0001 C CNN
F 1 "GND" V 5405 3522 50  0000 R CNN
F 2 "" H 5400 3650 50  0001 C CNN
F 3 "" H 5400 3650 50  0001 C CNN
	1    5400 3650
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR02
U 1 1 64C9BACE
P 7200 2600
F 0 "#PWR02" H 7200 2350 50  0001 C CNN
F 1 "GND" V 7205 2472 50  0000 R CNN
F 2 "" H 7200 2600 50  0001 C CNN
F 3 "" H 7200 2600 50  0001 C CNN
	1    7200 2600
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR06
U 1 1 64C9BCFB
P 7200 2800
F 0 "#PWR06" H 7200 2550 50  0001 C CNN
F 1 "GND" V 7205 2672 50  0000 R CNN
F 2 "" H 7200 2800 50  0001 C CNN
F 3 "" H 7200 2800 50  0001 C CNN
	1    7200 2800
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR010
U 1 1 64C9C684
P 7200 3450
F 0 "#PWR010" H 7200 3200 50  0001 C CNN
F 1 "GND" V 7205 3322 50  0000 R CNN
F 2 "" H 7200 3450 50  0001 C CNN
F 3 "" H 7200 3450 50  0001 C CNN
	1    7200 3450
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR014
U 1 1 64C9C980
P 7200 3650
F 0 "#PWR014" H 7200 3400 50  0001 C CNN
F 1 "GND" V 7205 3522 50  0000 R CNN
F 2 "" H 7200 3650 50  0001 C CNN
F 3 "" H 7200 3650 50  0001 C CNN
	1    7200 3650
	0    -1   -1   0   
$EndComp
$Comp
L power:+3V3 #PWR07
U 1 1 64C9D16A
P 3650 3700
F 0 "#PWR07" H 3650 3550 50  0001 C CNN
F 1 "+3V3" V 3650 3800 50  0000 L CNN
F 2 "" H 3650 3700 50  0001 C CNN
F 3 "" H 3650 3700 50  0001 C CNN
	1    3650 3700
	0    1    1    0   
$EndComp
$Comp
L power:+3V3 #PWR03
U 1 1 64CA5009
P 4800 2700
F 0 "#PWR03" H 4800 2550 50  0001 C CNN
F 1 "+3V3" V 4800 2800 50  0000 L CNN
F 2 "" H 4800 2700 50  0001 C CNN
F 3 "" H 4800 2700 50  0001 C CNN
	1    4800 2700
	0    -1   -1   0   
$EndComp
$Comp
L power:+3V3 #PWR011
U 1 1 64CA9ADB
P 4800 3550
F 0 "#PWR011" H 4800 3400 50  0001 C CNN
F 1 "+3V3" V 4800 3650 50  0000 L CNN
F 2 "" H 4800 3550 50  0001 C CNN
F 3 "" H 4800 3550 50  0001 C CNN
	1    4800 3550
	0    -1   -1   0   
$EndComp
$Comp
L power:+3V3 #PWR012
U 1 1 64CAA3EF
P 6600 3550
F 0 "#PWR012" H 6600 3400 50  0001 C CNN
F 1 "+3V3" V 6600 3650 50  0000 L CNN
F 2 "" H 6600 3550 50  0001 C CNN
F 3 "" H 6600 3550 50  0001 C CNN
	1    6600 3550
	0    -1   -1   0   
$EndComp
$Comp
L power:+3V3 #PWR04
U 1 1 64CAAC25
P 6600 2700
F 0 "#PWR04" H 6600 2550 50  0001 C CNN
F 1 "+3V3" V 6600 2800 50  0000 L CNN
F 2 "" H 6600 2700 50  0001 C CNN
F 3 "" H 6600 2700 50  0001 C CNN
	1    6600 2700
	0    -1   -1   0   
$EndComp
$EndSCHEMATC
