# M5StickS3 と AWS IoT Core の MQTT 疎通確認

M5StickS3 から10秒ごとにテレメトリを送信し、AWS IoT Core の MQTT テストクライアントから受け取ったコマンドを画面と `status` トピックへ表示する、Arduino IDE 用の検証コードです。

## 必要なライブラリ

- `M5Unified`
- `M5GFX`
- `PubSubClient`

Arduino IDE のボードには `M5StickS3` を選択します。

## AWS IoT Core のファイルを同期する

1. `m5sticks3-device-config` を使って、M5StickS3へWi-Fi設定を保存します。
2. `data/aws-iot/README.md` の名前に従い、エンドポイント・デバイス証明書・秘密鍵・Amazon Root CA 1 を `data/aws-iot/` へ置きます。
3. Arduino IDE のシリアルモニタを閉じ、`./tools/upload-littlefs.sh` を実行して、フォルダ全体をM5StickS3のLittleFSへ書き込みます。USB ポートは自動で選びます。
4. Arduino IDE でスケッチ本体を書き込みます。

`data/aws-iot/` の実ファイルは `.gitignore` で除外しています。証明書と秘密鍵を Git に追加しないでください。Wi-FiのSSIDとパスワードは、このスケッチではなくM5StickS3のNVSに保存します。

複数の USB ポートが見つかった場合だけ、Arduino IDE で確認したポート名を指定して実行します。

```bash
./tools/upload-littlefs.sh /dev/cu.usbmodemXXXX
```

## MQTT テストクライアントで確認する

1. `m5sticks3-iot-demo/#` を購読します。
2. `telemetry` が10秒ごとに届くことを確認します。
3. `m5sticks3-iot-demo/command` に `{ "message": "hello" }` を公開します。
4. M5StickS3 の画面と `status` トピックでコマンド受信を確認します。

この記事用のポリシーでは、`iot:Connect`、`iot:Publish`、`iot:Subscribe`、`iot:Receive` をトピック単位で許可します。
