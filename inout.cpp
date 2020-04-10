//================================================================
//================================================================
// CONTEC Linux DIO
// 入出力サンプル
//               						    CONTEC Co.,Ltd.
//											Ver1.00
//================================================================
//================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdio.h"

//================================================================
// コマンド定義
//================================================================
#define COMMAND_ERROR 0		// エラー
#define COMMAND_INP_PORT 1	// 1ポート入力
#define COMMAND_INP_BIT 2	// 1ビット入力
#define COMMAND_OUT_PORT 3	// 1ポート出力
#define COMMAND_OUT_BIT 4	// 1ビット出力
#define COMMAND_ECHO_PORT 5 // 1ポートエコーバック
#define COMMAND_ECHO_BIT 6	// 1ビットエコーバック
#define COMMAND_QUIT 7		// 終了

//================================================================
// main関数
//================================================================
int main(int argc, char *argv[])
{
	char buf[256];
	unsigned char data;
	long lret;
	short id;
	//-------------------------------------
	// 初期化
	//-------------------------------------

	lret = DioInit("DIO000", &id);
	if (lret != DIO_ERR_SUCCESS)
	{
		DioGetErrorString(lret, buf);
		fprintf(stderr, "%s=%ld:%s\n", "DioInit", lret, buf);
		return -1;
	}
	lret = DioInpBit(id, 0, &data);
	if (lret == DIO_ERR_SUCCESS)
	{
		if (data == 1)
			printf("1\n");
		else
			printf("0\n");
	}
	else
	{
		DioGetErrorString(lret, buf);
		fprintf(stderr, "%s=%ld:%s\n", "DioInpBit", lret, buf);
	}

	if (argc > 1)
	{
		if (0 == strcmp(argv[1], "1"))
		{
			data = 1;
		}
		else
			data = 0;
	}
	lret = DioOutBit(id, 0, data);
	if (lret != DIO_ERR_SUCCESS)
	{
		DioGetErrorString(lret, buf);
		fprintf(stderr, "%s=%ld:%s\n", "DioOutBit", lret, buf);
	}
	//-------------------------------------
	// 終了
	//-------------------------------------
	lret = DioExit(id);
	if (lret != DIO_ERR_SUCCESS)
	{
		DioGetErrorString(lret, buf);
		fprintf(stderr, "%s=%ld:%s\n", "DioExit", lret, buf);
		return -1;
	}
	return 0;
}
