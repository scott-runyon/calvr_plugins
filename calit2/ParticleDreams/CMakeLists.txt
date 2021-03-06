SET(LIB_NAME ParticleDreams)
SET(PLUGIN_HEADERS
    ParticleDreams.h
    PDObject.h
    CudaHelper.h
)

FIND_PACKAGE(CUDA)
FIND_PACKAGE(OASCLIENT)

CUDA_ADD_LIBRARY(${LIB_NAME} 
    CudaParticle.cu
    ParticleDreams.cpp
    PDObject.cpp
    CudaHelper.cpp
    "SHARED" OPTIONS --ptxas-options=-v)

INCLUDE_DIRECTORIES(${OSG_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${OASCLIENT_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${OSG_LIBRARIES})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${CUDA_LIBRARIES})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${OASCLIENT_LIBRARY})

ADD_CALVR_LIBRARIES(${LIB_NAME})

INSTALL(TARGETS ${LIB_NAME} DESTINATION lib/plugins)
