TARGET = OpenCheat
TYPE = ps-exe

SRCS = \
main.cpp \
cheat_to_code.cc \
drawing/font.cpp \
font.raw

LDFLAGS = -Wl,--defsym=TLOAD_ADDR=0x80040000

include third_party/nugget/psyqo/psyqo.mk

%.o: %.raw
	$(PREFIX)-objcopy -I binary --set-section-alignment .data=4 --rename-section .data=.rodata,alloc,load,readonly,data,contents -O $(FORMAT) -B mips $< $@