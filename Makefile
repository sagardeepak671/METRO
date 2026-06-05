.PHONY: run

run:
	./compile.sh
	./encoder
	-minisat input.satinput input.satoutput
	./decoder 
	python3 format_checker.py input


runn:
	python3 testcase_gen.py --N 40 --M 40 --K 30 --J 3 --mode constructive --count 1 --prefix input
	mv input_000.city input.city
	./compile.sh
	./encoder
	-minisat input.satinput input.satoutput
	./decoder 
	python3 format_checker.py input

clean:
	rm -f encoder decoder input.satinput input.satoutput input.metromap