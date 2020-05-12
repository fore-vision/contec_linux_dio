{
	"targets":[
		{
			"target_name": "contecgpio",
			"cflags!": [ "-fno-exceptions" ],
			"cflags_cc!": [ "-fno-exceptions" ,'-std=c++14'],
			"sources" : [ "contecDio.cpp"],
			"include_dirs" : [
				"<!@(node -p \"require('node-addon-api').include\")"
			],
			"libraries": [
				"-lcdio"],
			'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
		}
	]
}
