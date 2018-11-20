
PROG = radixhash
OBJS = handler.o preprocess.o results.o rhjoin.o query.o inter_res.o filter.o relation_list.o relation_map.o
SRCS = handler.c preprocess.c results.c rhjoin.c query.c inter_res.c filter.c relation_list.c relation_map.c
HEADER = preprocess.h results.h rhjoin.h structs.h query.h inter_res.h filter.h relation_list.h relation_map.h

$(PROG): $(OBJS)
	gcc -g $(OBJS) -o $(PROG)


handler.o: handler.c
	gcc -g3 -c handler.c
	
preprocess.o:
	gcc -g3 -c preprocess.c
	
results.o:
	gcc -g3 -c results.c
	
rhjoin.o:
	gcc -g3 -c rhjoin.c

query.o:
	gcc -g3 -c query.c

inter_res.o:
	gcc -g3 -c inter_res.c

filter.o:
	gcc -g3 -c filter.c

relation_map.o:
	gcc -g3 -c relation_map.c

relation_list.o:
	gcc -g3 -c relation_list.c

clean:
	rm $(OBJS) $(PROG)
