CONFIG=../metaproxy-config
SO=metaproxy_filter_myfilter.so

all: $(SO) mytest

$(SO): filter_myfilter.cpp
	$(CXX) -shared `$(CONFIG) --cflags` -fPIC $< \
		-o $@ `$(CONFIG) --libs`

mytest: mytest.cpp
	$(CXX) `$(CONFIG) --cflags` $< \
		-o $@ `$(CONFIG) --libs`

clean:
	rm -f *.so mytest

run: all
	../src/metaproxy -c config.xml
