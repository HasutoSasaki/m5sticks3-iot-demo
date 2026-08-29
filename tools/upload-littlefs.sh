#!/bin/zsh
set -euo pipefail
umask 077

if (( $# > 1 )); then
  print -u2 "Usage: $0 [/dev/cu.usbmodemXXXX]"
  exit 1
fi

if (( $# == 1 )); then
  readonly port="$1"
else
  detected_ports=(/dev/cu.usbmodem*(N))

  if (( ${#detected_ports[@]} == 0 )); then
    print -u2 "M5StickS3 の USB ポートが見つかりません。Arduino IDE でポートを確認してください。"
    exit 1
  fi

  if (( ${#detected_ports[@]} > 1 )); then
    print -u2 "複数の USB ポートが見つかりました。Arduino IDE でポートを確認して指定してください。"
    print -u2 "Usage: $0 /dev/cu.usbmodemXXXX"
    exit 1
  fi

  readonly port="${detected_ports[1]}"
fi

if [[ ! -e "${port}" ]]; then
  print -u2 "USB ポートが見つかりません: ${port}"
  exit 1
fi

readonly script_dir="${0:A:h}"
readonly sketch_dir="${script_dir:h}"
readonly data_dir="${sketch_dir}/data"
readonly arduino15_dir="${ARDUINO15_DIR:-${HOME}/Library/Arduino15}"
readonly tools_dir="${arduino15_dir}/packages/m5stack/tools"
readonly mklittlefs="$(find "${tools_dir}/mklittlefs" -type f -name mklittlefs -print -quit)"
readonly esptool="$(find "${tools_dir}/esptool_py" -type f -name esptool -print -quit)"
readonly temporary_dir="$(mktemp -d "${TMPDIR:-/tmp}/m5sticks3-littlefs.XXXXXX")"
readonly image_path="${temporary_dir}/aws-iot.littlefs.bin"
trap 'rm -rf "${temporary_dir}"' EXIT

if [[ ! -d "${data_dir}" || ! -x "${mklittlefs}" || ! -x "${esptool}" ]]; then
  print -u2 "M5Stack ESP32 Board 3.x と data/ が必要です。"
  exit 1
fi

for required_file in endpoint.txt amazon-root-ca.pem device-certificate.pem private-key.pem; do
  if [[ ! -s "${data_dir}/aws-iot/${required_file}" ]]; then
    print -u2 "Missing or empty: data/aws-iot/${required_file}"
    exit 1
  fi
done

print "LittleFS を ${port} へ書き込みます。"
"${mklittlefs}" -c "${data_dir}" -p 256 -b 4096 -s 0x180000 "${image_path}"
"${esptool}" --chip esp32s3 --port "${port}" --baud 921600 --before default-reset --after hard-reset \
  write-flash -z --flash-mode keep --flash-freq keep --flash-size keep 0x670000 "${image_path}"
print "LittleFS への書き込みが完了しました。"
