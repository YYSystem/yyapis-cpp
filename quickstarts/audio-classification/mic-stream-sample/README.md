# YYAPIs audio-classification C++ mic-stream-sample サンプル コンソールアプリ

## 事前準備

- [<u>git</u>](https://git-scm.com/downloads) - ソースコード管理システム
- [<u>開発者コンソール</u>](https://api-web.yysystem2021.com) の `yysystem.audioclassification.proto`、 `API キー`
- [<u>サンプルデータのデプロイ</u>](https://github.com/YYSystem/yyapis-docs/wiki/ClassifyStream) - サンプルデータのデプロイまで終わらせてください。
- [<u>gcc g++</u>](https://gcc.gnu.org/) (推奨バージョン 14.2) - brewなどでインストールしてください。
- [<u>Homebrew</u>](https://brew.sh/ja/) - パッケージマネージャ
- [<u>Ninja</u>](https://ninja-build.org/) - brewでインストールしてください。


## サンプルコードのダウンロード

1. git を使用して、任意のディレクトリにサンプルコードをダウンロードします。

```bash
git clone https://github.com/YYSystem/yyapis-cpp.git
```

2. clone したプロジェクトのディレクトリを移動します。

```bash
cd yyapis-cpp/quickstarts/audio-classification/mic-stream-sample
```

3. `mic-stream-sample` の直下に `protos` を作成します。

```bash
mkdir protos
```

4. YYAPIs 開発者コンソールから音響認識 API の proto ファイル(`yysystem.audioclassification.proto`)をダウンロードして、 `protos` ディレクトリを配置します。

```bash
yyapis-cpp/quickstarts/audio-classification/mic-stream-sample/protos/yysytem.proto # ← ここに配置する
```

## API キーと デプロイID の設定
1. `config.cc`の23行目にある YOUR API KEY に、開発者コンソールから取得した `API キー` を貼り付けてください。

```config.cc
static const string apiKey = "YOUR API KEY";
```

2. `config.cc`の27行目にある YOUR ENDPOINT_ID に、開発者コンソールから取得した `デプロイID` を貼り付けてください。

```config.cc
static const string endpoint_id = "YOUR ENDPOINT_ID";
```

## CMake と　vcpkg のインストール

1. brew を使用して、CMake をインストールします。

```bash
cd
brew install cmake
```

2. git を使用して、vcpkg をインストールします。

```bash
cd yyapis-cpp/quickstarts/speech-to-text/mic-stream-sample
git clone https://github.com/microsoft/vcpkg.git
```

3. bootstrap スクリプトを実行します。

```bash
cd vcpkg && ./bootstrap-vcpkg.sh
```

4. `vi ./~zshrc` で .zshrc ファイルを開いて、vcpkg の PATH を通します。

```~/.zshrc
export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

## ライブラリのインストール

1. vcpkg.json を使ってライブラリを一括インストールします。

```bash
cd ../code
vcpkg install
```

## ビルドと実行

1. ビルドと実行をします。
```bash
./start.sh
```

2. マイクが起動して音響分類が開始されます。

3. Ctrl + C で音響分類を終了します。