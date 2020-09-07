TARGET = hide_module
obj-m := ${TARGET}ko.o
${TARGET}ko-objs := ${TARGET}.o ../implementation/func.o


all:
		make modules -C "/lib/modules/$(shell uname -r)/build" M=$(PWD)


clean:
		make clean -C "/lib/modules/$(shell uname -r)/build" M=$(PWD)
		
