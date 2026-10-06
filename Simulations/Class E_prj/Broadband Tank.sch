<QucsStudio Schematic 5.9>
<Properties>
View=70,-344.604,1351,667.736,1.19098,76,241
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
GND * 1 140 410 0 0 0
Vac V3 1 140 370 18 -26 0 "3 V"1"1 MHz"1"0"0"0"0"con_2"0
GND * 1 290 380 0 0 0
GND * 1 490 380 0 0 0
GND * 1 750 380 0 0 0
GND * 1 860 380 0 0 0
C C1 1 290 350 17 -26 1 "50 pF"1"0"0""0"neutral"0"SMD0603"0
L L1 1 420 320 -26 10 0 "26.6 nH"1"0"0""0"inductor_1mH"0
C C2 1 490 350 17 -26 1 "43.7 pF"1"0"0""0"neutral"0"SMD0603"0
L L2 1 620 320 -26 10 0 "111 nH"1"0"0""0"inductor_1mH"0
C C3 1 690 320 -26 -55 2 "15.2 pF"1"0"0""0"neutral"0"SMD0603"0
L L3 1 750 350 10 -26 1 "35.1 nH"1"0"0""0"inductor_1mH"0
C C4 1 860 350 17 -26 1 "43.6 pF"1"0"0""0"neutral"0"SMD0603"0
L L4 1 1000 320 -26 10 0 "58.1 nH"1"0"0""0"inductor_1mH"0
C C5 1 1090 320 -26 -55 2 "24.6 pF"1"0"0""0"neutral"0"SMD0603"0
R R1 1 1190 350 15 -18 1 "50 Ω"1"26.85"0"european"0"SMD0603"0
GND * 1 1190 380 0 0 0
.AC AC1 1 120 140 0 9 0 "lin"1"80 MHz"1"230 MHz"1"100"1"no"0"none"0
</Components>
<Wires>
140 400 140 410
140 320 140 340
140 320 290 320
450 320 490 320
720 320 750 320
1030 320 1060 320
1120 320 1190 320
290 320 390 320
650 320 660 320
490 320 590 320
750 320 860 320
860 320 970 320
1190 320 1260 320
1260 320 1260 320 "out" 1290 270 0 ""
</Wires>
<Diagrams>
</Diagrams>
<Paintings>
</Paintings>
