ARCH ?= x86_64
CFLAGS += $(foreach d, $(INCLUDE), -I$d)
CFLAGS += -fno-stack-protector -mno-red-zone -fshort-wchar -fpic -ffreestanding -Os
EFI_INC = /usr/include/efi
EFI_ARCH_INC = $(EFI_INC)/$(ARCH)
INCLUDE = $(EFI_INC) $(EFI_ARCH_INC)

LDLIBS = -lefi
LDDIRS = /usr/lib
LDEFISYS=10
LDFLAGS += $(foreach d, $(LDDIRS), -L$d) $(LDLIBS)
LDFLAGS += -nostdlib -Wl,-Bsymbolic -Wl,--subsystem=$(LDEFISYS)

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = paradox.efi

QEMU_EFI = -drive if=pflash,format=raw,readonly=on,file="c:/Program Files/qemu/share/edk2-x86_64-code.fd"
EFI_FS = efi_root

.PHONY: all
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET) $(EFI_FS)

test: $(TARGET)
	mkdir -p $(EFI_FS)/EFI/BOOT
	cp $(^) $(EFI_FS)/EFI/BOOT/BOOTX64.EFI

	qemu-system-x86_64 -drive file=fat:rw:$(EFI_FS),format=raw \
    	$(QEMU_EFI)
