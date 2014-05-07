{
	"targets": [
		{
			"target_name": "pru",
			"sources": [
				"src/pru.cpp",
				"prussdrv/prussdrv.c",
			],
			"include_dirs": [
				"prussdrv"
			],
			"cflags": [
				"-fpermissive"
			]
		}
	]
}
