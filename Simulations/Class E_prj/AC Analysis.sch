<QucsStudio Schematic 5.9>
<Properties>
View=110,-166.03,1442.71,736.333,1.44643,77,411
Grid=10,10,1
DataSet=*.dat
DataDisplay=*.dpl
OpenDisplay=1
showFrame=0
FrameText0=Title \n @PATH@@FILE@
FrameText1=Drawn By:
FrameText2=Date: @DATE@
FrameText3=Revision:
</Properties>
<Symbol>
</Symbol>
<Components>
C C1 1 410 390 -83 -24 3 "228 nF"1"0"0""0"neutral"0"SMD0603"0
C C2 1 490 320 -36 -60 2 "399 nF"1"0"0""0"neutral"0"SMD0603"0
L L2 1 590 320 -26 10 0 "95 nH"1"0"0""0"inductor_1mH"0
GND * 1 410 480 0 0 0
GND * 1 310 550 0 0 0
.AC AC1 1 160 240 0 9 0 "log"1"0.1 MHz"1"10 MHz"1"300"1"no"0"power"0
Vac V3 1 310 520 18 -26 0 "3 V"1"1 MHz"1"0"0"0"0"con_2"0
GND * 1 680 500 0 0 0
GND * 1 770 500 0 0 0
C C3 1 680 470 -84 -26 3 "4.2 uF"1"0"0""0"neutral"0"SMD0603"0
L L1 1 770 470 -62 -26 3 "9 nH"1"0"0""0"inductor_1mH"0
GND * 1 880 490 0 0 0
R R1 1 880 400 -95 -6 3 "0.15 Ω"1"26.85"0"US"0"SMD0603"0
</Components>
<Wires>
410 320 410 360
410 320 460 320
520 320 560 320
410 420 410 480
310 320 410 320
310 320 310 490
620 320 680 320
770 320 770 440
680 320 770 320
680 320 680 440
770 320 880 320
880 430 880 490
880 320 880 370
880 320 880 320 "Vo" 910 270 0 ""
</Wires>
<Diagrams>
</Diagrams>
<Paintings>
</Paintings>
