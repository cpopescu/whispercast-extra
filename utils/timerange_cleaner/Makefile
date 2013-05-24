CC=ghc
EXE=TimeRangeCleaner TimeRangeRestore

all: $(EXE)

TimeRangeCleaner: TimeRangeCleaner.hs
	$(CC) -o $@ $<

TimeRangeRestore: TimeRangeRestore.hs
	$(CC) -o $@ $<


.PHONY: clean

clean:
	rm -f *.o *.hi $(EXE)

