CONFIG=../metaproxy-config
SO=metaproxy_filter_myfilter.so

all: $(SO)

$(SO): filter_myfilter.cpp
	echo $(LIBS)
	$(CXX) -shared `$(CONFIG) --cflags` -fPIC -I ../src $< \
		-o $(SO) `$(CONFIG) --libs`

clean:
	rm -f *.so

run: all
	../src/metaproxy -c config.xml
