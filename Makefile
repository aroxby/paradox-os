ARCH ?= x86_64
CFLAGS += $(foreach d, $(INCLUDE), -I$d)
CFLAGS += -fno-stack-protector -mno-red-zone -fshort-wchar -fpic -ffreestanding -Os
EFI_INC = /usr/include/efi
EFI_ARCH_INC = $(EFI_INC)/$(ARCH)
INCLUDE = $(EFI_INC) $(EFI_ARCH_INC)

EFI_LIBDIR ?= /usr/lib
EFI_CRT = $(EFI_LIBDIR)/crt0-efi-$(ARCH).o
EFI_LDSCRIPT = $(EFI_LIBDIR)/elf_$(ARCH)_efi.lds
EFI_LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDSCRIPT) -shared -Bsymbolic --no-undefined
EFI_LIBS = -lgnuefi -lefi
OBJCOPY ?= objcopy
OBJCOPY_SECTIONS = .text .sdata .data .rodata .dynamic .dynsym .rel .rela .reloc
OBJCOPYFLAGS = $(foreach s, $(OBJCOPY_SECTIONS), -j $(s)) -O efi-app-$(ARCH)

SRCS = $(wildcard *.c) $(wildcard *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCS)))
TARGET = paradox.efi
EFI_ELF = $(TARGET:.efi=.so)

# TODO: create target for EFI_FS
EFI_FS = efi_root

.PHONY: all
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(EFI_ELF): $(OBJS)
	$(LD) $(EFI_LDFLAGS) -L$(EFI_LIBDIR) $(EFI_CRT) $(OBJS) $(EFI_LIBS) -o $@

$(TARGET): $(EFI_ELF)
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

.PHONY: clean
clean:
	rm -rf $(OBJS) $(EFI_ELF) $(TARGET) $(EFI_FS)

test: $(TARGET)
	mkdir -p $(EFI_FS)/EFI/BOOT
	cp $(^) $(EFI_FS)/EFI/BOOT/BOOTX64.EFI

	qemu-system-x86_64 -drive file=fat:rw:$(EFI_FS),format=raw \
		-machine graphics=off \
		-nographic \
		-bios OVMF.fd \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		|| true
