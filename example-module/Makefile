CONFIG=../../yazpp/yazpp-config
SO=metaproxy_filter_myfilter.so

all: $(SO)

$(SO): filter_myfilter.cpp
	$(CXX) -shared `$(CONFIG) --cflags` -I ../src $< -o $(SO) ../src/.libs/libmetaproxy.so -ldl

clean:
	rm *.so

run: all
	../src/metaproxy -c config.xml
