UNAME_SYSTEM := $(shell uname -s)

CUDAPATH ?= /usr/local/cuda
NVCC = ${CUDAPATH}/bin/nvcc
CXXFLAGS = -fvisibility=hidden -I../OpenFX-1.3/include -I../Support/include

ifeq ($(UNAME_SYSTEM), Linux)
#	AMDAPP_PATH ?= /opt/AMDAPP
#	CXXFLAGS += -I${AMDAPP_PATH}/include -fPIC
	NVCCFLAGS = --compiler-options="-fPIC"
	LDFLAGS = -shared -fvisibility=hidden -L${CUDAPATH}/lib64 -lcuda -lcudart
	BUNDLE_DIR = LegalHdrPlugin.ofx.bundle/Contents/Linux-x86-64/
else
	LDFLAGS = -bundle -fvisibility=hidden -L${CUDAPATH}/lib -lcudart -F/Library/Frameworks -framework AppKit # -framework OpenCL -framework Metal
	BUNDLE_DIR = LegalHdrPlugin.ofx.bundle/Contents/MacOS/
#	METAL_OBJ = MetalKernel.o
endif

LegalHdrPlugin.ofx: LegalHdrPlugin.o CudaKernel.o ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp LegalHdrPlugin.ofx $(BUNDLE_DIR)
	cp -rf LegalHdrPlugin.ofx.bundle /Library/OFX/Plugins/

# VERSION WITH METAL AND OPENCL
#LegalHdrPlugin.ofx: LegalHdrPlugin.o CudaKernel.o $(METAL_OBJ) OpenCLKernel.o ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
#	$(CXX) $^ -o $@ $(LDFLAGS)
#	mkdir -p $(BUNDLE_DIR)
#	cp LegalHdrPlugin.ofx $(BUNDLE_DIR)


CudaKernel.o: CudaKernel.cu
	${NVCC} -c $< $(NVCCFLAGS)

#MetalKernel.o: MetalKernel.mm
#	$(CXX) -c $< $(CXXFLAGS)

%.o: ../Support/Library/%.cpp
	$(CXX) -c $< $(CXXFLAGS)

clean:
	rm -f *.o *.ofx
	rm -fr LegalHdrPlugin.ofx.bundle
