DIST=dist^\
TMP=$(DIST)tmp^\
CFLAGS=/nologo /Fe$(DIST) /Fo$(TMP) /GL /Ox /W4

main: $(DIST)widexe.exe

$(DIST)widexe.exe: widexe.c
	-md $(DIST) $(TMP)
	$(CC) $(CFLAGS) widexe.c

clean:
	-rd /s /q dist
