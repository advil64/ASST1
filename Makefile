all: fileCompressor

fileCompressor: fileCompressor.c
	gcc fileCompressor.c -o fileCompressor

clean:
	rm -rf fileCompressor