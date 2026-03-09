# Range Hood Noise Detector

ESP32 + INMP441 を使ったレンジフード騒音検出器。騒音レベルが一定以上継続した場合に IFTTT 経由で iPhone へ通知を送ります。

## ハードウェア構成

### 必要部品

| 部品 | 型番 |
|---|---|
| マイコン | ESP32 DevKit (ESP32-WROOM-32) |
| マイク | INMP441 (I2S デジタルマイク) |
| ボタン × 2 | タクトスイッチ |

### 配線表

| INMP441 ピン | ESP32 ピン | 説明 |
|---|---|---|
| VDD | 3.3V | 電源 |
| GND | GND | グランド |
| SCK | GPIO 26 | シリアルクロック |
| WS | GPIO 25 | ワードセレクト (L/R クロック) |
| SD | GPIO 34 | シリアルデータ (入力専用ピン) |
| L/R | GND | 左チャンネル選択 |

| ボタン | ESP32 ピン | 説明 |
|---|---|---|
| 電源ボタン | GPIO 0 (BOOT) | 長押し 3 秒でシャットダウン |
| Wi-Fi リセットボタン | GPIO 35 | 起動時に押しながら電源ON → Wi-Fi 設定消去 |

## ソフトウェア構成

```
src/
  main.cpp          メインループ・オーケストレーション
  audio.cpp         I2S + Goertzel フィルタによる騒音検出
  wifi_manager.cpp  Wi-Fi 接続管理 + Captive Portal
  notification.cpp  IFTTT Webhook 通知
  storage.cpp       NVS (不揮発メモリ) 設定保存
  power.cpp         電源ボタン・スリープ管理
include/
  config.h          全定数・ピン定義
  *.h               各モジュールのヘッダ
data/
  index.html        Wi-Fi 設定用 Captive Portal UI
```

## ビルドと書き込み

### 前提条件

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) または PlatformIO IDE (VSCode 拡張) をインストール済みであること

### ビルド

```bash
pio run
```

### ファームウェア書き込み

```bash
pio run --target upload
```

### LittleFS ファイルシステム書き込み (Captive Portal UI)

```bash
pio run --target uploadfs
```

### シリアルモニタ

```bash
pio device monitor
```

## 初回セットアップ手順

1. ファームウェアと LittleFS イメージを書き込む
2. ESP32 を起動する
3. スマートフォンの Wi-Fi 設定で `RangeHoodSetup` に接続する
4. 自動的に設定ページが開く（開かない場合は `http://192.168.4.1/` を開く）
5. Wi-Fi SSID・パスワード、IFTTT Webhooks キー、閾値を入力して保存
6. デバイスが自動的に再起動し、指定した Wi-Fi に接続する

## IFTTT 設定

1. [IFTTT](https://ifttt.com/) アカウントを作成する
2. **Webhooks** サービスを有効化し、API キーを取得する
   - Settings → Documentation に記載
3. **New Applet** を作成する
   - If: Webhooks → "Receive a web request" → Event name: `range_hood_noise`
   - Then: Notifications → "Send a rich notification from the IFTTT app"
4. 取得した API キーを Captive Portal の設定画面に入力する

## 動作仕様

| 項目 | 値 |
|---|---|
| サンプリング周波数 | 44,100 Hz |
| 計測時間 | 3 秒 |
| 計測間隔 | 10 秒 (ライトスリープ) |
| 騒音判定周波数 | 100 Hz 帯 (Goertzel 法) |
| デフォルト閾値 | 65 dB |
| 連続超過回数 | 3 回 → 通知送信 |
| 通知クールダウン | 60 秒 |

## Wi-Fi 設定リセット

電源 OFF 状態からリセットボタン (GPIO 35) を押しながら電源を入れると、保存済みの Wi-Fi 設定が消去され Captive Portal が起動します。

## ライセンス

MIT
