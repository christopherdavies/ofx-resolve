UNAME_SYSTEM := $(shell uname -s)

CUDAPATH ?= /usr/local/cuda
NVCC = ${CUDAPATH}/bin/nvcc
# CXXFLAGS = -fvisibility=hidden -I../../openfx/Support/include -I../../openfx/include # for OFX 1.4
CXXFLAGS = -fvisibility=hidden -I../OpenFX-1.3/include -I../Support/include # for OFX 1.3


ifeq ($(UNAME_SYSTEM), Linux)
#	AMDAPP_PATH ?= /opt/AMDAPP
#	CXXFLAGS += -I${AMDAPP_PATH}/include -fPIC
	CXXFLAGS += -fPIC -I${CUDAPATH}/include
	NVCCFLAGS = --compiler-options="-fPIC"
	ARCH = Linux-x86-64
	CUFLAGS = -shared -fvisibility=hidden -L${CUDAPATH}/lib64 -L/usr/lib64/nvidia -lcuda -lcudart
	LDFLAGS = -shared $(CUFLAGS)
	OFX_DIR = /usr/OFX/Plugins
	BUNDLE_DIR = LegalHdrPlugin.ofx.bundle/Contents/$(ARCH)/
else
	LDFLAGS = -bundle -fvisibility=hidden -L${CUDAPATH}/lib64 -lcudart -F/Library/Frameworks -framework AppKit # -framework OpenCL -framework Metal
	BUNDLE_DIR = LegalHdrPlugin.ofx.bundle/Contents/MacOS/
#	METAL_OBJ = MetalKernel.o
endif

LegalHdrPlugin.ofx: LegalHdrPlugin.o CudaKernel.o ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp LegalHdrPlugin.ofx $(BUNDLE_DIR)
	# cp -rf LegalHdrPlugin.ofx.bundle /Library/OFX/Plugins/

# VERSION WITH METAL AND OPENCL
#LegalHdrPlugin.ofx: LegalHdrPlugin.o CudaKernel.o $(METAL_OBJ) OpenCLKernel.o ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
#	$(CXX) $^ -o $@ $(LDFLAGS)
#	mkdir -p $(BUNDLE_DIR)
#	cp LegalHdrPlugin.ofx $(BUNDLE_DIR)


CudaKernel.o: CudaKernel.cu
	${NVCC} -c $< $(NVCCFLAGS)

#MetalKernel.o: MetalKernel.mmsetGPURenderArgs
#	$(CXX) -c $< $(CXXFLAGS)

%.o: ../../openfx/Support/Library/%.cpp
	$(CXX) -c $< $(CXXFLAGS)

clean:
	rm -f *.o *.ofx
	rm -fr LegalHdrPlugin.ofx.bundle
