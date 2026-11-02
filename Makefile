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

SRC_DIRS = src/loader
SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c) $(wildcard $(dir)/*.cpp))
BUILD_DIR = build
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCS)))
OBJS := $(addprefix $(BUILD_DIR)/,$(OBJS))
TARGET = $(BUILD_DIR)/paradox.efi
EFI_ELF = $(TARGET:.efi=.so)

EFI_FS = efi_root
EFI_FS_BOOT = $(EFI_FS)/EFI/BOOT
EFI_FS_EXE = $(EFI_FS_BOOT)/BOOTX64.EFI

.PHONY: all
all: $(TARGET)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

$(EFI_ELF): $(OBJS)
	$(LD) $(EFI_LDFLAGS) -L$(EFI_LIBDIR) $(EFI_CRT) $(OBJS) $(EFI_LIBS) -o $@

$(TARGET): $(EFI_ELF)
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

$(EFI_FS_BOOT):
	mkdir -p $(@)

$(EFI_FS_EXE): $(TARGET) | $(EFI_FS_BOOT)
	cp $(<) $(@)

test: $(EFI_FS_EXE)
	qemu-system-x86_64 -drive file=fat:rw:$(EFI_FS),format=raw \
		-machine graphics=off \
		-nographic \
		-bios OVMF.fd \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		--no-reboot \
		|| true

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(EFI_FS)
