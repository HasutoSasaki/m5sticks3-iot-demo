# AWS IoT Core のローカルファイル

このディレクトリの実ファイルは Git 管理外です。次の4ファイルを置きます。

| ファイル名 | 入れる内容 |
| --- | --- |
| `endpoint.txt` | AWS IoT Core「接続」→「ドメイン設定」のデータエンドポイントを1行で記載 |
| `amazon-root-ca.pem` | Amazon Root CA 1 の PEM ファイル |
| `device-certificate.pem` | 接続キットの `m5sticks3-iot-demo.cert.pem` |
| `private-key.pem` | 接続キットの `m5sticks3-iot-demo.private.key` |

PEM ファイルは内容を変更せず、そのままコピーします。`private-key.pem` は共有・送信・Gitへの追加をしないでください。

ファイルをそろえたら、スケッチのディレクトリで次を実行します。

```bash
./tools/upload-littlefs.sh /dev/cu.usbmodem2101
```

この操作は LittleFS の領域全体を書き換えます。
