all: fileCompressor

ASST1: fileCompressor.c
	gcc fileCompressor

clean:
	rm -rf fileCompressor