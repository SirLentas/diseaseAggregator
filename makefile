all: worker diseaseAggregator

worker: ./src/worker.c ./structs/file_list.c ./structs/summary_list.c ./structs/records.c
	gcc -g3 -o worker ./src/worker.c ./structs/file_list.c ./structs/summary_list.c ./structs/records.c

diseaseAggregator: ./src/main.c ./structs/country_list.c ./structs/summary_list.c
	gcc -g3 -o diseaseAggregator ./src/main.c ./structs/country_list.c ./structs/summary_list.c

clean:
	rm -f diseaseAggregator worker
