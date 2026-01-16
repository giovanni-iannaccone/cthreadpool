CC := gcc
CFLAGS := -Wall -Wextra

TEST_SRCS := $(wildcard test/*.c)
TEST_EXEC := $(TEST_SRCS:.c=)

.PHONY: test clean

test: $(TEST_EXEC)
	@$(foreach TEST_EXEC, $?, ./$(TEST_EXEC);)

%: %.c
	@$(CC) $(CFLAGS) $< -o $@

clean:
	@rm -rf $(TEST_EXEC)
