CONFIG=../../yazpp/yazpp-config
SO=metaproxy_filter_myfilter.so
$(SO): filter_myfilter.cpp
	$(CXX) -shared -nostdlib `$(CONFIG) --cflags` -I ../src $< -o $(SO) ../src/.libs/libmetaproxy.so

clean:
	rm $(SO)
run: $(SO)
	../src/metaproxy -c config.xml
