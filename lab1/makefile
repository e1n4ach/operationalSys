# Имя исполняемого файла
TARGET = solution

# Компилятор
CC = gcc

# Флаги компиляции
CFLAGS = -o

# Цель по умолчанию
all: $(TARGET)

# Правило для сборки исполняемого файла
$(TARGET): solution.c
	$(CC) $(CFLAGS) $(TARGET) solution.c

# Правило для запуска программы
run: $(TARGET)
	./$(TARGET)

# Правило для очистки сгенерированных файлов
clean:
	rm -f $(TARGET)

.PHONY: all clean run