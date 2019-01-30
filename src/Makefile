
# ShiftAC's C/C++ makefile template for mkcproj 1.2

TARGET := .
PACKAGE_PREFIX := m3
CC := gcc
CXX := g++
CMACRO := -D "PROGNAME=\"$(PACKAGE_PREFIX)\"" \
-D "VERSION=\"0.5.5.0095\"" -D ENABLE_AVX -D ENABLE_CWND_AWARE #  -D ENABLE_DEBUG_LOG
CXXMACRO := $(CMACRO)
COMMONFLAGS := -O2 -g -Wall -I ./include -rdynamic -Werror -mavx2
CFLAGS := $(CMACRO) $(COMMONFLAGS)
CXXFLAGS := $(CXXMACRO) $(COMMONFLAGS) -std=c++11
SRC := Log.c NetworkUtils.c ArrayMetricProvider.cc AsynchronousTCPContext.cc BlockAllocator.cc BlockRRConnectionSetHandler.cc Config.cc Connection.cc ConnectionClassifier.cc ConnectionManager.cc ConnectionMetadata.cc ContextManager.cc DataMessage.cc DataMessageHeader.cc DeviceMetadata.cc DumbTCPCallback.cc ForwardModule.cc HTTPDetector.cc HTTPMessage.cc Interface.cc InterfaceManager.cc InternalMessage.cc InternalMessageParser.cc IPPacket.cc KernelMessage.cc LivePCAP.cc Message.cc MobileRelay.cc NaiveTCPMessageProvider.cc NaiveTCPStreamWatcher.cc OfflinePCAP.cc PacketBufferAllocator.cc PacketFilter.cc PacketInterceptor.cc PCAP.cc PacketReader.cc PolicyManager.cc ProgramPrototype.cc ProtocolMetric.cc Proxy.cc RawSocket.cc RobustFileStream.cc RRConnectionSetHandler.cc ServerInterfaceManager.cc ServerPacketSource.cc Session.cc SimpleExecutable.cc StringUtils.cc TCPBasicMetric.cc TCPStateMachine.cc ThreadUtils.cc DatabaseProvider.cc GreedyConnectionSetHandler.cc ThresholdConnectionSetHandler.cc
GEN_SRC := Reflect.cc bpf_gram.c bpf_lex.c
OUT := $(addsuffix .o, $(basename $(SRC) $(GEN_SRC)))
SCRIPTS_DIR := ./scripts
BIN_SCRIPT := initdevice.sh
LIB := -lpthread -lpcap -lcjson -lhdf5_cpp -lhdf5_serial -ll
PROGS := MobileRelayApp ProxyApp $(BIN_SCRIPT)
PROGNAMES := $(addprefix $(PACKAGE_PREFIX)-,$(PROGS))

$(PACKAGE_PREFIX)-%: %.cpp $(OUT)
	$(CXX) $(CXXFLAGS) -o $(TARGET)/$(shell echo $@ | tr '[A-Z]' '[a-z]') $< $(OUT) $(LIB)

$(PACKAGE_PREFIX)-%: %.cc $(OUT)
	$(CXX) $(CXXFLAGS) -o $(TARGET)/$(shell echo $@ | tr '[A-Z]' '[a-z]') $< $(OUT) $(LIB)

$(PACKAGE_PREFIX)-%.sh: $(SCRIPTS_DIR)/%.sh
	cp $< $(TARGET)/$@

$(PACKAGE_PREFIX)-%: %.c $(OUT)
	$(CC) $(CFLAGS) -o $(TARGET)/$(shell echo $@ | tr '[A-Z]' '[a-z]') $< $(OUT) $(LIB)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.cpp:
	$(SCRIPTS_DIR)/generate$@.sh > $@

%.cc:
	$(SCRIPTS_DIR)/generate$@.sh > $@

%.c:
	$(SCRIPTS_DIR)/generate$@.sh > $@
	
.PHONY: all
all: depend $(GEN_SRC) $(OUT) $(PROGNAMES)

.PHONY: sourceClean
sourceClean:
	-rm $(GEN_SRC)

.PHONY: clean
clean: sourceClean
	-rm $(OUT) $(PROGNAMES) *~ Makefile.dep 2> /dev/null
	-rm NetEmulator 2> /dev/null
	-rm bpf_*

.PHONY: linecount
linecount: clean
	-find | xargs cat 2> /dev/null | wc -l

depend:
	-makedepend -f- -- $(CFLAGS) -- $(SRC) > Makefile.dep 2> /dev/null

# make `make` happy...
Makefile.dep:
	touch Makefile.dep

bpf_gram.c: bpf.y
	bison bpf.y -o bpf_gram.c -d

bpf_lex.c: bpf.l
	flex -o bpf_lex.c bpf.l 

bpf_lex.o: bpf_lex.c
	$(CC) $(CFLAGS) -w -c $<

-include Makefile.dep
