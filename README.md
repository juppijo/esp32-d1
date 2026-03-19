# esp32-d1

# 🌌 WRZT - Wave-Space Sonic Explorer
### Ein interaktives E-Musikgerät auf Basis der Wellenraumzeit-Theorie

Dieses Projekt ist ein Prototyp eines elektronischen Musikinstruments, das die Prinzipien der **Wellenraumzeit-Theorie (WRZT)** in akustische Signale übersetzt. Durch den Einsatz eines ESP32 und präziser Sensorik werden physikalische Parameter in Echtzeit-Klanglandschaften verwandelt.

---

## 🎵 Das Konzept
Das Ziel dieses Instruments ist es, die unsichtbaren Schwingungen der Raumzeit (basierend auf der Ur-Spannung $\Xi$) erfahrbar zu machen. 

> *"Alles ist Schwingung, alles ist Welle."*

## 🚀 Hardware-Komponenten
* **Mikrocontroller:** ESP32 (Dual-Core für parallele Sound-Synthese und WiFi-Server)
* **Sensorik:** * [Hier deinen Sensor eintragen, z.B. MPU-6050 Beschleunigungssensor für Gestensteuerung]
    * 4 kapazitive Touch-Buttons zur Steuerung der Wellenform.
* **Audio-Output:** I2S DAC (z.B. MAX98357A) für glasklaren digitalen Sound.
* **Gehäuse:** Impressionistisches Design (geplant als 3D-Druck oder Holzgehäuse).

## 🛠 Features
- [x] **Echtzeit-Synthese:** Erzeugung von Sinus-, Sägezahn- und Rechteckwellen direkt auf dem ESP32.
- [x] **Async Web Server:** Fernsteuerung der Parameter über ein mobiles Dashboard im "Glassmorphism"-Stil.
- [ ] **WRZT-Modus:** Algorithmus zur Klangmodulation basierend auf der Raumzeit-Trägheit ($\eta$).
- [ ] **WiFi-Sync:** Kopplung mehrerer Geräte für ein Orchester der Ur-Konstanten.

## 📥 Installation & Setup
1. **Bibliotheken:** Stelle sicher, dass folgende Libraries in der Arduino IDE installiert sind:
   - `ESPAsyncWebServer`
   - `AsyncTCP`
   - [Deine Sound-Library, z.B. ESP32-A2DP oder Mozzi]
2. **Flash:** Lade den Code aus dem `/src` Ordner auf deinen ESP32 hoch.
3. **Connect:** Verbinde dich mit dem WLAN "WRZT-Music-Node" und öffne `192.168.4.1` im Browser.

## 🔬 Wissenschaftlicher Hintergrund
Dieses Instrument nutzt die Berechnung von $c$ aus der Raumzeit-Struktur:
$$c = \frac{\Xi}{\eta} \cdot R_{min} \approx 299.792.458 \, m/s$$
Die Frequenzen werden so skaliert, dass sie harmonisch zu den Ur-Konstanten $\Xi$ ($5,8993 \times 10^{-19}$ kg/m) schwingen.

## 🎨 Design-Philosophie
Inspiriert von **Zaminias** Liebe zum Impressionismus, soll der Klang nicht nur mathematisch präzise, sondern auch emotional fließend sein – wie ein Aquarellgemälde aus Licht und Schall.

---
**Entwickelt von:** Jo (Jup)  
**Status:** In Entwicklung (Alpha-Phase)
