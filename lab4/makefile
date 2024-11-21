# Компилятор и флаги
CC = gcc
CFLAGS = -c -fPIC
LDFLAGS = -shared
DLFLAGS = -ldl
MATH_LIB = -lm
RM = rm -f

# Исходные файлы
DERIVATIVE_SRCS = libderivative_forward.c libderivative_central.c
CONVERTER_SRCS = libconverter_binary.c libconverter_ternary.c
PROGRAMS = program1 program2

# Цели
all: $(PROGRAMS)

# Динамические библиотеки
libderivative.so: libderivative_forward.o libderivative_central.o
	$(CC) $(LDFLAGS) -o $@ $^ $(MATH_LIB)

libconverter.so: libconverter_binary.o libconverter_ternary.o
	$(CC) $(LDFLAGS) -o $@ $^

# Компиляция исходников
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Сборка программ
program1: program1.c libderivative.so libconverter.so
	$(CC) -o $@ $< -L. -lderivative -lconverter $(MATH_LIB)

program2: program2.c libderivative.so libconverter.so
	$(CC) -o $@ $< $(DLFLAGS) $(MATH_LIB)

# Запуск программ
run1: program1
	LD_LIBRARY_PATH=$(PWD):$$LD_LIBRARY_PATH ./program1

run2: program2
	LD_LIBRARY_PATH=$(PWD):$$LD_LIBRARY_PATH ./program2

# Очистка
clean:
	$(RM) *.o *.so $(PROGRAMS)
