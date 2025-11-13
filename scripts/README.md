# Hilfsskripte

## `install-arduino-cli.sh`
Lädt die aktuelle Arduino-CLI für die jeweilige Plattform herunter und installiert sie in den Ordner `.arduino-cli/bin` im Repository (oder in den durch `ARDUINO_CLI_HOME` gesetzten Pfad).

```bash
./scripts/install-arduino-cli.sh
export PATH="$(pwd)/.arduino-cli/bin:$PATH"
arduino-cli version
```

## `docker-arduino-cli.sh`
Führt Befehle innerhalb des offiziellen Docker-Images `arduino/arduino-cli` aus. Docker muss auf dem Host verfügbar sein.

```bash
./scripts/docker-arduino-cli.sh compile --fqbn esp32:esp32:esp32 examples/Basic
```
