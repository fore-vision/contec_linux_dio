=====================================================================
=             Linux版デジタル入出力ドライバについて                 =
=                                      API-DIO(LNX)       Ver6.80   =
=                                                  CONTEC Co.,Ltd.  =
=====================================================================

◆ 目次
=======
  はじめに
  製品概要
  制限事項
  注意事項
  インストール方法
  アンインストール方法
  サンプルプログラム
  バージョンアップ履歴


◆ はじめに
===========
  日頃から格別のお引き立てを賜りまして厚く御礼申し上げます。

  API-DIO(LNX)に関する説明を以下に記載します。ヘルプ等に記載
  されていない事項もありますので、ぜひご一読下さい。

◆ 製品概要
===========
・API-DIO(LNX)は、モジュール形式のドライバとシェアードライブラリにより、弊社製
 デジタルI/Oボードを制御するための関数群を提供しています。

・入出力、割り込み、タイマによるトリガ監視といった基本的な機能を提供しています。

・設定プログラム(config)と設定ファイルにより、使用するデバイスを設定して使用します。

・設定プログラムは実行環境へ移行を容易にする設定ファイルとドライバ起動スクリプト、
  停止スクリプトを出力します。

・ドライバに組み込んで実行できるユーザー割り込み処理ソースコードが添付されています。

◆ 制限事項
===========
・製品仕様対象：対応Linuxバージョン

  ディストリビューション        カーネルバージョン
  ----------------------        ------------------
  RedHat Linux
     Enterprise Linux 6         2.6.32-71.el6.i686
     Enterprise Linux 6.7       2.6.32-573.el6.i686
     Enterprise Linux 6.7       2.6.32-573.el6.x86_64
     Enterprise Linux 7.0       3.10.0-123.el7.x86_64
     Enterprise Linux 7.1       3.10.0-229.el7.x86_64
     Enterprise Linux 7.2       3.10.0-327.el7.x86_64
     Enterprise Linux 8.0       4.18.0-80.el8.x86_64

・製品仕様対象外：動作確認のみ

  ディストリビューション        カーネルバージョン
  ----------------------        ------------------
  CentOS
     7.0-1406                   3.10.0-123.el7.x86_64
     7.2-1511                   3.10.0-327.el7.x86_64
  Ubuntu
     14.04.3 LTS                3.19.0-25-generic
     16.04.1 LTS                4.4.0-31-generic (i686)
     16.04.1 LTS                4.4.0-31-generic (x86_64)
     16.04.5 LTS                4.15.0-29-generic (i686)
     18.04.1 LTS                4.15.0-29-generic (x86_64)
     18.04.3 LTS                5.0.0-23-generic (x86_64)
  Fedora
     Fedora 20                  3.11.10-301.fc20.x86_64
     Fedora 24                  4.5.5-300.fc24.i686
     Fedora 24                  4.5.5-300.fc24.x86_64

上記以外のバージョンにつきましては、動作検証を行っておりません。
あらかじめご了承ください。

◆注意事項
==========
  ドライバのインストールおよび設定、起動はroot権限で行う必要があります。

◆インストール方法
==================
  本ドライバは、圧縮ファイルで提供しています。シェルから下記のようにコマンドを
実行してコピー／解凍して使用してください。
ライブラリのインストールパスを/usr/local/lib等に変更したい場合は、あらかじめ
Makefileを見てインストール先を変更しておいてください。

※下記のXXXはドライバのバージョンです。

 # cd
 # mount /dev/cdrom /mnt/cdrom
 # cp /mnt/cdrom/linux/dio/cdioXXX.tgz ./
 # tar xvfz cdioXXX.tgz
 ................
 # cd contec/cdio
 # make
 ................
 # make install
 ................
 # cd config
 # ./config
 ..... 以下設定 .........
 # ./contec_dio_start.sh

 ドライバの起動と停止は contec_dio_start.sh とcontec_dio_stop.sh で行います。
