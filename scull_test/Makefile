target := scull_test

src_dir := src
obj_dir := obj

source := $(wildcard $(src_dir)/*.c)
object := $(addprefix $(obj_dir)/,$(patsubst %.c,%.o,$(notdir $(source))))

CC :=gcc
CFLAGS += -Iinclude -fPIC
LDFLAGS += -lpthread

all:$(target)

$(target):$(object)
	$(CC) -o $@ $^ $(LDFLAGS)
$(obj_dir)/%.o:$(src_dir)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)
$(object): | $(obj_dir)
$(obj_dir):
	@mkdir -p $@
.PHONY:clean
clean:
	@rm -rf $(target) $(obj_dir)
