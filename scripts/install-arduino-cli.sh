#!/usr/bin/env bash
set -euo pipefail

if ! command -v curl >/dev/null 2>&1; then
  echo "Fehler: curl wird zum Herunterladen von arduino-cli benötigt." >&2
  exit 1
fi

ARCHIVE=""
OS=$(uname -s)
ARCH=$(uname -m)
case "${OS}" in
  Linux)
    case "${ARCH}" in
      x86_64|amd64)
        ARCHIVE="arduino-cli_latest_Linux_64bit.tar.gz"
        ;;
      armv7l|armv6l)
        ARCHIVE="arduino-cli_latest_Linux_ARMv7.tar.gz"
        ;;
      aarch64|arm64)
        ARCHIVE="arduino-cli_latest_Linux_ARM64.tar.gz"
        ;;
    esac
    ;;
  Darwin)
    case "${ARCH}" in
      x86_64)
        ARCHIVE="arduino-cli_latest_macOS_64bit.tar.gz"
        ;;
      arm64)
        ARCHIVE="arduino-cli_latest_macOS_arm64.tar.gz"
        ;;
    esac
    ;;
  *)
    ;;
esac

if [[ -z "${ARCHIVE}" ]]; then
  echo "Fehler: Keine passende arduino-cli Binary für ${OS}/${ARCH} bekannt." >&2
  exit 1
fi

INSTALL_ROOT=${ARDUINO_CLI_HOME:-"${PWD}/.arduino-cli"}
BIN_DIR="${INSTALL_ROOT}/bin"
mkdir -p "${BIN_DIR}"

TMPDIR=$(mktemp -d)
cleanup() {
  rm -rf "${TMPDIR}"
}
trap cleanup EXIT

ARCHIVE_PATH="${TMPDIR}/${ARCHIVE}"
URL="https://downloads.arduino.cc/arduino-cli/${ARCHIVE}"

echo "Lade ${URL} ..."
curl -fsSL "${URL}" -o "${ARCHIVE_PATH}"

tar -xzf "${ARCHIVE_PATH}" -C "${TMPDIR}"

# extrahiertes Verzeichnis enthält die Binärdatei "arduino-cli"
if [[ ! -f "${TMPDIR}/arduino-cli" ]]; then
  echo "Fehler: Konnte arduino-cli im Archiv nicht finden." >&2
  exit 1
fi

mv "${TMPDIR}/arduino-cli" "${BIN_DIR}/"
chmod +x "${BIN_DIR}/arduino-cli"

cat <<MSG
arduino-cli wurde nach ${BIN_DIR} installiert.
Füge den folgenden Eintrag zu deiner PATH-Variable hinzu oder source ihn direkt:

    export PATH="${BIN_DIR}:\$PATH"

Du kannst jetzt '${BIN_DIR}/arduino-cli version' ausführen, um die Installation zu prüfen.
MSG
