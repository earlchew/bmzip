CXXFLAGS += -g -O2 -std=c++11 -fshort-wchar -DCOMPRESSION_API_EXPORT

MS-COMPRESSION = \
    Bitstream.o \
    Dictionary.o \
    compression.o \
    lznt1.o \
    lzx.o \
    xpress.o \
    xpress_huff.o

bmzip:	bmzip.o GetBootmanagerVersion.o $(MS-COMPRESSION)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

%.o:	%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%.o:	ms-compression/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY:	clean
clean:
	rm -f *.o bmzip
