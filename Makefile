CC	= /usr/bin/gcc
CPP	= /usr/bin/g++
CFLAGS	= -ggdb \
	  -std=gnu11 -fPIC -O2 -I include
WFLAGS 	= -Wall -Wextra -Wshadow -Wpointer-arith \
	  -Wcast-qual -Wcast-align -Wwrite-strings \
	  -Wmissing-prototypes -Winline \
	  -Wno-missing-field-initializers
CPPFLAGS = -ggdb -std=c++11 -O2 -I include -Wall -Wextra

LDFLAGS = -lcunit -Llib -lpalloc

OBJDIR	= obj
BINDIR  = bin
LIBDIR	= lib
TEST_OBJDIR = obj/test


vpath %.cpp src
vpath %.c src test
vpath %.h include test
vpath %.o $(OBJDIR) $(TEST_OBJDIR)
vpath %.so $(LIBDIR)


.PHONY: all
all: $(LIBDIR)/libpalloc.so docs

$(LIBDIR)/libpalloc.so: $(OBJDIR)/pool.o \
			$(OBJDIR)/type_info.o \
			$(OBJDIR)/pool_iterator.o \
			$(OBJDIR)/reference_table.o \
			$(OBJDIR)/pool_map.o \
			$(OBJDIR)/gc.o
	$(CC) $(CFLAGS) $(WFLAGS) -shared -Wl,-soname,$@ -o $@ $^

$(OBJDIR)/%.o: %.cpp %.h 
	$(CPP) $(CPPFLAGS) $< -c -o $@

$(OBJDIR)/%.o: %.c %.h pool_private.h
	$(CC) $(CFLAGS) $(WFLAGS) $< -c -o $@ 

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(WFLAGS) $< -c -o $@ 

$(TEST_OBJDIR)/%.o: %.c %.h pool_private.h
	$(CC) $(CFLAGS) -Wall -Wextra -Wno-cast-qual $< -c -o $@ 

$(BINDIR)/test_project: $(TEST_OBJDIR)/test_project.o \
			$(TEST_OBJDIR)/test_type_info.o \
			$(TEST_OBJDIR)/test_pool.o \
			$(TEST_OBJDIR)/test_iterator.o \
			$(TEST_OBJDIR)/test_reference_table.o \
			$(TEST_OBJDIR)/test_pool_map.o \
			$(TEST_OBJDIR)/test_gc.o \
			$(LIBDIR)/libpalloc.so
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BINDIR)/alloc_benchmark: alloc_benchmark.c $(LIBDIR)/libpalloc.so \
					     $(TEST_OBJDIR)/test_type_info.o
	$(CC) $(CFLAGS) $(WFLAGS) $(LDFLAGS) $< $(TEST_OBJDIR)/test_type_info.o -o $@

$(BINDIR)/map_benchmark: map_benchmark.c $(LIBDIR)/libpalloc.so \
					 $(OBJDIR)/linked_list.o \
					 $(TEST_OBJDIR)/test_type_info.o 
	$(CC) $(CFLAGS) $(WFLAGS) $(LDFLAGS) $^ -o $@

$(BINDIR)/map_with_deletions_benchmark: map_with_deletions_benchmark.c \
					 $(OBJDIR)/linked_list.o \
					 $(OBJDIR)/benchmark_vector_map.o \
					 $(LIBDIR)/libpalloc.so \
					 $(TEST_OBJDIR)/test_type_info.o 
	$(CC) $(CFLAGS) $(WFLAGS) $(LDFLAGS) $^ -o $@  -lstdc++

$(BINDIR)/benchmark_bintree:	benchmark_bintree.c \
				$(OBJDIR)/benchmark_stl_tree.o \
				$(TEST_OBJDIR)/test_type_info.o 
	$(CC) $(CFLAGS) $(WFLAGS) $(LDFLAGS) $^ -o $@  -lstdc++

.PHONY: docs
docs:
	doxygen Doxyfile #&&\
	#cd doc/latex &&\
	#make

.PHONY: benchmark
benchmark: $(BINDIR)/alloc_benchmark \
	   $(BINDIR)/map_benchmark \
	   $(BINDIR)/map_with_deletions_benchmark \
	   $(BINDIR)/benchmark_bintree
	@for b in $^ ; do \
		echo -ne "\nRunning benchmark $$b\n"; \
		$$b; \
	done

.PHONY: test
test: $(BINDIR)/test_project
	@for t in $^ ; do \
		echo -ne "\nRunning test suite $$t"; \
		$$t; \
		echo -ne "\n\nRunning Valgrind on test suite $$t\n"; \
		(valgrind --tool=memcheck --leak-check=full $$t 1> /dev/null) 2>&1 |\
		sed -e '/Invalid write of/,+7d'; \
		true; \
	done

.PHONY: verbose_test
verbose_test: $(BINDIR)/test_project
	@for t in $^ ; do \
		echo -ne "\nRunning test suite $$t"; \
		$$t; \
		echo -ne "\n\nRunning Valgrind on test suite $$t\n"; \
		valgrind --tool=memcheck --leak-check=full $$t 1> /dev/null; \
		true; \
	done

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *.o
	rm -f $(TEST_OBJDIR)/*.o
	rm -fr doc/*

.PHONY: mrproper
mrproper: clean
	rm -f $(BINDIR)/*
	rm -f $(LIBDIR)/*
	
