# YYAPIs speech-to-text C++ FileStreamSample サンプル コンソールアプリ

## 事前準備

- [<u>git</u>](https://git-scm.com/downloads) - ソースコード管理システム
- [<u>開発者コンソール</u>](https://api-web.yysystem2021.com) の `yysystem.proto`、 `API キー`
- [<u>gcc g++</u>](https://gcc.gnu.org/) (推奨バージョン 14.2) - brewなどでインストールしてください
- [<u>Homebrew</u>](https://brew.sh/ja/) - パッケージマネージャ


## サンプルコードのダウンロード

1. git を使用して、任意のディレクトリにサンプルコードをダウンロードします。

```bash
git clone https://github.com/YYSystem/yyapis-cpp.git
```

2. clone したプロジェクトのディレクトリを移動します。

```bash
cd yyapis-cpp/quickstarts/speech-to-text/FileStreamSample
```

3. `FileStreamSample` の直下に `protos` を作成します。

```bash
mkdir protos
```

4. YYAPIs 開発者コンソールから音響認識 API の proto ファイル(`yysystem.proto`)をダウンロードして、 `protos` ディレクトリを配置します。

```bash
yyapis-cpp/quickstarts/speech-to-text/FileStreamSample/protos/yysytem.proto # ← ここに配置する
```

## API キーの設定
1. `config.cc`の18行目にある YOUR API KEY に、開発者コンソールから取得した `API キー` を貼り付けてください。

```config.cc
static const string apiKey = "YOUR API KEY";
```

## vcpkg のインストール

1. git を使用して、vcpkg をインストールします。

```bash
cd yyapis-cpp/quickstarts/speech-to-text/FileStreamSample
git clone https://github.com/microsoft/vcpkg.git
```

2. bootstrap スクリプトを実行します。

```bash
cd vcpkg && ./bootstrap-vcpkg.sh
```

3. `vi ./~zshrc` で .zshrc ファイルを開いて、vcpkg の PATH を通します。

```~/.zshrc
export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

```bash
source ~/.zshrc
```

## Cmake と Ninja をインストール

1. brew を使用して、CMake と Ninja をインストールします。

```bash
cd
brew install cmake ninja
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

2. マイクが起動して音声認識が開始されます。

3. Ctrl + C で音声認識を終了します。