システム起動時に毎回ドライバを起動する場合は、/etc/rc.d/rc.local等に、起動
スクリプトの処理内容を記述してください。

◆アンインストール方法
======================
  アンインストールはアンインストール用シェルスクリプトを実行することで行えます。

 # cd contec/cdio
 # ./cdio_uninstall.sh
 ...................
 #
 詳細はcdio_uninstall.shスクリプトを参照してください。

◆サンプルプログラム
====================
  サンプルプログラムは、各言語ごとに以下のディレクトリで構成されています。
  /<User Directory>/contec/cdio/samples

 ディレクトリの説明

  /inc
    C/C++の関数定義ファイルがあります。ご自分でプログラムを作成される場合、
    これらのファイルをC/C++場合インクルード してご使用下さい。
    また、これらのファイルは編集しないでください。
  /console
    C言語のサンプルプログラムソースコード、及びMakefileが入っています。

◆バージョンアップ履歴	API-DIO(LNX)
=========================================
  Ver6.71 -> Ver6.80 (Web Release)
  --------------------------------
  ・以下新規ディストリビューションに対応
    RedHat Enterprise Linux 8.0

  ・以下のディストリビューションで確認
    Ubuntu 18.04.3 LTS (x86_64)

  ・使用可能デバイスの追加
    追加デバイス：CPS-RRY-4PCC
                  (CPS-BXC200と組み合わせて使用)

  Ver6.60 -> Ver6.71
  --------------------------------
  ・下記デバイスでジェネレーティングをDioDmTransferStop()で停止させた時、
    DioDmSetStopEvent()で設定した転送完了イベントが発生しない場合がある不具合を修正。
      - PIO-32DM(PCI)
      - DIO-32DM-PE
      - DIO-32DM2-PE

  Ver6.51 -> Ver6.60 (2019/10 Web Release)
  --------------------------------
  ・使用可能デバイスの追加
    追加デバイス：DIO-CPS-BXC200 (CPS-BXC200の本体DIO)
                  CPS-DIO-0808L, CPS-DIO-0808BL, CPS-DIO-0808RL,
                  CPS-DI-16L, CPS-DI-16RL, CPS-DO-16L, CPS-DO-16RL
                  (CPS-BXC200と組み合わせて使用)

  Ver6.50 -> Ver6.51 (2019/07 Web Release)
  --------------------------------
  ・DioGet8255Mode関数を実行するとカーネルパニックが起きる不具合を修正

  Ver6.40 -> Ver6.50 (Web Release)
  --------------------------------
  ・以下のディストリビューションで確認
    Ubuntu 16.04.5 LTS (i686)
    Ubuntu 18.04.1 LTS (x86_64)

  Ver6.20 -> Ver6.40 (2018/11 Web Release)
  --------------------------------
  ・使用可能デバイスの追加
    追加デバイス：DIO-1616LN-USB, DIO-1616LN-ETH, DIO-0404LY-WQ
                  CPSN-DI-08L, CPSN-DI-08BL, CPSN-DO-08L, CPSN-DO-08BL

  ・USB及び、PCI / PCI Express バスデバイスの組み合わせにおいて
    PCI / PCI Express バスデバイスに対してDioInit()を実行すると10003エラーになる不具合を修正

  ・USBを採用した同一機種を複数台使用した場合に、正常に制御できない場合がある不具合を修正

  Ver6.10 -> Ver6.20 (2017/12 Web Release)
  --------------------------------
  ・使用可能デバイスの追加
    追加デバイス：DIO-24DY-USB、DIO-0808LY-USB, DIO-0808TY-USB
                  DI-16TY-USB, DO-16TY-USB, DIO-1616LX-USB
                  DIO-3232LX-USB, DIO-6464LX-USB, DIO-128SLX-USB
                  DIO-1616RYX-USB, DIO-1616BX-USB, DIO-48DX-USB
                  RRY-16CX-USB, DIO-0808RN-USB, DIO-1616HN-USB

  Ver6.00 -> Ver6.10 (Web Release)
  --------------------------------
  ・以下のディストリビューションで確認
    Ubuntu 16.04.1 LTS
    Fedora 24

  Ver5.20 -> Ver6.00 (Web Release)
  --------------------------------
  ・ドライバソースコードの部分公開

  ・以下のディストリビューションで確認
    CentOS 7.0-1406
    CentOS 7.2-1511
    Ubuntu 14.04.3 LTS
    Fedora 20

  Ver4.50 -> Ver5.20 (Web Release)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DIO-1616L-LPE, DIO-1616B-LPE

  ・新規ディストリビューションに対応
    Red Hat Enterprise Linux 6
    Red Hat Enterprise Linux 6.7

  ・以下ディストリビューションは、64ビット版に対応
    Red Hat Enterprise Linux 6.7
    Red Hat Enterprise Linux 7
    Red Hat Enterprise Linux 7.1
    Red Hat Enterprise Linux 7.2

  ・以下ディストリビューションサポートを終了
    RedHat Linux 7.1
    RedHat Linux 7.2
    RedHat Linux 7.3
    RedHat Linux 8.0
    RedHat Linux 9
    Red Hat Professional Workstation
    Red Hat Enterprise Linux 4
    Red Hat Enterprise Linux 5
    TurboLinux   7.0
    TurboLinux   8
    TurboLinux   10 Server
    TurboLinux   11 Server


  Ver4.40->Ver4.50 (Ver Mar.2011)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DI-128RL-PCI, DO-128RL-PCI

  Ver4.30 -> Ver4.40 (Web Release)
  --------------------------------
  ・新規ディストリビューションに対応(SMP対応含む)
  ・Red Hat Enterprise Linux 5
  ・Turbo Linux 11 server

  Ver4.21->Ver4.30 (Ver Jan.2009)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DIO-32DM-PE
  ・カードバスデバイスで正常にIOできない場合がある不具合を修正

  Ver4.20->Ver4.21 (web release)
  --------------------------------
  ・バスマスタの転送完了通知が実際よりも多く通知されることがある不具合を修正
  ・configでデバイスの自動検出を行うとセグメンテーションフォールトを起こす場合がある不具合を修正

  Ver4.10->Ver4.20 (Ver Apr.2008)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DI-64T-PE, DO-64T-PE, DI-32T-PE, DO-32T-PE
                DIO-48D-PE, DIO-96D-LPE, DI-128T-PE, DO-128T-PE

  Ver4.00->Ver4.10 (Ver Jan.2008)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：RRY-32-PE, RRY-16C-PE, DIO-1616RY-PE
  ・DioDmSetBuff関数にてカーネルパニックを起こすことがある不具合を修正

  Ver3.51->Ver4.00 (Ver Oct.2007)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DI-32B-PE, DO-32B-PE, DIO-1616H-PE, DIO-3232H-PE
                DIO-1616RL-PE, DIO-3232RL-PE

  Ver3.50->Ver3.51 (web release)
  --------------------------------
  ・Ver3.50のドライバファイル一部破損のため入れ替え

  Ver3.40->Ver3.50 (Ver Jun.2007)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DIO-96D2-LPCI

  Ver3.30->Ver3.40 (Ver Feb.2007)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DIO-3232B-PE, DIO-3232F-PE, DIO-1616B-PE, DIO-1616TB-PE
                DI-64L-PE, DI-32L-PE, DO-64L-PE, DO-32L-PE
                DIO-6464T2-PCI, DI-128T2-PCI, DO-128T2-PCI,DI-32T2-PCI
                DO-32T2-PCI, DI-64T2-PCI, DO-64T2-PCI, DIO-48D2-PCI

  Ver3.21->Ver3.30 (Ver Nov.2006)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DIO-3232T-PE, DIO-1616T-PE, DIO-6464T-PE, DIO-6464L-PE, DI-128L-PE, DO-128L-PE

  Ver3.20->Ver3.21
  --------------------------------
  ・PIO-32DM(PCI)をカーネル2.4系で1GByte以上のメモリを搭載したPCで使用すると、
    DioDmSetBuff関数にてカーネルパニックを起こすことがある不具合を修正
    (カーネルバージョン2.4.13以上でのみ有効)

  Ver3.10->Ver3.20 (Ver Apr.2006)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：DIO-48D-LPE, DIO-1616T-LPE, DIO-3232L-PE, DIO-1616L-PE

  Ver3.00->Ver3.10 (Ver Nov.2005)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-32/32F(PCI)H, PIO-16/16T(PCI)H, PIO-16/16TB(PCI)H

  Ver2.20->Ver3.00 (Ver Aug.2005)
  --------------------------------
  ・kernel 2.6.xx に対応
  ・Red Hat Enterprise Linux 4 に対応
  ・Turbo Linux 10 Server に対応

  Ver2.10->Ver2.20 (Ver Jun.2005)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-64/64L(PCI)H, PI-128L(PCI)H, PO-128L(PCI)H
  ・関数追加: DioDmGetWritePointerUserBuf()
  ・infiniteサンプルでDioDmGetWritePointerUserBuf()を使用するように変更。
  ・バスマスタ転送カウントが24bitから25bitに桁上がりするときに
    正常にカウントされない不具合を修正。

  Ver.2.00 -> Ver.2.10 (Ver Apr.2005)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-32/32T(PCI)H, PIO-32/32B(PCI)V, RRY-16C(PCI)H,
                RRY-32(PCI)H, PIO-48D(LPCI)H

  Ver.1.50 -> Ver.2.00 (Ver Jan.2005)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-48D(CB)H

  Ver.1.40 -> Ver.1.50 (Ver Oct.2004)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-32/32H(PCI)H, PIO-16/16H(PCI)H, PIO-32/32RL(PCI)H
                PIO-16/16RL(PCI)H, PIO-16/16L(CB)H

  Ver.1.30 -> Ver.1.40 (Web Release)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-32DM(PCI)
  ・PIO-32DM(PCI)用の新関数を追加しました。
  ・Red Hat Professional Workstation に対応
  ・DioExit実行時、正常に終了処理が行われず、アプリケーションを終了しなければ
    同一ボードを利用できるプロセス数が減っていく不具合を修正。

  Ver.1.20 -> Ver.1.30 (Web Release)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-48D(PCI)
  ・PIO-48D(PCI)用の新関数を追加しました。
  ・トリガ使用時にほかのボードを扱っているプロセスが存在すると
    そのボードを扱っているプロセスがすべて終了したときにトリガが
    停止してしまう不具合を修正。
  ・割り込み使用時、1kHZ程度以上のパルスを入れ続けると割り込みが
    入らなくなることがある不具合を修正。

  Ver.1.10 -> Ver.1.20 (Ver Nov.2003)
  --------------------------------
  ・使用可能ボードの追加
    追加ボード：PIO-16/16B(PCI)H,PI-32B(PCI)H,PO-32B(PCI)H,
                PIO-16/16L(LPCI)H,PIO-16/16B(LPCI)H,
                PIO-16/16T(LPCI)H

  Ver.1.02 -> Ver.1.10 (Web Release)
  --------------------------------
  ・RedHat Linux 8.0 RedHat Linux 9 に対応

  Ver.1.01 -> Ver.1.02 (Web Release)
  --------------------------------
  ・PIO-XX(PCI)H（PIO-32/32B(PCI)Hを除く）、PI-XX(PCI)H、PIO-16/16RY(PCI)で
    割り込みが正常に通知されない不具合を修正

  Ver.1.00 -> Ver.1.01 (Ver. Dec.2002)
  --------------------------------
  ・割り込み使用時にCPU使用率が100%になっていたのを修正
  ・Help修正

  Ver.1.00 (Web Release)
  --------------------------------
  ・ファーストリリース

