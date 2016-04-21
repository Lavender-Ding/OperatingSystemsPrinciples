lab0: lab0.c
	@ gcc -Wall -o lab0 -g lab0.c

.PHONY: clean check dist

clean:
	@ rm -f *.o lab0 *~ core

dist:
	@ tar -zcvf lab0-604588135.tar.gz *

check: lab0
	@ echo "-----------------------------------------"
	@ echo "There are 7 test cases to check lab0."
	@ echo "-----------------------------------------"
	@ echo "test 1: ./lab0 --input=IN --output=OUT"
	@ echo "A simple test input file to check lab0" > IN
	@ ./lab0 --input=IN --output=OUT
	@ if [ $$? -eq 0 ]; then \
		if cmp -s IN OUT; then \
			echo "Success in test 1!"; \
		else \
			echo "Fail in test 1!"; \
		fi; \
	else \
		echo "Fail in test 1"; \
	fi;
	@ echo "-----------------------------------------"
	@ echo "test 2: ./lab0 --input=IN --output=OUT --segfault --catch"
	@ ./lab0 --input=IN --output=OUT --segfault --catch || \
	if [ $$? -eq 3 ]; then \
		echo "Successfully set and catch the fault!"; \
	else \
		echo "Fail to set or catch the fault!"; \
	fi;
	@ echo "-----------------------------------------"
	@ echo "test 3: ./lab0 --segfault --catch"
	@ ./lab0 --segfault --catch || \
	if [ $$? -eq 3 ]; then \
		echo "Successfully set and catch the fault!"; \
	else \
		echo "Fail to set or catch the fault!"; \
	fi;
	@ echo "-----------------------------------------"
	@ echo "test 4: ./lab0 --segfault"
	@ ./lab0 --segfault || echo ""
	@ echo "-----------------------------------------"
	@ echo "test 5: ./lab0 --input=INFILE --output=OUT"
	@ ./lab0 --input=INFILE --output=OUT || \
	if [ $$? -eq 1 ]; then \
		echo "Successfully find the error and exit with 1!"; \
	else \
		echo "Fail to exit with 1!"; \
	fi;
	@ echo "-----------------------------------------"
	@ echo "test 6: ./lab0 --input"
	@ ./lab0 --input || \
	if [ $$? -eq 1 ]; then \
		echo "Successfully find the error and exit with 1!"; \
	else \
		echo "Fail to exit with 1!"; \
	fi;
	@ echo "-----------------------------------------"
	@ echo "test 7: ./lab0 --input=IN --output=OUT (change the mode of OUT)"
	@ chmod 00000 OUT
	@ ./lab0 --input=IN --output=OUT || \
	if [ $$? -eq 2 ]; then \
		echo "Successfully find the error and exit with 2!"; \
	else \
		echo "Fail to exit with 2!"; \
	fi;
	@ chmod 00777 OUT
	@ rm -f IN OUT
	@ rm -f *.o lab0 *~ core





