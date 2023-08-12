# picorto
Raspberry Pico W Irrigazione Orto

Progetto di irrigazione automatica di un orto

Componenti:
Raspberry Pico W Microcontrollore con modulo Wifi
INA219 sensore di tensione e corrente
DS3231 RTC con sensore di temperatura integrato
DC-DC step.down converter ingresso 12 Volt Uscita 5 Volt
Rele 5 VDC optoisolati Maximum load: AC 250V/10A, DC 30V/10A. 
Proto Board
Kit Pannello solare 20 Watt con regolatore di carica
Batteria Piombo da moto 12 Volt 9 Ampere
Contenitore Stagno
Passacavi
Pompa ad immersione 12 Volt 16 Litri al minuto
Elettrovalvola a Solenoide DC 12 Volt in Ottone Normalmente Chiusa per Acqua N/C 
Cisterna acqua da 1000 Litri
Interruttore galleggiante verticale con sensore di controllo del livello dell'acqua
Interruttore a Pulsante a Scatto in Metallo con Luce LED Blu DC 12V impermeabile
Morsetti a leva
Tubo in Polietilene Ala Gocciolante Ø 16 Passo 20 a Goccia Irrigazione Orto
TUBO IRRIGAZIONE DIAM 16 MM - IN POLIETILENE PN6
Kit Raccordi Irrigazione a Goccia
picchetti di Ancoraggio per Il Fissaggio a Terra per l'irrigazione
Cavo Rosso Nero in silicone 18 AWG
Cavetti 6 colori 24 AWG per realizzare le connessioni sulla proto board

Programmazione:
Ambiente di sviluppo C++ Arduino IDE 2.1.1

Librerie Utilizzate:
AsyncWebServer_RP2040W
LittleFS
Adafruit_INA219
RTClib

Wifi abilitato in modalità access point default SSID=PICORTO password=cambiami
Indirizzo IP Web Server http://192.168.42.1
Orario di erogazione unico giornaliero preimpostato alle 20:30 per 15 minuti
SSID, password, orario e tempo erogazione modaficabili da pagina Web e salvati in Flash nel file config.bin
Orario RTC impostabile dall'utente via pagina Web non gestisce il cambio ora legale
Se il livello dell'acqua nella cisterna è sotto i 10 cm dell'interruttore galleggiante, l'erogazione non viene effettuata
Salvataggio in flash delle erogazioni nel file erogazioni.log con visualizzazione storico su pagina Web

Aspetti da migliorare/sviluppare
Pagine Web integrare JavaScript nella pagina di stato per ottimizzare l'auto refresh
Migliorare il codice CSS per lo stile dei form di input
