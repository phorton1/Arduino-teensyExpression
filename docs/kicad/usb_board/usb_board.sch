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
L Connector_Generic:Conn_01x04 J1
U 1 1 64C5F026
P 7250 4350
F 0 "J1" H 7650 4200 50  0000 R CNN
F 1 "USB_OUT" H 7900 4300 50  0000 R CNN
F 2 "0_my_footprints:myJSTx04" H 7250 4350 50  0001 C CNN
F 3 "~" H 7250 4350 50  0001 C CNN
	1    7250 4350
	1    0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J3
U 1 1 64C5F5AC
P 6000 4450
F 0 "J3" V 6000 4550 50  0000 L CNN
F 1 "JUMP_OUT" V 6000 3850 50  0000 L CNN
F 2 "0_my_footprints:myPinHeader_1x02" H 6000 4450 50  0001 C CNN
F 3 "~" H 6000 4450 50  0001 C CNN
	1    6000 4450
	-1   0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J4
U 1 1 64C5F785
P 5900 4450
F 0 "J4" V 5900 4550 50  0000 L CNN
F 1 "JUMP_IN" V 5900 3900 50  0000 L CNN
F 2 "0_my_footprints:myPinHeader_1x02" H 5900 4450 50  0001 C CNN
F 3 "~" H 5900 4450 50  0001 C CNN
	1    5900 4450
	1    0    0    1   
$EndComp
$Comp
L 0_my_symbols:myUSB_B_Socket J5
U 1 1 64C9F949
P 4450 4150
F 0 "J5" H 4950 3500 50  0000 L CNN
F 1 "myUSB_B_Socket" H 4400 3600 50  0000 L CNN
F 2 "0_my_footprints:myUSB_B_Socket" H 5950 4550 50  0001 C CNN
F 3 " ~" H 5950 4550 50  0001 C CNN
	1    4450 4150
	-1   0    0    1   
$EndComp
$Comp
L power:+5V #PWR0101
U 1 1 64CC1368
P 4950 4300
F 0 "#PWR0101" H 4950 4150 50  0001 C CNN
F 1 "+5V" V 4950 4450 50  0000 L CNN
F 2 "" H 4950 4300 50  0001 C CNN
F 3 "" H 4950 4300 50  0001 C CNN
	1    4950 4300
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 64CC1AEE
P 4950 4500
F 0 "#PWR0102" H 4950 4250 50  0001 C CNN
F 1 "GND" V 4950 4350 50  0000 R CNN
F 2 "" H 4950 4500 50  0001 C CNN
F 3 "" H 4950 4500 50  0001 C CNN
	1    4950 4500
	0    -1   -1   0   
$EndComp
Text GLabel 4950 4100 2    50   Input ~ 0
D-
Text GLabel 4950 3900 2    50   Input ~ 0
D+
$Comp
L power:+5V #PWR0103
U 1 1 64CC7C18
P 5700 4350
F 0 "#PWR0103" H 5700 4200 50  0001 C CNN
F 1 "+5V" V 5700 4500 50  0000 L CNN
F 2 "" H 5700 4350 50  0001 C CNN
F 3 "" H 5700 4350 50  0001 C CNN
	1    5700 4350
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0104
U 1 1 64CC7C1E
P 5700 4450
F 0 "#PWR0104" H 5700 4200 50  0001 C CNN
F 1 "GND" V 5700 4300 50  0000 R CNN
F 2 "" H 5700 4450 50  0001 C CNN
F 3 "" H 5700 4450 50  0001 C CNN
	1    5700 4450
	0    1    1    0   
$EndComp
$Comp
L power:+5V #PWR0105
U 1 1 64CCCF6D
P 6400 3700
F 0 "#PWR0105" H 6400 3550 50  0001 C CNN
F 1 "+5V" V 6400 3850 50  0000 L CNN
F 2 "" H 6400 3700 50  0001 C CNN
F 3 "" H 6400 3700 50  0001 C CNN
	1    6400 3700
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR0106
U 1 1 64CCDCEE
P 6300 3700
F 0 "#PWR0106" H 6300 3450 50  0001 C CNN
F 1 "GND" V 6300 3550 50  0000 R CNN
F 2 "" H 6300 3700 50  0001 C CNN
F 3 "" H 6300 3700 50  0001 C CNN
	1    6300 3700
	1    0    0    -1  
$EndComp
Text GLabel 7050 4250 0    50   Input ~ 0
D-
Text GLabel 7050 4150 0    50   Input ~ 0
D+
Wire Wire Line
	4700 4100 4950 4100
Wire Wire Line
	4700 4300 4950 4300
Wire Wire Line
	4200 4100 4200 3750
Wire Wire Line
	4200 3750 4900 3750
Wire Wire Line
	4900 3750 4900 3900
Wire Wire Line
	4900 3900 4950 3900
Wire Wire Line
	4200 4300 4200 4600
Wire Wire Line
	4200 4600 4900 4600
Wire Wire Line
	4900 4600 4900 4500
Wire Wire Line
	4900 4500 4950 4500
Wire Wire Line
	7050 4350 6500 4350
Wire Wire Line
	7050 4450 6600 4450
Wire Wire Line
	6600 3700 6600 4450
Connection ~ 6600 4450
Wire Wire Line
	6600 4450 6200 4450
Wire Wire Line
	6500 3700 6500 4350
Connection ~ 6500 4350
Wire Wire Line
	6500 4350 6200 4350
Text Notes 3650 3050 0    50   ~ 0
I think the old schematic is wrong for this board,\nwhere it shows (L to R) 5V, D-, D+, and GND.\nI think that during implementation I switched\nto these pinouts, and that this board is compatible.
Text Notes 6600 3050 1    50   ~ 0
5v to board\ngnd to board\n5v from board\ngnd from board
Text Notes 7350 4450 0    50   ~ 0
D+\nD-\n5V\nGND
$Comp
L Connector_Generic:Conn_01x04 J2
U 1 1 64C60186
P 6500 3500
F 0 "J2" V 6500 3750 50  0000 C CNN
F 1 "PWR_IO" V 6400 3850 50  0000 C CNN
F 2 "0_my_footprints:myJSTx04" H 6500 3500 50  0001 C CNN
F 3 "~" H 6500 3500 50  0001 C CNN
	1    6500 3500
	0    1    -1   0   
$EndComp
Text Notes 6600 3350 1    50   ~ 0
yellow\nwhite\nred\nblack\n
$EndSCHEMATC
