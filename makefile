COMPILER = clang++
CCFLAGS = -w
LPFLAGS = -w -lpthread -ltbb -ltbbmalloc
CCINCLUDES = -I. -O3 
TOPK = topk.o docopt.o
ATOPK = atopk.o docopt.o
TOPKTHREAD = topk-thread.o docopt.o
TOPKTHREADFREELOCK = topk-thread-freelock.o docopt.o
TOPKTHREADTBB = topk-thread-tbb.o docopt.o
TOPKTHREADSKIP = topk-thread-skip.o docopt.o
TOPKTHREADSKIPFREE = topk-thread-skipfree.o docopt.o
TOPKTHREADRANDOM = topk-thread-random.o docopt.o
TOPKTHREADQUEUE = topk-thread-queue.o docopt.o
PPJOIN = ppjoin.o docopt.o
PPJOIN2 = ppjoin2.o docopt.o
PEL = pel.o docopt.o
PPJOINTHREAD = ppjoin-thread.o docopt.o
PPJOINTHREADAVG = ppjoin-thread-avg.o docopt.o
PPJOINTHREADBLOCK = ppjoin-thread-block.o docopt.o
BIN2BIL = bin2bil.o docopt.o
BIN2GRAPH = bin2graph.o docopt.o

all: ${BIN2BIL} ${BIN2GRAPH} ${TOPK} ${ATOPK} ${TOPKTHREAD} ${TOPKTHREADQUEUE} ${TOPKTHREADRANDOM} ${TOPKTHREADFREELOCK} ${TOPKTHREADSKIP} ${TOPKTHREADSKIPFREE} ${TOPKTHREADTBB} ${PPJOIN} ${PPJOIN2} ${PEL} ${PPJOINTHREAD}  ${PPJOINTHREADAVG} ${PPJOINTHREADBLOCK}
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -o bin2bil ${BIN2BIL}
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -o bin2graph ${BIN2GRAPH}
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -o topk ${TOPK}
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -o atopk ${ATOPK}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o topk-thread ${TOPKTHREAD}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o topk-thread-freelock ${TOPKTHREADFREELOCK}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o topk-thread-tbb ${TOPKTHREADTBB}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o topk-thread-skip ${TOPKTHREADSKIP}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o topk-thread-skipfree ${TOPKTHREADSKIPFREE}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o topk-thread-random ${TOPKTHREADRANDOM}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o topk-thread-queue ${TOPKTHREADQUEUE}
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -o ppjoin ${PPJOIN}
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -o ppjoin2 ${PPJOIN2}
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -o pel ${PPJOIN2}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o ppjoin-thread ${PPJOINTHREAD}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o ppjoin-thread-avg ${PPJOINTHREADAVG}
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -o ppjoin-thread-block ${PPJOINTHREADBLOCK}

docopt.o: docopt.cpp/docopt.cpp 
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c docopt.cpp/docopt.cpp

bin2bil.o: bin2bil.cpp
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c bin2bil.cpp

bin2graph.o: bin2graph.cpp
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c bin2graph.cpp
	
topk.o: topk.cpp
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c topk.cpp

atopk.o: atopk.cpp
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c atopk.cpp

topk-thread.o: topk-thread.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c topk-thread.cpp

topk-thread-freelock.o: topk-thread-freelock.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c topk-thread-freelock.cpp

topk-thread-tbb.o: topk-thread-tbb.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c topk-thread-tbb.cpp

topk-thread-skip.o: topk-thread-skip.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c topk-thread-skip.cpp

topk-thread-skipfree.o: topk-thread-skipfree.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c topk-thread-skipfree.cpp

topk-thread-random.o: topk-thread-random.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c topk-thread-random.cpp

topk-thread-queue.o: topk-thread-queue.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c topk-thread-queue.cpp

ppjoin.o: ppjoin.cpp
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c ppjoin.cpp

ppjoin2.o: ppjoin2.cpp
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c ppjoin2.cpp

pel.o: pel.cpp
	${COMPILER} ${CCINCLUDES} ${CCFLAGS} -c pel.cpp

ppjoin-thread.o: ppjoin-thread.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c ppjoin-thread.cpp

ppjoin-thread-avg.o: ppjoin-thread-avg.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c ppjoin-thread-avg.cpp

ppjoin-thread-block.o: ppjoin-thread-block.cpp
	${COMPILER} ${CCINCLUDES} ${LPFLAGS} -c ppjoin-thread-block.cpp

clean:
	rm -f bin2bil bin2graph topk btopk2 topkcol atopk btopk btopk-thread ppjoin ppjoin2 bppjoin bppjoin2 bppjoin-thread ppjoin-thread topk-thread topk-thread-queue topk-thread-random topk-thread-skip topk-thread-skipfree topk-thread-tbb topk-thread-freelock *.o
