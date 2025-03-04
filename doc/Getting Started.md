# Inbetriebnahme des OBP60
Dies Kapitel beschäftigt sich  mit der ersten Inbetriebnahme des OBP60, vom Auspacken bis zum Anzeigen der ersten Daten auf dem Display. Ziel ist es, die erste Inbetriebnahme für Nutzer die sich mit dem Gerät noch  nicht auskennen, möglichst einfach zu machen. 

## Packungsinhalt
In der Packung ist bis auf Verbindungskabel alles, was man so braucht um das Gerät zu montieren und anzuschließen:
- Das Gerät selbst, das OBP60 Marine Display
- Eine Schutzkappe - wichtig wenn man das Gerät im Freien montiert
- Dichtung zur wasserdichten Montage an einer glatten Oberfläche
- Befestigungsschrauben
- Zwei Klemmblöcke, an die die diversen Kabel angeschlossen werden
- Zwei Jumper, also Codierstecker, mit denen die NMEA2000 und NMEA183 Terminierung ktiviert werden können.

## Erste Inbetriebnahme
Es empfiehlt sich, die allererste Inbetriebnahme des Gerätes am Arbeitstisch zu machen. Man benötigt dafür ein 12V Netzteil. Nützlich sind ein USB-C Kabel, ein Computer und eine NMEA Simulationssoftware.

So sieht das Gerät von hinten aus:
![Rückseite](https://obp60-v2-docu.readthedocs.io/de/latest/_images/Bus_Systems.png)

Die 12V Versorgung sind die beiden obersten Pins am rechten Anschluß CN2, also .12V und GNDS.
USB-C wird am ovalen Stecker unterhalb des Anschlusses CN2 angeschlossen.

Das Gerät braucht für den Betrieb die 12V Versorgung. Ein Testbetrieb am PC nur mit USB-C ist nicht möglich.

Nach dem Einschalten der Stromversorgung piept das Gerät und zeigt diesen Startbildschirm:
![Startbildschirm](https://obp60-v2-docu.readthedocs.io/de/latest/_images/OBP60_OBP_Logo_tr.png)
Kurz darauf wird ein QR-Code gezeigt, mit dem man sich per Handy direkt mit dem OBP60 verbinden kann. 
![QR Code](https://obp60-v2-docu.readthedocs.io/de/latest/_images/OBP60_QR_Code_tr.png)

Das Gerät spannt ein eigenes WLAN auf. In der Netzwerkliste z.B. auf Android ist ganz unten ein QR-Symbol, darauf klicken und man kann den QR Code scannen, das Handy loggt sich dann direkt in das WLAN des OBP60 ein. 

## Montage
Um das Gerät zu montieren, soll es aufgeschraubt werden. Die beiden mitgelieferten Sechskantschrauben werden von innen durchgesteckt und dann das Gehäuse wieder verschraubt. 

Der notwendige Ausschnitt für die Kabel und die beiden Schrauben sidn nach dem [Massbild](https://obp60-v2-docu.readthedocs.io/de/latest/_static/files/Drawing_OBP60_V2.pdf) zu erstellen.

Danach dei Dichtung auf die Rückseite des Gehäuses aufsetzen, Schrauben durchstecken und festschrauben.

## Elektrische Anschlüsse
Für den Normalbetrieb müssen mindestens die 12V Versorgung und ein Netzwerk, also NMEA2000 oder NMEA0183, angeschlossen werden. 

- 12V: [+] und [-] an die beiden oberen Anschlüsse am Klemmblock CN2
- NMEA2000: Die Anschlüse CAN H, CAN L und Shield. Die Farbcodierung kann je nach System unterschiedlich sein, bei Raymarine Seatalk NG (das ja auch NMEA2000 ist) ist CAN L Weiss, CAN H blau. Shield ist die Abschirmung des Kabels 
- NMEA0183: Hier werden die Signalleitungen für A und B sowie Shield angeschlossen



